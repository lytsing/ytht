#ifndef __YSPAM_H
#define __YSPAM_H
#define YSPAM_PORT  1712 
#define YSPAM_KEY   0x91871712
#define YSPAM_TITLE_LEN 80
#define YSPAM_SENDER_LEN 50

struct mailid_param
{
	uint32_t mailid_high;
	uint32_t mailid_low;
	int32_t mail_magic;
};

struct filename_param
{
	char filename[20]; 
}; //32 char

struct mailinfo_param
{
	uint8_t type;
	char padding[3];
	char title[YSPAM_TITLE_LEN];
	char sender[YSPAM_SENDER_LEN];
};

struct mailupdate_param
{
	uint32_t mailid_high;
	uint32_t mailid_low;
	char filename[20];
};

struct spamheader
{
	char title[YSPAM_TITLE_LEN];
	uint32_t mailid_high;
	uint32_t mailid_low;
	uint32_t time;
	int32_t magic;
	char sender[YSPAM_SENDER_LEN];
};

struct yspam_proto_req {
	uint32_t key;		//magic key
	char userid[IDLEN+1];
	uint8_t type;		//data type, spam or ham
	uint16_t unused2;
	union yspam_req_param {
		struct mailid_param spam;
		struct filename_param ham;
		struct mailinfo_param newmail;
		struct mailupdate_param updatemail;
	} param;
} __attribute__ ((packed));

struct yspam_proto_ret {
	uint32_t key;		//magic key
	int8_t status;
	uint8_t unused[3];
	union yspam_ret_param {
		struct mailid_param mailid;
		struct mailinfo_param mail;
		uint32_t spamnum;
	} param;
} __attribute__ ((packed));

#define YSPAM_ERR_BROKEN  1	//link is broken
#define YSPAM_ERR_KEY     2	//error key
#define YSPAM_ERR_OPT     3	//no such operation
#define YSPAM_ERR_NOSUCHMAIL     4	//error occor when processing image
#define YSPAM_ERR_SQL  5	//can not connect
#define YSPAM_ERR_HEADER 6
#define YSPAM_ERR_CONNECT 7

#define YSPAM_FEED_SPAM 1
#define YSPAM_FEED_HAM 2
#define YSPAM_NEWMAIL 3
#define YSPAM_GETALLSPAM 4
#define YSPAM_GET_SPAM 5
#define YSPAM_UPDATE_FILENAME 6

#define YSPAM_SPAM 1
#define YSPAM_HAM 2

struct yspam_ctx {
	struct sockaddr_in host;
} __attribute__ ((packed));

struct yspam_ctx * yspam_init(char *host);
void yspam_fini(struct yspam_ctx *ctx);

int yspam_newmail(struct yspam_ctx *ctx, char *user, char *title, uint8_t type, char *sender, unsigned long long *mailid);
int yspam_feed_spam(struct yspam_ctx *ctx, char *user, char *filename);
int yspam_feed_ham(struct yspam_ctx *ctx, char *user, unsigned long long mailid, int magic);
int yspam_getallspam(struct yspam_ctx *ctx, char *user, struct spamheader **sh, uint16_t *spamnum);
int yspam_getspam(struct yspam_ctx *ctx, char *user, unsigned long long mailid, int magic, char *, char *);
int yspam_update_filename(struct yspam_ctx *ctx, unsigned long long mailid, char *filename);
#endif
