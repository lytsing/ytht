#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <ght_hash_table.h>
#include <signal.h>
#include "bbs.h"

int
initBlogLogMSQ()
{
	int msqid;
	struct msqid_ds buf;
	msqid = msgget(getBBSKey(BLOGLOG_MSQ), IPC_CREAT | 0664);
	if (msqid < 0)
		return -1;
	msgctl(msqid, IPC_STAT, &buf);
	buf.msg_qbytes = 1024 * 512;
	msgctl(msqid, IPC_SET, &buf);
	return msqid;
}

char *
rcvlog(int msqid, int nowait)
{
	static char buf[1024];
	struct mymsgbuf *msgp = (struct mymsgbuf *) buf;
	int retv;
	retv =
	    msgrcv(msqid, msgp, sizeof (buf) - sizeof (msgp->mtype) - 2, 0,
		   (nowait ? IPC_NOWAIT : 0) | MSG_NOERROR);
	while (retv > 0 && msgp->mtext[retv - 1] == 0)
		retv--;
	if (retv <= 0)
		return NULL;
	msgp->mtext[retv] = 0;
	return msgp->mtext;
}

char *
getfilename(int *day)
{
	static char logf[256];
	time_t dtime;
	struct tm *n;
	time(&dtime);
	n = localtime(&dtime);
	sprintf(logf, MY_BBS_HOME "/newtrace/%d-%02d-%02d.blog",
		1900 + n->tm_year, 1 + n->tm_mon, n->tm_mday);
	*day = n->tm_mday;
	return logf;
}

struct event {
	struct event *next;
	char *eventstr;
	time_t t;
} *eventhead = NULL, *eventtail = NULL;
int eventcount = 0;

ght_hash_table_t *eventtable = NULL;

int
expireevent()
{
	struct event *e;
	time_t now_t = time(NULL);
	while (eventhead) {
		e = eventhead;
		if (now_t - e->t < 60 * 60 * 24 * 2 && eventcount < 200000)
			break;
		eventhead = e->next;
		ght_remove(eventtable, strlen(e->eventstr), e->eventstr);
		eventcount--;
		free(e->eventstr);
		free(e);
	}
	if (!eventhead)
		eventtail = NULL;
	return 0;
}

int
filterlog(char *eventstr)
{
	struct event *e;
	int len = strlen(eventstr) + 1;

	expireevent();

	if (ght_get(eventtable, len, eventstr))
		return 1;

	//record this event...
	e = malloc(sizeof (*e));
	if (!e)
		return 1;
	e->eventstr = malloc(len);
	if (!e->eventstr) {
		free(e);
		return 1;
	}
	memcpy(e->eventstr, eventstr, len);
	e->t = time(NULL);
	e->next = NULL;

	if (ght_insert(eventtable, e, len, e->eventstr) < 0) {
		eventcount++;
		free(e->eventstr);
		free(e);
		return 1;
	}
	if (eventhead == NULL) {
		eventhead = e;
		eventtail = e;
	} else {
		eventtail->next = e;
		eventtail = e;
	}
	return 0;
}

int sync_flag = 0;
int fd = -1;
char buf[100 * 1024];
int n = 0;

static void
set_sync_flag(int signno)
{
	sync_flag = 1;
}

static void
write_back_all(int signno)
{
	if (fd >= 0)
		write(fd, buf, n);
	exit(0);
}

int
main()
{
	char *str, *ptr;
	int l, msqid, i;
	int lastday, day;

	umask(027);

	msqid = initBlogLogMSQ();
	if (msqid < 0)
		return -1;

	if (fork())
		return 0;
	setsid();
	if (fork())
		return 0;

	close(0);
	close(1);
	close(2);

	eventtable = ght_create(200000);
	if (!eventtable)
		return -1;

	fd = open(MY_BBS_HOME "/reclog/bloglogd.lock", O_CREAT | O_RDONLY,
		  0660);
	if (flock(fd, LOCK_EX | LOCK_NB) < 0)
		return -1;

	signal(SIGHUP, set_sync_flag);
	signal(SIGTERM, write_back_all);
	fd = open(getfilename(&lastday), O_WRONLY | O_CREAT | O_APPEND, 0660);
	i = 0;
	while (1) {
		while ((str = rcvlog(msqid, 1))) {
			ptr = strchr(str, ' ');
			if (!ptr)
				continue;
			ptr++;
			if (filterlog(ptr))
				continue;

			getfilename(&day);
			if (day != lastday) {
				if (fd >= 0) {
					write(fd, buf, n);
					n = 0;
					i = 0;
					close(fd);
				}
				fd = open(getfilename(&lastday),
					  O_WRONLY | O_CREAT | O_APPEND, 0660);
			}
			ptr = strchr(ptr, ' ');
			if (ptr)
				ptr = strchr(ptr + 1, ' ');
			if (!ptr)
				continue;
			*ptr = '\n';
			ptr[1] = 0;

			l = strlen(str);
			if (l > sizeof (buf) - n) {
				write(fd, buf, n);
				sync_flag = 0;
				n = 0;
				i = 0;
			}
			memcpy(buf + n, str, l);
			n += l;
		}
		sleep(1);
		i++;
		if (i < 30 && !sync_flag)
			continue;
		write(fd, buf, n);
		n = 0;
		i = 0;
		sync_flag = 0;
	}
}
