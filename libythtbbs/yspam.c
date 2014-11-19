#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
extern int errno;
#include "ythtbbs.h"

struct yspam_ctx *
yspam_init(char *host)
{
	int ret = 0;
	struct yspam_ctx *ctx;

	ctx = malloc(sizeof (struct yspam_ctx));
	if (ctx == NULL)
		return NULL;
	memset(ctx, 0, sizeof (struct yspam_ctx));
	ctx->host.sin_port = htons(YSPAM_PORT);
	ctx->host.sin_family = AF_INET;
	ret = inet_aton(host, &(ctx->host.sin_addr));
	if (ret == 0) {
		free(ctx);
		ctx = NULL;
		return NULL;
	}
	return ctx;
}

void
yspam_fini(struct yspam_ctx *ctx)
{
	if (ctx != NULL)
		free(ctx);
	return;
}

static int
yspam_proto(struct yspam_ctx *ctx, struct yspam_proto_req *yspam_req, struct yspam_proto_ret *yspam_ret)
{
	int fd, ret;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		yspam_ret->status = YSPAM_ERR_CONNECT;
		return -YSPAM_ERR_CONNECT;
	}
	if (connect(fd, (struct sockaddr *) &(ctx->host),
		    sizeof (ctx->host)) < 0) {
		close(fd);
		yspam_ret->status = YSPAM_ERR_CONNECT;
		return -YSPAM_ERR_CONNECT;
	}
	ret = send(fd, yspam_req, sizeof (struct yspam_proto_req), 0);
	if (ret < sizeof (*yspam_req)) {
		close(fd);
		yspam_ret->status = YSPAM_ERR_BROKEN;
		return -YSPAM_ERR_BROKEN;
	}
	ret = recv(fd, yspam_ret, sizeof (struct yspam_proto_ret), 0);
	if (ret != sizeof (struct yspam_proto_ret)) {
		close(fd);
		yspam_ret->status = YSPAM_ERR_BROKEN;
		return -YSPAM_ERR_BROKEN;
	}
	if (ntohl(yspam_ret->key) != YSPAM_KEY) {
		close(fd);
		yspam_ret->status = YSPAM_ERR_BROKEN;
		return -YSPAM_ERR_BROKEN;
	}
	if (yspam_ret->status > 0) {
		close(fd);
		return -yspam_ret->status;
	}
	return fd;
}

int
yspam_newmail(struct yspam_ctx *ctx, char *user, char *title, uint8_t type,
	      char *sender, unsigned long long *mailid)
{
	int ret;
	struct yspam_proto_req yspam_req;
	struct yspam_proto_ret yspam_ret;

	bzero(&yspam_req, sizeof (struct yspam_proto_req));
	yspam_req.key = htonl(YSPAM_KEY);
	if (strlen(user) > IDLEN)
		return -YSPAM_ERR_HEADER;
	strcpy(yspam_req.userid, user);
	yspam_req.type = YSPAM_NEWMAIL;
	yspam_req.param.newmail.type = type;
	strncpy(yspam_req.param.newmail.title, title, YSPAM_TITLE_LEN);
	yspam_req.param.newmail.title[YSPAM_TITLE_LEN - 1] = 0;
	strncpy(yspam_req.param.newmail.sender, sender, YSPAM_SENDER_LEN);
	yspam_req.param.newmail.sender[YSPAM_SENDER_LEN - 1] = 0;
	yspam_proto(ctx, &yspam_req, &yspam_ret);
	ret = -yspam_ret.status;
	if (ret < 0)
		return ret;
	*mailid = (((unsigned long long)ntohl(yspam_ret.param.mailid.mailid_high))<<32)+(unsigned long long)(ntohl(yspam_ret.param.mailid.mailid_low)); 
	close(ret);
	return 0;
}

int
yspam_feed_spam(struct yspam_ctx *ctx, char *user, char *filename)
{
	int ret;
	struct yspam_proto_req yspam_req;
	struct yspam_proto_ret yspam_ret;

	bzero(&yspam_req, sizeof (struct yspam_proto_req));
	yspam_req.key = htonl(YSPAM_KEY);
	if (strlen(user) > IDLEN)
		return -YSPAM_ERR_HEADER;
	strcpy(yspam_req.userid, user);
	yspam_req.type = YSPAM_FEED_SPAM;
	if (strlen(filename) >= 20)
		return -YSPAM_ERR_HEADER;
	strncpy(yspam_req.param.ham.filename, filename, 20);
	yspam_req.param.ham.filename[19] = 0;
	ret = yspam_proto(ctx, &yspam_req, &yspam_ret);
	if (ret < 0)
		return ret;
	close(ret);
	return 0;
}

