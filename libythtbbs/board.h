/* board.c */
#ifndef __BOARD_H
#define __BOARD_H

#define BOARDAUX ".BOARDAUX"

#define BMNUM 16
struct boardheader {		/* This structure is used to hold data in */
	char filename[24];	/* the BOARDS files */
	char title[24];
	int clubnum;
	unsigned level;
	char flag;
	char secnumber1;
	char secnumber2;
	char type[5];
	char bm[BMNUM][IDLEN + 1];
	int hiretime[BMNUM];
	int board_ctime;
	int board_mtime;
	char sec1[4];
	char sec2[4];
	unsigned char limitchar;	//upper limit/100
	char flag2;
	char nouse[2];
	char unused[156];
};

struct boardmem {		/* used for caching files and boards */
	struct boardheader header;
	int lastpost;
	int total;
	short inboard;
	short bmonline;
	short bmcloak;
	int stocknum;
	int score;
	time_t wwwindext;
	int unused[8];
	char unused1[2];
	unsigned char unused3:4, hasnotes:1, wwwbkn:1, wwwicon:1, wwwindex:1;
	char ban;
};

struct hottopic {
	int thread;
	int num;		//讨论人数
	char title[60];
};

struct lastmark {
	int thread;
	char marked;
	char nouse[3];
	char title[60];
	char authors[60];	//可能有多作者
};

struct relateboard {
	char filename[24];
	char title[24];
};

#define MAXHOTTOPIC 5
#define MAXLASTMARK 9
#define MAXTOPFILE 5
#define MAXRELATE 6

struct boardaux {		/* used to save lastmark/hottopic/置底 */
	int nhottopic;
	struct hottopic hottopic[MAXHOTTOPIC];
	int nlastmark;
	struct lastmark lastmark[MAXLASTMARK];
	int ntopfile;
	struct fileheader topfile[MAXTOPFILE];
	int nousea[5];
	int nrelate;
	struct relateboard relate[MAXRELATE];
	char intro[200];
	char nouse[1440];	//total 4k
};

struct boardmanager {		/* record in user directionary */
	char board[24];
	char bmpos;
	char unused;
	short bid;
};

struct myparam1 {		/* just use to pass a param to fillmboard() */
	struct userec user;
	int fd;
	short bid;
};

struct bm_eva {	// for BM evaluation weekly
	char userid[IDLEN + 1];
	char week, total_week;
	char ave_score;
	short weight;
	char leave;
	char last_pass;
	int nouse;
};

struct board_bmstat {
        char boardname[24];
        struct bm_eva bm[BMNUM];
        int boardscore;
        char sec;
        char unused[7];
};

char *bm2str(char *buf, struct boardheader *bh);
char *sbm2str(char *buf, struct boardheader *bh);
int chk_BM(struct userec *, struct boardheader *bh, int isbig);
int chk_BM_id(char *, struct boardheader *);
int dosetbmstatus(struct boardmem *bcache, char *userid, int online, int visible);
int bmfilesync(struct userec *);
int fillmboard(struct boardheader *bh, struct myparam1 *param);
int getlastpost(char *board, int *lastpost, int *total);
struct boardaux *getboardaux(int bnum);
int addtopfile(int bnum, struct fileheader *fh);
int deltopfile(int bnum, int num);
int updateintro(int bnum, char *filename);
#endif
