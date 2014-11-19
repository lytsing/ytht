/* login.c */
#include "../include/bbs.h"
extern int isanonymous;
int yftpd_login(char *password);
void login_end(void);
int getusernum(char *id);
int checkuser(char *id, char *pw);
char mytoupper(unsigned char ch);
char *setuserfile(char *buf, char *filename);
void readclubrights(void);
int hasreadperm(struct boardmem *bptr);
int seek_in_file(const char *filename, const char *seekstr);
