#define _GNU_SOURCE
#include <string.h>
#include "ythtbbs.h"
#include "../local_utl/usesql.h"
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
extern int errno;

MYSQL *sql = NULL;
int
connsql()
{
	if (sql)
		return 0;
	sql = mysql_init(NULL);
	mysql_real_connect(sql, "localhost", SQLUSER, SQLPASSWD, SQLDB, 0, NULL,
			   0);
	return 0;
}

void
closesql()
{
	if (sql) {
		mysql_close(sql);
		sql = NULL;
	}
}

struct bbsinfo bbsinfo;

static int
yspam_quit(int connfd, int ret)
{
	struct yspam_proto_ret head_ret;

	bzero(&head_ret, sizeof (struct yspam_proto_ret));
	head_ret.key = htonl(YSPAM_KEY);
	head_ret.status = -ret;
	send(connfd, &head_ret, sizeof (head_ret), 0);
	return ret;
}

static int
do_feed_ham(char *userid, unsigned long long mailid, int magic, struct yspam_proto_ret *yret)
{
	char sqlstr[1024];
	char euser[IDLEN * 2 + 1];
	char filename[256];
	int feedback;
	MYSQL_RES *result;
	MYSQL_ROW row;
	char *title, *sender;
	int ret;
	mysql_escape_string(euser, userid, strlen(userid));
	snprintf(sqlstr, sizeof (sqlstr),
		 "select Title, Sender, Feedback from spam where Id='%llu' and Magic='%d' and User='%s'",
		 mailid, magic, euser);
	if (mysql_query(sql, sqlstr) < 0)
		return -YSPAM_ERR_SQL;
	result = mysql_store_result(sql);
	if (result == NULL)
		return -YSPAM_ERR_SQL;
	if (mysql_num_rows(result) != 1) {
		mysql_free_result(result);
		return -YSPAM_ERR_SQL;
	}
	row = mysql_fetch_row(result);
	title = row[0];
	sender = row[1];
	feedback = atoi(row[2]);
	sprintf(filename, MY_BBS_HOME "/maillog/%llu.bbs", mailid);
	if (title[0] == 0)
		ret = system_mail_file(filename, userid, "ÎÞÌâ", sender);
	else
		ret = system_mail_file(filename, userid, title, sender);
	mysql_free_result(result);
	if (ret < 0)
		return -YSPAM_ERR_SQL;
	snprintf(sqlstr, sizeof (sqlstr),
		 "update spam set Mailtype='1', Filename='M.%d.A', Feedback='1' where Id='%llu' and Magic='%d' and User='%s'",
		 ret, mailid, magic, euser);
	mysql_query(sql, sqlstr);
	if (feedback)
		snprintf(sqlstr, sizeof (sqlstr), "/usr/bin/bogofilter -d /home/bbs/.bogofilter -Sn -M < " MY_BBS_HOME "/maillog/%llu", mailid);
	else
		snprintf(sqlstr, sizeof (sqlstr), "/usr/bin/bogofilter -d /home/bbs/.bogofilter -n -M < " MY_BBS_HOME "/maillog/%llu", mailid);
	system(sqlstr);
	return 0;
}

static int
do_getspam(char *userid, unsigned long long mailid, int magic, struct yspam_proto_ret *yret)
{
	char sqlstr[1024];
	char euser[IDLEN * 2 + 1];
	MYSQL_RES *result;
	MYSQL_ROW row;
	mysql_escape_string(euser, userid, strlen(userid));
	snprintf(sqlstr, sizeof (sqlstr),
		 "select Title, Sender from spam where Id='%llu' and Magic='%d' and User='%s'",
		 mailid, magic, euser);
	if (mysql_query(sql, sqlstr) < 0)
		return -YSPAM_ERR_SQL;
	result = mysql_store_result(sql);
	if (result == NULL)
		return -YSPAM_ERR_SQL;
	if (mysql_num_rows(result) != 1) {
		mysql_free_result(result);
		return -YSPAM_ERR_SQL;
	}
	row = mysql_fetch_row(result);
	strncpy(yret->param.mail.title, row[0], YSPAM_TITLE_LEN);
	yret->param.mail.title[YSPAM_TITLE_LEN - 1] = 0;
	strncpy(yret->param.mail.sender, row[1], YSPAM_SENDER_LEN);
	yret->param.mail.sender[YSPAM_SENDER_LEN - 1] = 0;
	mysql_free_result(result);
	return 0;
}