int
yspam_feed_ham(struct yspam_ctx *ctx, char *user, unsigned long long mailid, int magic)
{
	int ret;
	struct yspam_proto_req yspam_req;
	struct yspam_proto_ret yspam_ret;
	unsigned int high, low;

	bzero(&yspam_req, sizeof (struct yspam_proto_req));
	yspam_req.key = htonl(YSPAM_KEY);
	yspam_req.type = YSPAM_FEED_HAM;
	if (strlen(user) > IDLEN)
		return -YSPAM_ERR_HEADER;
	strcpy(yspam_req.userid, user);
	high = mailid >> 32;
	low = mailid & 0xFFFFFFFF;
	yspam_req.param.spam.mailid_high=htonl(high);
	yspam_req.param.spam.mailid_low=htonl(low);
	yspam_req.param.spam.mail_magic = htonl(magic);
	ret = yspam_proto(ctx, &yspam_req, &yspam_ret);
	if (ret < 0)
		return ret;
	close(ret);
	return 0;
}

int
yspam_getallspam(struct yspam_ctx *ctx, char *user, struct spamheader **sh, uint16_t *spamnum)
{
	int ret;
	struct yspam_proto_req yspam_req;
	struct yspam_proto_ret yspam_ret;
	int count, size, retv;
	int fd;

	bzero(&yspam_req, sizeof (struct yspam_proto_req));
	yspam_req.key = htonl(YSPAM_KEY);
	yspam_req.type = YSPAM_GETALLSPAM;
	if (strlen(user) > IDLEN)
		return -YSPAM_ERR_HEADER;
	strcpy(yspam_req.userid, user);
	ret = yspam_proto(ctx, &yspam_req, &yspam_ret);
	if (ret < 0) {
		*sh = NULL;
		*spamnum = 0;
		return ret;
	}
	fd = ret;
	*spamnum = ntohs(yspam_ret.param.spamnum);
	if (*spamnum == 0) {
		*sh = NULL;
		close(fd);
		return -YSPAM_ERR_NOSUCHMAIL;
	}
	size = *spamnum * sizeof(struct spamheader);
	*sh = malloc(size);
	if (*sh == NULL) {
		*spamnum = 0;
		close(fd);
		return -YSPAM_ERR_SQL;
	}
	count = 0;
	while (count < size) {
		while ((retv = recv(fd, (void *)(*sh) + count, size - count, 0))==-1) {
			if (errno != EINTR) {
				*spamnum = 0;
				*sh = NULL;
				close(fd);
				return -YSPAM_ERR_BROKEN;
			}
		}
		count += retv;
	}
	close(fd);
	return 0;
}

int
yspam_getspam(struct yspam_ctx *ctx, char *user, unsigned long long mailid, int magic, char *title, char *sender)
{
	int ret;
	struct yspam_proto_req yspam_req;
	struct yspam_proto_ret yspam_ret;
	unsigned int high, low;

	bzero(&yspam_req, sizeof (struct yspam_proto_req));
	yspam_req.key = htonl(YSPAM_KEY);
	yspam_req.type = YSPAM_GET_SPAM;
	if (strlen(user) > IDLEN)
		return -YSPAM_ERR_HEADER;
	strcpy(yspam_req.userid, user);
	high = mailid >> 32;
	low = mailid & 0xFFFFFFFF;
	yspam_req.param.spam.mailid_high=htonl(high);
	yspam_req.param.spam.mailid_low=htonl(low);
	yspam_req.param.spam.mail_magic = htonl(magic);
	ret = yspam_proto(ctx, &yspam_req, &yspam_ret);
	if (ret < 0)
		return ret;
	strcpy(title, yspam_ret.param.mail.title);
	strcpy(sender, yspam_ret.param.mail.sender);
	return 0;
}

int
yspam_update_filename(struct yspam_ctx *ctx, unsigned long long mailid, char *filename)
{
	int ret;
	struct yspam_proto_req yspam_req;
	struct yspam_proto_ret yspam_ret;
	unsigned int high, low;

	bzero(&yspam_req, sizeof (struct yspam_proto_req));
	yspam_req.key = htonl(YSPAM_KEY);
	yspam_req.type = YSPAM_UPDATE_FILENAME;
	if (strlen(filename) > 19)
		return -YSPAM_ERR_HEADER;
	strcpy(yspam_req.param.updatemail.filename, filename);
	high = mailid >> 32;
	low = mailid & 0xFFFFFFFF;
	yspam_req.param.updatemail.mailid_high=htonl(high);
	yspam_req.param.updatemail.mailid_low=htonl(low);
	ret = yspam_proto(ctx, &yspam_req, &yspam_ret);
	if (ret < 0)
		return ret;
	return 0;
}
