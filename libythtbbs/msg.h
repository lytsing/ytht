/* msg.h */
#ifndef __MSG_H
#define __MSG_H
struct msghead {
	int pos, len;
	char sent;
	char mode;
	char id[IDLEN + 2];
	time_t time;
	int frompid, topid;
};
#define MAX_MSG_SIZE 1024  //�����Ϣ����

int save_msgtext(char *uident, struct msghead *head, const char *msgbuf);
int translate_msg(char *src, struct msghead *head, char *dest, int add_site);
int get_unreadcount(char *uident);
int get_msgcount(int id, char *uident);
int load_msghead(int id, char *uident, struct msghead *head, int index);
int load_msgtext(char *uident, struct msghead *head, char *msgbuf);
int get_unreadmsg(char *uident);
int clear_msg(char *uident);
#endif
