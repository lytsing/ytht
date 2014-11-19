/* user.c */
#ifndef __USER_H
#define __USER_H

struct userec {			/* Structure used to hold information in */
	char userid[IDLEN + 2];	/* PASSFILE */
	char flags[2];
	time_t firstlogin;
	time_t lastlogin;
	time_t lastlogout;
	unsigned char dieday:3, inprison:1, mypic:1, hasblog:1, nouse1:2;
	unsigned char extralife5;
	char nouse[2];
	unsigned long int lasthost;
	char username[NAMELEN];
	unsigned short numdays;	//曾经登录的天数
	short mailsize;         //in kilobytes
	unsigned int numlogins;
	unsigned int numposts;
	time_t stay;
	unsigned userlevel;
	unsigned long int ip;
	unsigned int userdefine;
	char passwd[MD5LEN];	// MD5PASSLEN = 16
	int salt;		//salt == 0 means des; salt!=0 means md5
	time_t kickout;
};

/* these are flags in userec.flags[0] */
#define PAGER_FLAG   0x1	/* true if pager was OFF last session */
#define CLOAK_FLAG   0x2	/* true if cloak was ON last session */
#define NOUSE_FLAG   0x8	
#define BRDSORT_FLAG2 0x10	/* true if the boards sorted by score */
//#define BRDSORT_FLAG 0x20  /* true if the boards sorted alphabetical, */
			   /* available only if FLAG2 is false */
#define BRDSORT_MASK 0x30
#define CURSOR_FLAG  0x80	/* true if the cursor mode open */
#define ACTIVE_BOARD 0x200	/* true if user toggled active movie board on */

struct userdata {
	char realmail[60];
	char email[60];
	char realname[40];
	char address[40];
	char unused[56];
};

struct old_userec {		/* Structure used to hold information in */
	char userid[IDLEN + 2];	/* PASSFILE */
	time_t firstlogin;
	char lasthost[16];
	unsigned int numlogins;
	unsigned int numposts;
	char flags[2];
	char passwd[OLDPASSLEN];
	char username[NAMELEN];
	unsigned short numdays;	//曾经登录的天数
	char unuse[30];
	time_t dietime;
	time_t lastlogout;
	char ip[16];
	char realmail[STRLEN - 16];
	unsigned userlevel;
	time_t lastlogin;
	time_t stay;
	char realname[NAMELEN];
	char address[STRLEN];
	char email[STRLEN];
	int signature;
	unsigned int userdefine;
	time_t notedate_nouse;
	int noteline_nouse;
	int notemode_nouse;
};

struct override {
	char id[IDLEN + 1];
	char exp[40];
};

/* mytoupper: 将中文ID映射到A-Z的目录中 */
static inline char
mytoupper(unsigned char ch)
{
	if (isalpha(ch))
		return toupper(ch);
	else
		return ch % ('Z' - 'A') + 'A';
}

char *sethomepath(char *buf, const char *userid);
char *sethomefile(char *buf, const char *userid, const char *filename);
char *setmailfile(char *buf, const char *userid, const char *filename);
int saveuservalue(char *userid, char *key, char *value);
int readuservalue(char *userid, char *key, char *value, int size);
char *cuserexp(int);
char *cperf(int);
int countexp(struct userec *);
int countperf(struct userec *);
int countlife(struct userec *);
int userlock(char *userid, int locktype);
int userunlock(char *userid, int fd);
int checkbansite(const char *addr);
int userbansite(const char *userid, const char *fromhost);
void logattempt(const char *user, char *from, char *zone, time_t time);
int inoverride(char *who, char *owner, char *file);
int saveuserdata(char *uid, struct userdata *udata);
int loaduserdata(char *uid, struct userdata *udata);
int insertuserec(const struct userec *urec);
int getuser(const char *userid, struct userec **urec);
int user_registered(const char *userid);
int deluserec(const char *userid);
int kickoutuserec(const char *userid);
int updateuserec(const struct userec *x, const int usernum);
int getuserbynum(const int uid, struct userec **urec);
int apply_passwd(int (*fptr) (const struct userec *, char *), char *arg);
int lock_passwd();
int unlock_passwd(int fd);
int has_fill_form(char *userid);
extern struct userec *passwdptr;
#endif
