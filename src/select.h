#ifndef _SELECT_H
#define _SELECT_H
#define SHOW_QUIT 0             /*�����˳� */
#define SHOW_DIRCHANGE 1        /*��Ҫ���»�ȡ����ˢ���б� */
#define SHOW_REFRESH 2          /* ֻ�ػ��б� */
#define SHOW_REFRESHSELECT 3    /* ֻ�ػ�ѡ�����һ�� */
#define SHOW_SELCHANGE 4        /* ֻ�ı���ѡ�� */
#define SHOW_SELECT 5           /* ѡ�����б�֮�󷵻� */
#define SHOW_CONTINUE 6         /*����ѭ�� */

#define	LF_MULTIPAGE 	0x1     /*��ҳ */
#define	LF_HILIGHTSEL	0x2     /*ѡ����б�ɫ */
#define	LF_VSCROLL	0x4     /*ÿ��itemռһ�� */
#define	LF_NEEDFORCECLEAN 0x8
#define LF_FORCEREFRESHSEL 0x10
    /*
     * ���ÿ��item���ȳ�����Ҫǿ���ÿո���� 
     */
#define	LF_BELL			0x10    /*�����ʱ������ */
#define	LF_LOOP		0x20    /*ѭ���ģ��������һ��������ǰһ������ǰһ����ǰ�����һ�� */
#define	LF_NUMSEL		0x40    /*������ѡ��*/

#define  LF_ACTIVE	0x10000 /*�б������־ */
#define  LF_INITED	0x20000 /*�б��Ѿ���ʼ����� */

/*��չ�Ĺ��ܼ�����*/
#define KEY_REFRESH		0x1000  /* �ػ�*/
#define KEY_ACTIVE 		0x1001  /* ���no use now*/
#define KEY_DEACTIVE 	0x1002  /* �ۻ���no use now*/
#define KEY_SELECT		0x1003  /* ѡ���¼�*/
#define KEY_INIT		       0x1004  /*��ʼ��*/
#define KEY_TALK		0x1005  /*TALK  REQUEST*/
#define KEY_TIMEOUT		0x1006  /*��ʱ*/
#define KEY_ONSIZE          0x1007  /*���ڴ�С�ı�*/
#define KEY_DIRCHANGE	    0x1008  /*Ŀ¼���ݸı�*/
#define KEY_SELCHANGE       0x1009  /*ѡ��ı�*/
#define KEY_INVALID         0xFFFF

#define KEY_BUF_LEN         4 /*һ���ڲ���key���г���*/

typedef struct tagPOINT {
    int x, y;
} POINT;


struct key_translate {

    int ch;
    int command;
};
struct _select_def {
    int flag;                   /*��� */
    int item_count;             /*��item���� */
    int item_per_page;          /*һҳ�ܹ��м���item��һ��Ӧ�õ���list_linenum */
    POINT *item_pos;            /*һҳ����ÿ��item��λ�ã����Ϊ�գ���һ��һ������ */
    POINT title_pos;            /*����λ�� */
    POINT endline_pos;          /*ҳĩλ�� */
    void *arg;                  /*�������ݵĲ��� */
/*	int page_num;  ��ǰҳ�� */
    int pos;                    /* ��ǰѡ���itemλ�� */
    int page_pos;               /*��ǰҳ�ĵ�һ��item��� */
    char *prompt;               /*ѡ���itemǰ�����ʾ�ַ� */
    POINT cursor_pos;           /*���λ�� */
    int new_pos;                /*������SHOW_SELECTCHANGE ��ʱ�򣬰��µ�λ�÷������� */
    struct key_translate *key_table;    /*������� */
    /*
     * �ڲ�ʹ�õı��� 
     */
    int tmpnum; /*���ڶ�����LF_NUMSEL�����ֱ���*/
    int keybuf[KEY_BUF_LEN]; /*����һ��KEY_BUF_LEN��key�����У�
                                              ������������������key����*/
    int keybuflen;
    
    int (*init) (struct _select_def * conf);    /*��ʼ�� */
    int (*page_init) (struct _select_def * conf);       /*��ҳ��ʼ������ʱposλ���Ѿ����ı��� */
    int (*get_data) (struct _select_def * conf, int pos, int len);      /*���posλ��,����Ϊlen������ */
    int (*show_data) (struct _select_def * conf, int pos);      /*��ʾposλ�õ����ݡ� */
    int (*show_title) (struct _select_def * conf);      /*��ʾ���⡣ */
    int (*show_endline) (struct _select_def * conf);    /*��ʾ��ĩ�� */
    int (*pre_key_command) (struct _select_def * conf, int* command);        /*���������ұ�����ǰ����������� */
    int (*key_command) (struct _select_def * conf, int command);        /*����������� */
    void (*quit) (struct _select_def * conf);    /*���� */
    int (*on_selchange) (struct _select_def * conf, int new_pos);       /*�ı�ѡ���ʱ��Ļص����� */
    int (*on_size)(struct _select_def* conf);   /*term���ڴ�С�ı�*/

    /*ѡ����ĳһ��*/
    int (*on_select) (struct _select_def * conf);       
    
    int (*active) (struct _select_def * conf);  /*�����б� */
    int (*deactive) (struct _select_def * conf);        /*�б�ʧȥ���� */
};
int list_select(struct _select_def *conf, int key);
int list_select_loop(struct _select_def *conf);
int list_select_has_key(struct _select_def* conf);
int list_select_add_key(struct _select_def* conf,int key); /* ����һ���������뻺����*/
int list_select_remove_key(struct _select_def* conf); /*�����뻺��������ȡ��һ����*/
struct _select_def* select_get_current_conf(); /*��õ�ǰ��conf*/

/* �򵥵�ѡ���*/
#define SIF_SINGLE 0x1
#define SIF_NUMBERKEY	0x100  /* ����ѡ��0-9*/
#define SIF_ALPHAKEY	0x200  /*��ĸѡ��a-z*/
#define SIF_ESCQUIT	0x400  /* ESC�˳� */
#define SIF_RIGHTSEL 0x800 /* �Ҽ�ѡ��*/


#define SIT_SELECT	0x1
#define SIT_EDIT	0x2
#define SIT_CHECK	0x3

struct _select_item {
	int x,y;
	int hotkey;
	int type;
	void* data;
};

union _select_return_value {
	int selected;
	char* returnstr;
};

int simple_select_loop(const struct _select_item* item_conf,int flag,int titlex,int titley,union _select_return_value* ret);
#endif

