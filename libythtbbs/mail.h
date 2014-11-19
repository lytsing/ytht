/* mail.c */
#ifndef __MAIL_H
#define __MAIL_H
#define MAX_MAIL_HOLD (100)
#define MAX_SYSOPMAIL_HOLD (300)
int update_mailsize_up(struct fileheader *, char *userid);
int update_mailsize_down(struct fileheader *, char *userid);
int max_mailsize(struct userec *lookupuser);
int DIR_do_editmail(struct fileheader *fileinfo, struct fileheader *newfileinfo, char *userid);
int get_mailsize(struct userec *lookupuser);
int mail_buf(char *buf, int size, char *userid, char *title, char *sender); //this api check mailsize
int mail_file(char *filename, char *userid, char *title, char *sender); //this api check mailsize
int system_mail_buf(char *buf, int size, char *userid, char *title, char *sender); //this api don't check mailsize
int system_mail_file(char *filename, char *userid, char *title, char *sender); //this api don't check mailsize
int system_mail_link(char *filename, char *userid, char *title, char *sender); //don't check mailsize. symbolic link
int calc_mailsize(struct userec *user, int needlock);
char * check_mailperm(struct userec *lookupuser);
#ifdef INTERNET_EMAIL
int bbs_sendmail(const char *fname, const char *title, const char *receiver, const char *sender, int filter_ansi);
int bbs_sendmail_noansi(char *fname, char *title, char *receiver, char *sender);
#endif
int invalidaddr(const char *addr);
#endif
