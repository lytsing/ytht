/* boardrc.c */
#ifndef __BOADRC_H
#define __BOADRC_H
#define BRC_MAXSIZE     28672
#define BRC_MAXSIZE_GUEST 10240
#define BRC_MAXNUM      60
#define BRC_STRLEN      15
struct onebrc {
	short num;
	short cur;
	short changed;
	char board[BRC_STRLEN];
	int list[BRC_MAXNUM];
	int notetime;
};

struct onebrc_c {
	short len;
	char data[280];
} __attribute__ ((__packed__));

//brcinfo 是用来在brc文件中存放一些杂项信息的, 比如用户上次所点击的分类讨论区
//brcinfo将附加在onebrc_c的data段中, 前面附加"#INFO"字符串, 所以这个结构体要小
//于sizeof(onebrc_c->data)-6
struct brcinfo {
	unsigned char mybrdmode:1;	//WWW预定看版是否用英文显示, 0 中文; 1 中文
	unsigned char linkmode:1;	//是否不识别连接, 0 识别; 1 不识别
	unsigned char defmode:1;	//WWW版面文章列表方式, 0 普通; 1 主题
	unsigned char nouse:5;
	unsigned char wwwstyle;
	unsigned char t_lines;
	char lastsec[4];		//上次WWW选择的大区
	char myclass[24];
	char myclasstitle[13];		//如果为""则显示为版面标题
} __attribute__ ((__packed__));

struct allbrc {
	short size;
	short changed;
	short maxsize;
	char brc_c[BRC_MAXSIZE];
};

void brc_init(struct allbrc *allbrc, char *userid, char *filename);
void brc_fini(struct allbrc *allbrc, char *userid);
void brc_getboard(struct allbrc *allbrc, struct onebrc *brc, char *board);
void brc_putboard(struct allbrc *allbrc, struct onebrc *brc);
void brc_addlistt(struct onebrc *brc, int t);
int brc_unreadt(struct onebrc *brc, int t);
int brc_unreadt_quick(struct allbrc *allbrc, char *board, int t);
void brc_clearto(struct onebrc *brc, int t);
void brc_init_old(struct allbrc *allbrc, char *filename);
void brc_getinfo(struct allbrc *allbrc, struct brcinfo *info);
void brc_putinfo(struct allbrc *allbrc, struct brcinfo *info);
#define UNREAD(x, y) (((x)->edittime)?(brc_unreadt(y, (x)->edittime)):brc_unreadt(y, (x)->filetime))
#define SETREAD(x, y) (((x)->edittime)?(brc_addlistt(y, (x)->edittime)):brc_addlistt(y, (x)->filetime))
#endif
