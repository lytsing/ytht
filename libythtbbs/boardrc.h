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

//brcinfo ��������brc�ļ��д��һЩ������Ϣ��, �����û��ϴ�������ķ���������
//brcinfo��������onebrc_c��data����, ǰ�渽��"#INFO"�ַ���, ��������ṹ��ҪС
//��sizeof(onebrc_c->data)-6
struct brcinfo {
	unsigned char mybrdmode:1;	//WWWԤ�������Ƿ���Ӣ����ʾ, 0 ����; 1 ����
	unsigned char linkmode:1;	//�Ƿ�ʶ������, 0 ʶ��; 1 ��ʶ��
	unsigned char defmode:1;	//WWW���������б�ʽ, 0 ��ͨ; 1 ����
	unsigned char nouse:5;
	unsigned char wwwstyle;
	unsigned char t_lines;
	char lastsec[4];		//�ϴ�WWWѡ��Ĵ���
	char myclass[24];
	char myclasstitle[13];		//���Ϊ""����ʾΪ�������
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