static int
do_feed_spam(char *userid, char *filename, struct yspam_proto_ret *yret)
{
	char sqlstr[1024];
	char euser[IDLEN * 2 + 1], efilename[41];
	unsigned long long mailid;
	int feedback;
	MYSQL_RES *result;
	MYSQL_ROW row;
	mysql_escape_string(euser, userid, strlen(userid));
	mysql_escape_string(efilename, filename, strlen(filename));
	snprintf(sqlstr, sizeof (sqlstr), "select Id, Feedback from spam where User='%s' and Filename='%s' and Mailtype='1'", euser, efilename);
	if (mysql_query(sql, sqlstr) < 0)
		return -YSPAM_ERR_SQL;
	result = mysql_store_result(sql);
	if (result == NULL)
		return -YSPAM_ERR_SQL;
	if (mysql_num_rows(result) != 1) {
		mysql_free_result(result);
		return -YSPAM_ERR_SQL;
	}
	row = mysql_fetch_row(result);
	mailid = strtoull(row[0], NULL, 10);
	feedback = atoi(row[1]);
	mysql_free_result(result);
	snprintf(sqlstr, sizeof (sqlstr),
		 "update spam set Mailtype='2', Feedback='1' where User='%s' and Filename='%s'",
		 euser, efilename);
	if (mysql_query(sql, sqlstr) < 0)
		return -YSPAM_ERR_SQL;
	if (mysql_affected_rows(sql) < 1)
		return -YSPAM_ERR_NOSUCHMAIL;
	snprintf(sqlstr, sizeof (sqlstr), "/usr/bin/bogofilter -d /home/bbs/.bogofilter -sN -M < " MY_BBS_HOME "/maillog/%llu", mailid);
	system(sqlstr);
	return 0;
}

static int
do_update_filename(char *filename, unsigned long long mailid, struct yspam_proto_ret *yret)
{
	char sqlstr[1024];
	char efilename[41];
	mysql_escape_string(efilename, filename, strlen(filename));
	snprintf(sqlstr, sizeof (sqlstr),
		 "update spam set Filename='%s' where Id='%llu'",
		 efilename, mailid);
	if (mysql_query(sql, sqlstr) < 0)
		return -YSPAM_ERR_SQL;
	if (mysql_affected_rows(sql) < 1)
		return -YSPAM_ERR_NOSUCHMAIL;
	return 0;
}

static int
do_newmail(char *userid, char *title, uint8_t type, char *sender, struct yspam_proto_ret *yret)
{
	char sqlstr[1024];
	char euser[IDLEN * 2 + 1];
	char etitle[YSPAM_TITLE_LEN * 2 + 1];
	char esender[YSPAM_SENDER_LEN * 2 + 1];
	time_t now;
	int magic;
	unsigned long long mailid;
	now = time(NULL);
	getrandomint(&magic);
	mysql_escape_string(euser, userid, strlen(userid));
	mysql_escape_string(etitle, title, strlen(title));
	mysql_escape_string(esender, sender, strlen(sender));
	snprintf(sqlstr, sizeof (sqlstr),
		 "insert into spam (User, Time, Title, Filename, Magic, Mailtype, Sender) values ('%s','%lu','%s', '', '%d', '%d', '%s')",
		 euser, now, etitle, magic, type, esender);
	if (mysql_query(sql, sqlstr) < 0)
		return -YSPAM_ERR_SQL;
	mailid = mysql_insert_id(sql);
	yret->param.mailid.mailid_low = htonl(mailid & 0xFFFFFFFF);
	yret->param.mailid.mailid_high = htonl(mailid >> 32);
	yret->param.mailid.mail_magic = htonl(magic);
	return 0;
}

static int
do_getallspam(char *userid, struct yspam_proto_ret *yret, struct spamheader **sh)
{
	char sqlstr[1024];
	char euser[IDLEN * 2 + 1];
	unsigned long long mid;
	struct spamheader *sh1;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int num;
	mysql_escape_string(euser, userid, strlen(userid));
	snprintf(sqlstr, sizeof (sqlstr),
		 "select Id, Time,Title, Magic, Sender from spam where User='%s' and Mailtype='2' ORDER BY Time",
		 euser);
	if (mysql_query(sql, sqlstr) < 0) {
		*sh = NULL;
		yret->param.spamnum = htons(0);
		return -YSPAM_ERR_SQL;
	}
	result = mysql_store_result(sql);
	if (result == NULL) {
		*sh = NULL;
		yret->param.spamnum = htons(0);
		return -YSPAM_ERR_SQL;
	}
	num = mysql_num_rows(result);
	if (num < 0) {
		*sh = NULL;
		yret->param.spamnum = htons(0);
		mysql_free_result(result);
		return -YSPAM_ERR_SQL;
	}
	if (num == 0) {
		*sh = NULL;
		yret->param.spamnum = htons(0);
		mysql_free_result(result);
		return -YSPAM_ERR_NOSUCHMAIL;
	}
	*sh = malloc(sizeof (struct spamheader) * num);
	if (NULL == *sh) {
		yret->param.spamnum = htons(0);
		mysql_free_result(result);
		return -YSPAM_ERR_SQL;
	}
	yret->param.spamnum = htons(num);
	sh1 = *sh;
	while ((row = mysql_fetch_row(result))) {
		strncpy(sh1->title, row[2], 60);
		sh1->title[59] = 0;
		mid = strtoull(row[0], NULL, 10);
		sh1->mailid_low = htonl(mid & 0xFFFFFFFF);
		sh1->mailid_high = htonl(mid >> 32);
		sh1->time = htonl(strtoul(row[1], NULL, 10));
		sh1->magic = htonl(atoi(row[3]));
		strncpy(sh1->sender, row[4], 40);
		sh1->sender[39] = 0;
		sh1++;
	}
	mysql_free_result(result);
	return num;
}

