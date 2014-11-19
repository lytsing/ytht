#ifndef __BBSTELNET_H
#define __BBSTELNET_H
typedef void  (*generate_attach_link_t)(char* ,int,long ,char *,void* );
extern int canProcessSIGALRM;
extern int occurredSIGALRM;
extern int canProcessSIGUSR2;
extern int occurredSIGUSR2;
extern void (*handlerSIGALRM) (int);
extern char clearbuf[];
extern char cleolbuf[];
extern char save_title[STRLEN];
extern char currboard[STRLEN];
extern time_t currboardstarttime;
extern char currdirect[STRLEN];
extern int currfiletime;
extern char currmaildir[STRLEN];
extern char endstandout[];
extern char fromhost[30];
extern char realfromhost[30];
extern unsigned int realfromIP;
extern char lookgrp[];
extern char page_requestor[];
extern char quote_file[], quote_user[];
extern char ReadPost[];
extern char ReplyPost[];
extern int automargins;
extern int can_R_endline;
extern int clearbuflen;
extern int cleolbuflen;
extern int convcode;
extern int digestmode;
extern int editansi;
extern int effectiveline;
extern int enabledbchar;
extern int endlineoffset;
extern int endstandoutlen;
extern int ERROR_READ_SYSTEM_FILE;
extern int friendflag;
extern int inprison;
extern int isattached;
extern int iscolor;
extern int local_article;
extern int msg_num;
extern int have_msg_unread;
extern int nettyNN;
extern int numboards;
extern int numf, friendmode;
extern int numofsig;
extern int page, range;
extern int pty;
extern int range;
extern int refscreen;
extern int RMSG;
extern int showansi;
extern int started;
extern int strtstandoutlen;
extern int talkidletime;
extern int talkrequest;
extern int t_columns;
extern int t_lines;
extern int toggle1, toggle2;
extern int WishNum;
extern int mot;
extern struct onebrc brc;
extern struct one_key friend_list[];
extern struct one_key mail_comms[];
extern struct one_key read_comms[];
extern struct one_key reject_list[];
extern struct one_key backnumber_comms[];
extern struct one_key selectbacknumber_comms[];
extern struct postheader header;
extern struct user_info **user_record;
extern time_t login_start_time;
extern time_t now_t, last_utmp_update;
extern int cur_ln;
extern int scr_cols;
extern int disable_move;
extern char IScurrBM;
extern char ISdelrq;
extern int readingthread;
extern int inBBSNET;
extern unsigned char board_sorttype;
extern struct userec *currentuser;	/*  user structure is loaded from passwd */
				  /*  file at logon, and remains for the   */
				  /*  entire session */
extern struct mmapfile passwd_mf;
extern struct userec *passwd_ptr;
extern struct bbsinfo bbsinfo;
extern char temp_sessionid[];

#define ZMODEM_RATE 5000
#define LINELEN 256
#define MAXT_LINES 100

#define DOECHO (1)		/* Flags to getdata input function */
#define NOECHO (0)
#define QUIT 0x666		/* Return value to abort recursive functions */
#define DONOTHING       0	/* Read menu command return states */
#define FULLUPDATE      1	/* Entire screen was destroyed in this oper */
#define PARTUPDATE      2	/* Only the top three lines were destroyed */
#define DOQUIT          3	/* Exit read menu was executed */
#define NEWDIRECT       4	/* Directory has changed, re-read files */
#define READ_NEXT       5	/* Direct read next file */
#define READ_PREV       6	/* Direct read prev file */
#define GOTO_NEXT       7	/* Move cursor to next */
#define DIRCHANGED      8	/* Index file was changed */
#define UPDATETLINE	9	/* t_lines was changed */

#define I_TIMEOUT   (-2)	/* Used for the getchar routine select call */
#define I_OTHERDATA (-333)	/* interface, (-3) will conflict with chinese */

#define SCREEN_SIZE (23)	/* Used by read menu  */

extern struct user_info uinfo; /* Ditto above...utmp entry is stored here
				   and written back to the utmp file when
				   necessary (pretty darn often). */
extern int usernum;		/* Index into passwds file user record */
extern int utmpent;		/* Index into this users utmp file entry */
extern int count_friends, count_users;	/*Add by SmallPig for count users and friends */

extern int t_lines, t_realcols, t_columns;	/* Screen size / width */

extern int nettyNN;
extern char currboard[];	/* name of currently selected board */

extern int selboard;		/* THis flag is true if above is active */

extern char genbuf[1024];	/* generally used global buffer */

extern jmp_buf byebye;		/* Used for exception condition like I/O error */

extern int in_mail;
extern int showansi;

extern sigjmp_buf bus_jump;
/*SREAD Define*/
#define SR_BMBASE       (10)
#define SR_BMDEL	(11)
#define SR_BMMARK       (12)
#define SR_BMDIGEST     (13)
#define SR_BMIMPORT     (14)
#define SR_BMTMP        (15)
#define SR_BMNOREPLY    (16)
#define SR_BMTMPDEL		(17)
/*SREAD Define*/

#ifndef EXTEND_KEY
#define EXTEND_KEY
#define KEY_TAB         9
#define KEY_ESC         27
#define KEY_UP          0x0101
#define KEY_DOWN        0x0102
#define KEY_RIGHT       0x0103
#define KEY_LEFT        0x0104
#define KEY_HOME        0x0201
#define KEY_INS         0x0202
#define KEY_DEL         0x0203
#define KEY_END         0x0204
#define KEY_PGUP        0x0205
#define KEY_PGDN        0x0206
#endif

#define Ctrl(c)         ( c & 037 )

/* =============== ANSI EDIT ================== */
#define   ANSI_RESET    "\033[0m"
#define   ANSI_REVERSE  "\033[7m\033[4m"
extern int editansi;
extern int KEY_ESC_arg;
/* ============================================ */

/* pty exec */
#ifdef CAN_EXEC
#if defined(CONF_HAVE_OPENPTY)
#include <pty.h>
#endif
#include "tmachine.h"
//#include <utmp.h>

extern int tmachine_init(int net);

extern queue_tl qneti, qneto;
extern int term_cols, term_lines;
extern char term_type[64];
extern int term_convert;
#endif

#define NUMBUFFER 20

#ifdef SSHBBS
int ssh_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int ssh_write(int fd, const void *buf, size_t count);
int ssh_read(int fd, void *buf, size_t count);
#endif

void prints(char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
struct keeploc {
	char *key;
	int top_line;
	int crs_line;
	struct keeploc *next;
};

#include "select.h"
#endif //__BBSTELNET_H
