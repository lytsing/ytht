#ifndef __UTMP_H
#define __UTMP_H
#define CLUB_SIZE      4	/* 4 * sizeof(int) 为完全 close club数目上限 */
#define MAXFRIENDS (200)
#define MAXREJECTS (32)

struct wwwsession {
	unsigned char edit_mode:1, mybrd_mode:1, att_mode:1, ipmask:4, doc_mode:1;
	unsigned char link_mode:1, def_mode:1, t_lines:6;
	char iskicked;
	char unused;
	time_t login_start_time;
	time_t lastposttime;
	time_t lastinboardtime;
};

struct user_info {		/* Structure used in UTMP file */
	int active;		/* When allocated this field is true */
	int uid;		/* Used to find user name in passwd file */
	int pid;		/* kill() to notify user of talk request */
	int invisible;		/* Used by cloaking function in Xyz menu */
	int sockactive;		/* Used to coordinate talk requests */
	int sockaddr;		/* ... */
	int destuid;		/* talk uses this to identify who called */
	int mode;		/* UL/DL, Talk Mode, Chat Mode, ... */
	int pager;		/* pager toggle, YEA, or NA */
	int in_chat;		/* for in_chat commands   */
	int fnum;		/* number of friends */
	short ext_idle;		/* has extended idle time, YEA or NA */
	short isssh;		/* login from ssh */
	time_t lasttime;	/* time of the last action */
	unsigned int userlevel;	//change by lepton for www
	char chatid[10];	/* chat id, if in chat mode */
	char from[20];		/* machine name the user called in from */
				/* mask might be applied on it */
	char sessionid[40];	/* add by leptin for www use */
	char userid[IDLEN + 1];
	char nouse1;
	short signature;
	char username[NAMELEN];
	unsigned int unreadmsg;
	//time_t        lastposttime;
	short curboard;
	//time_t  lastinboardtime;
	int clubrights[CLUB_SIZE];
	unsigned long fromIP; 	//The IP address from where the user connected
	time_t nouse4;	//Added by ylsdd for www use
	char nouse2[12];
	unsigned friend[MAXFRIENDS];
	unsigned reject[MAXREJECTS];
	struct wwwsession wwwinfo;
	struct onebrc brc;
};
int isreject(struct user_info *uentp, struct user_info *reader);
unsigned int utmp_iphash(char *fromhost);
#endif