static int
yspam_do_child(int connfd)
{
	int res;
	struct yspam_proto_req head_req;
	struct yspam_proto_ret head_ret;
	unsigned long long mailid;
	struct spamheader *sh = NULL;
	int count, size, retv;

	res = recv(connfd, &head_req, sizeof (head_req), 0);
	if (res != sizeof (head_req)) {
		return yspam_quit(connfd, -YSPAM_ERR_HEADER);
	}
	if (ntohl(head_req.key) != YSPAM_KEY)
		return yspam_quit(connfd, -YSPAM_ERR_KEY);
	bzero(&head_ret, sizeof(struct yspam_proto_ret));
	head_ret.key = htonl(YSPAM_KEY);
	switch (head_req.type) {
	case YSPAM_FEED_SPAM:
		res =
		    do_feed_spam(head_req.userid,
				 head_req.param.ham.filename, &head_ret);
		break;
	case YSPAM_FEED_HAM:
		mailid =
		    (((my_ulonglong) ntohl(head_req.param.spam.mailid_high)) <<
		     32) +
		    (my_ulonglong) (ntohl(head_req.param.spam.mailid_low));
		res =
		    do_feed_ham(head_req.userid, mailid,
				ntohl(head_req.param.spam.mail_magic), &head_ret);
		break;
	case YSPAM_NEWMAIL:
		res =
		    do_newmail(head_req.userid,
			       head_req.param.newmail.title,
			       head_req.param.newmail.type,
			       head_req.param.newmail.sender, &head_ret);
		break;
	case YSPAM_GETALLSPAM:
		res =
		    do_getallspam(head_req.userid, &head_ret, &sh);
		break;
	case YSPAM_UPDATE_FILENAME:
		mailid =
		    (((my_ulonglong) ntohl(head_req.param.updatemail.mailid_high)) <<
		     32) +
		    (my_ulonglong) (ntohl(head_req.param.updatemail.mailid_low));
		res = do_update_filename(head_req.param.updatemail.filename, mailid, &head_ret);
		break;
	case YSPAM_GET_SPAM:
		mailid =
		    (((my_ulonglong) ntohl(head_req.param.spam.mailid_high)) <<
		     32) +
		    (my_ulonglong) (ntohl(head_req.param.spam.mailid_low));
		res =
		    do_getspam(head_req.userid, mailid,
			       ntohl(head_req.param.spam.mail_magic), &head_ret);
		break;
	default:
		res = -YSPAM_ERR_OPT;
		break;
	}
	switch (head_req.type) {
	case YSPAM_FEED_SPAM:
	case YSPAM_FEED_HAM:
	case YSPAM_NEWMAIL:
	case YSPAM_GET_SPAM:
		if (res < 0)
			return yspam_quit(connfd, res);
		send(connfd, &head_ret, sizeof (head_ret), 0);
		return 0;
		break;
	case YSPAM_GETALLSPAM:
		if (res < 0) {
			if (sh)
				free(sh);
			return yspam_quit(connfd, res);
		}
		send(connfd, &head_ret, sizeof (head_ret), 0);
		count = 0;
		size = res * sizeof (struct spamheader);
		while (count < size) {
			while ((retv =
				send(connfd, (void *) sh + count, size - count,
				     0)) == -1) {
				if (errno != EINTR) {
					if (sh)
						free(sh);
					return -YSPAM_ERR_BROKEN;
				}
			}
		}
		count += retv;
		if (sh)
			free(sh);
		return 0;
		break;
	default:
		break;
	}
	return 0;
}

static int
yspam_service(void)
{
	int listenfd, connfd;
	int val;
	struct linger ld;
	struct sockaddr_in cliaddr, servaddr;
	int caddr_len;

	if (connsql() < 0) {
		exit(-1);
	}
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if (listenfd < 0) {
		exit(-1);
	}

	val = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *) &val,
		   sizeof (val));
	ld.l_onoff = ld.l_linger = 0;
	setsockopt(listenfd, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof (ld));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(YSPAM_PORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) <
	    0) {
		exit(-1);
	}

	listen(listenfd, 32);

	while (1) {
		connfd =
		    accept(listenfd, (struct sockaddr *) &cliaddr, &caddr_len);
		if (connfd < 0)
			continue;
		yspam_do_child(connfd);
		close(connfd);
	}
	return 0;
}

int
main(int argc, char **argv)
{
	if (daemon(1, 0))
		return -1;
	if (initbbsinfo(&bbsinfo) < 0) {
		perror("init error");
		return -1;
	}
	yspam_service();
	closesql();
	return 0;
}
