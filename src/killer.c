/******************************************************
ɱ����Ϸ2003, ����: bad@smth.org  Qian Wenjie
��ˮľ�廪bbsϵͳ������

����Ϸ����������������⸴����ֲ
�����޸ĺ���ļ�ͷ��������Ȩ��Ϣ
******************************************************/
/******************************************************
 * ɱ����Ϸ 2003 for FB 2000 ����: TurboZV@UESTC Zhang Wei
 *
 * ���Ѿ���smth��ɱ����Ϸ��ֲ��FB2000ϵͳ�ϣ�
 * ллbad@smthΪ����Ϸ�ĸ���
 * 
 *����Ϸ����������������⸴����ֲ
 *�����޸ĺ���ļ�ͷ��������Ȩ��Ϣ
 *				    2003-5-12
******************************************************/
/* ChangeLog 030515
 *  1) ��ȷ�Ĵ�������ɫ����ʾ
 *  2) ������鹦��(*)
 *  3) ͶƱ���»��߱�ʾ(*)
 *  4) ����һ�����а���(*)
 *  5) �Ծ���Ĺ�������һЩ����(*)
 *  6) ����ֻ����Ctrl+T��̽���� (*)
 * ��*Ϊbad@smth�������޸�
 * */

#define BBSMAIN
#include "select.h"
#include "bbs.h"
#include "bbstelnet.h"

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define PINK 5
#define CYAN 6
#define WHITE 7

#define MAX_ROOM 30
#define MAX_PEOPLE 100
#define MAX_MSG 2000

#define BBS_PAGESIZE    (t_lines - 4)

int k_getdata_val;

int k_getdata(line, col, prompt, buf, len, echo, clearlabel)
int line, col, len, echo, clearlabel;
char *prompt, *buf;
{
	int retv;
    buf[0] = '\0';
    k_getdata_val = 0;
    retv = getdata(line, col, prompt, buf, len, echo, clearlabel);
    if(k_getdata_val)
	    return -k_getdata_val;
    return retv;
}

void k_setfcolor(int i, int j)
{
}

void k_resetcolor()
{
}

int strlen2(const char *s)
{
        int p;
        int inansi;
        p=0;
        inansi=0;
        while (*s) {
                if (*s=='\x1b')
                        inansi=1;
                else if (inansi && *s=='m')
                        inansi=0;
                else if (!inansi)
                        p++;
                s++;
        }
        return p;
}


#define ROOM_LOCKED 01
#define ROOM_SECRET 02
#define ROOM_DENYSPEC 04

struct room_struct {
    int w;
    int style; /* 0 - chat room 1 - killer room */
    char name[14];
    char title[NAMELEN];
    char creator[IDLEN+2];
    unsigned int level;
    int flag;
    int people;
    int maxpeople;
};

#define PEOPLE_SPECTATOR 01
#define PEOPLE_KILLER 02
#define PEOPLE_ALIVE 04
#define PEOPLE_ROOMOP 010
#define PEOPLE_POLICE 020
#define PEOPLE_TESTED 040

struct people_struct {
    int style;
    char id[IDLEN+2];
    char nick[NAMELEN];
    int flag;
    int pid;
    int vote;
    int vnum;
};

#define INROOM_STOP 1
#define INROOM_NIGHT 2
#define INROOM_DAY 3

struct inroom_struct {
    int w;
    int status;
    int killernum;
    int policenum;
    struct people_struct peoples[MAX_PEOPLE];
    char msgs[MAX_MSG][60];
    int msgpid[MAX_MSG];
    int msgi;
};

struct room_struct * rooms;
struct inroom_struct * inrooms;

struct killer_record {
    int w; //0 - ƽ��ʤ�� 1 - ɱ��ʤ��
    time_t t;
    int peoplet;
    char id[MAX_PEOPLE][IDLEN+2];
    int st[MAX_PEOPLE]; // 0 - ����ƽ�� 1 - ����ƽ�� 2 - ����ɱ�� 3 - ����ɱ�� 4 - �������
};

int myroom, mypos;

int kicked;
/*
void save_result(int w)
{
    int fd;
    struct flock ldata;
    struct killer_record r;
    int i,j;
    char filename[80]="etc/.KILLERRESULT";
    r.t = time(0);
    r.w = w;
    r.peoplet = 0; j = 0;
    for(i=0;i<MAX_PEOPLE;i++)
    if(inrooms[myroom].peoples[i].style!=-1&&!(inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR)) {
        strcpy(r.id[j], inrooms[myroom].peoples[i].id);
        r.st[j] = 4;
        if(!(inrooms[myroom].peoples[i].flag&PEOPLE_KILLER)) {
            if(inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE)
                r.st[j] = 0;
            else
                r.st[j] = 1;
        } else {
            if(inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE)
                r.st[j] = 2;
            else
                r.st[j] = 3;
        }
        
        j++;
        r.peoplet++;
    }
    if((fd = open(filename, O_WRONLY|O_CREAT, 0644))!=-1) {
        ldata.l_type=F_WRLCK;
        ldata.l_whence=0;
        ldata.l_len=0;
        ldata.l_start=0;
        if(fcntl(fd, F_SETLKW, &ldata)!=-1){
            lseek(fd, 0, SEEK_END);
            write(fd, &r, sizeof(struct killer_record));
            	
            ldata.l_type = F_UNLCK;
            fcntl(fd, F_SETLKW, &ldata);
        }
        close(fd);
    }
}

void load_msgs()
{
    FILE* fp;
    int i;
    char filename[80], buf[80];
    msgst=0;
    sprintf(filename, "home/%c/%s/.INROOMMSG%d", toupper(currentuser->userid[0]), currentuser->userid, uinfo.pid);
    fp = fopen(filename, "r");
    if(fp) {
        while(!feof(fp)) {
            if(fgets(buf, 79, fp)==NULL) break;
            if(buf[0]) {
                if(!strncmp(buf, "�㱻����", 8)) kicked=1;
                if(msgst==200) {
                    msgst--;
                    for(i=0;i<msgst;i++)
                        strcpy(msgs[i],msgs[i+1]);
                }
                strcpy(msgs[msgst],buf);
                msgst++;
            }
        }
        fclose(fp);
    }
}*/

void start_change_inroom()
{
    if(inrooms[myroom].w) sleep(0);
    inrooms[myroom].w = 1;
}

void end_change_inroom()
{
    inrooms[myroom].w = 0;
}

struct action {
    char *verb;                 /* ���� */
    char *part1_msg;            /* ��� */
    char *part2_msg;            /* ���� */
};

struct action party_data[] = {
    {"?", "���ɻ�Ŀ���", ""},
    {"admire", "��", "�ľ���֮���������Ͻ�ˮ���಻��"},
    {"agree", "��ȫͬ��", "�Ŀ���"},
    {"bearhug", "�����ӵ��", ""},
    {"bless", "ף��", "�����³�"},
    {"bow", "�Ϲ��Ͼ�����", "�Ϲ�"},
    {"bye", "����", "�ı�Ӱ����Ȼ���¡����������������������ĸ���:\n\"���վ�����...\""},
    {"caress", "����ĸ���", ""},
    {"cat", "��ֻСè���������", "�Ļ���������"},
    {"cringe", "��", "������ϥ��ҡβ����"},
    {"cry", "��", "�������"},
    {"comfort", "���԰�ο", ""},
    {"clap", "��", "���ҹ���"},
    {"dance", "����", "������������"},
    {"dogleg", "��", "����"},
    {"drivel", "����", "����ˮ"},
    {"dunno", "�ɴ��۾���������ʣ�", "����˵ʲ���Ҳ���Ү... :("},
    {"faint", "�ε���", "�Ļ���"},
    {"fear", "��", "¶�����µı���"},
    {"fool", "����ע��", "�����׳�....\n������������....�˼����Ļ....\n����̫��ϧ�ˣ�"},
    {"forgive", "��ȵĶ�", "˵�����ˣ�ԭ������"},
    {"giggle", "����", "ɵɵ�Ĵ�Ц"},
    {"grin", "��", "¶��а���Ц��"},
    {"grrr", "��", "��������"},
    {"hand", "��", "����"},
    {"hammer", "����ô�ô���������� �ۣ���",
     "ͷ������һ�ã�\n***************\n*  5000000 Pt *\n***************\n      | |      �������\n      | |         �ö������Ӵ\n      |_|"},
    {"heng", "��������", "һ�ۣ� ����һ�����߸ߵİ�ͷ��������,��мһ�˵�����..."},
    {"hug", "�����ӵ��", ""},
    {"idiot", "����س�Ц", "��������"},
    {"kick", "��", "�ߵ���ȥ����"},
    {"kiss", "����", "������"},
    {"laugh", "������Ц", ""},
    {"lovelook", "����", "���֣������ĬĬ���ӡ�Ŀ�����к���ǧ�����飬�������"},
    {"nod", "��", "��ͷ����"},
    {"nudge", "�����ⶥ", "�ķʶ���"},
    {"oh", "��", "˵����Ŷ�����Ӱ�����"},
    {"pad", "����", "�ļ��"},
    {"papaya", "������", "��ľ���Դ�"},
    {"pat", "��������", "��ͷ"},
    {"pinch", "�����İ�", "š�ĺ���"},
    {"puke", "����", "�°��°�����˵�¶༸�ξ�ϰ����"},
    {"punch", "�ݺ�����", "һ��"},
    {"pure", "��", "¶�������Ц��"},
    {"qmarry", "��", "�¸ҵĹ�������:\n\"��Ը��޸�����\"\n---���������ɼΰ�"},
    {"report", "͵͵�ض�", "˵���������Һ��𣿡�"},
    {"shrug", "���ε���", "�����ʼ��"},
    {"sigh", "��", "̾��һ����"},
    {"slap", "žž�İ���", "һ�ٶ���"},
    {"smooch", "ӵ����", ""},
    {"snicker", "�ٺٺ�..�Ķ�", "��Ц"},
    {"sniff", "��", "��֮�Ա�"},
    {"sorry", "ʹ�����������", "ԭ��"},
    {"spank", "�ð��ƴ�", "��ƨ��"},
    {"squeeze", "������ӵ����", ""},
    {"thank", "��", "��л"},
    {"tickle", "��ߴ!��ߴ!ɦ", "����"},
    {"waiting", "����ض�", "˵��ÿ��ÿ�µ�ÿһ�죬ÿ��ÿ���Ҷ������������"},
    {"wake", "Ŭ����ҡҡ", "��������ߴ�У��������ѣ��������ģ���"},
    {"wave", "����", "ƴ����ҡ��"},
    {"welcome", "���һ�ӭ", "�ĵ���"},
    {"wink", "��", "���ص�գգ�۾�"},
    {"xixi", "�����ض�", "Ц�˼���"},
    {"zap", "��", "���Ĺ���"},
    {"inn", "˫�۱�������ˮ���޹�������", ""},
    {"mm", "ɫ���еĶ�", "�ʺã�����ü�á�������"},
    {"disapp", "����û��ͷ����Ϊʲô", "����������������ȫû��Ӧ��û�취��"},
    {"miss", "��ϵ�����", "�����������������������̫--��������!���಻����?"},
    {"buypig", "ָ��", "���������ͷ������һ�룬лл����"},
    {"rascal", "��", "��У������������å����������������������������������"},
    {"qifu", "С��һ�⣬��", "�޵��������۸��ң����۸��ң�������"},
    {"wa", "��", "���һ�����������������ۿ����Ү������������������������������"},
    {"libel", "ร���������죬", "�������ҽ�������һ�����Ը���ٰ�������"},
    {"badman", "ָ��", "���Դ�, �����㵸�ĺ�:�� ɱ~~��~~��~~��~~��~~~��"}, 
    {NULL, NULL, NULL}
};

struct action speak_data[] = {
    {"ask", "ѯ��", NULL},
    {"chant", "����", NULL},
    {"cheer", "�Ȳ�", NULL},
    {"chuckle", "��Ц", NULL},
    {"curse", "����", NULL},
    {"demand", "Ҫ��", NULL},
    {"frown", "��ü", NULL},
    {"groan", "����", NULL},
    {"grumble", "����ɧ", NULL},
    {"hum", "������", NULL},
    {"moan", "��̾", NULL},
    {"notice", "ע��", NULL},
    {"order", "����", NULL},
    {"ponder", "��˼", NULL},
    {"pout", "������˵", NULL},
    {"pray", "��", NULL},
    {"request", "����", NULL},
    {"shout", "���", NULL},
    {"sing", "����", NULL},
    {"smile", "΢Ц", NULL},
    {"smirk", "��Ц", NULL},
    {"swear", "����", NULL},
    {"tease", "��Ц", NULL},
    {"whimper", "���ʵ�˵", NULL},
    {"yawn", "��Ƿ����", NULL},
    {"yell", "��", NULL},
    {NULL, NULL, NULL}
};

struct action condition_data[] = {
    {":D", "�ֵĺϲ�£��", NULL},
    {":)", "�ֵĺϲ�£��", NULL},
    {":P", "�ֵĺϲ�£��", NULL},
    {":(", "�ֵĺϲ�£��", NULL},
    {"applaud", "žžžžžžž....", NULL},
    {"blush", "��������", NULL},
    {"cough", "���˼���", NULL},
    {"faint", "�۵�һ�����ε��ڵ�", NULL},
    {"happy", "������¶�����Ҹ��ı��飬��ѧ�Ա��˵���ߺ�������", NULL},
    {"lonely", "һ�������ڷ��������������ϣ��˭�����㡣������", NULL},
    {"luck", "�ۣ���������", NULL},
    {"puke", "����ģ������˶�����", NULL},
    {"shake", "ҡ��ҡͷ", NULL},
    {"sleep", "Zzzzzzzzzz�������ģ�����˯����", NULL},
    {"so", "�ͽ���!!", NULL},
    {"strut", "��ҡ��ڵ���", NULL},
    {"tongue", "��������ͷ", NULL},
    {"think", "����ͷ����һ��", NULL},
    {"wawl", "���춯�صĿ�", NULL},
    {"goodman", "�ü����޹��ı��鿴�Ŵ��: ��������Ǻ���Ү~~��", NULL},
    {NULL, NULL, NULL}
};

void
send_msg(int u, char* msg)
{
    int i, j, k, f;
    char buf[200], buf2[200], buf3[80];

    strcpy(buf, msg);

    for(i=0;i<=6;i++) {
        buf3[0]='%'; buf3[1]=i+48; buf3[2]=0;
        while(strstr(buf, buf3)!=NULL) {
            strcpy(buf2, buf);
            k=strstr(buf, buf3)-buf;
            buf2[k]=0; k+=2;
            sprintf(buf, "%s\033[3%dm%s", buf2, (i>0)?i:7, buf2+k);
        }
    }
    
    while(strchr(buf, '\n')!=NULL) {
        i = strchr(buf, '\n')-buf;
        buf[i]=0;
        send_msg(u, buf);
        strcpy(buf2, buf+i+1);
        strcpy(buf, buf2);
    }
    while(strlen(buf)>56) {
        int maxi=0;
        k=0; j = 0; f = 0;
        for(i=0;i<strlen(buf);i++) {
            if(buf[i]==' ') f = 1;
            if(f) {
                if(isalpha(buf[i])) f=0;
                continue;
            }
            if(k==0&&i<=56) {
                if(i>maxi)
                    maxi = i;
            }
            j++;
            if(k) k=0;
            else if(buf[i]<0) k=1;
        }
        if(maxi<strlen(buf)&&maxi!=0) {
            strcpy(buf2, buf);
            buf[maxi]=0;
            send_msg(u, buf);
            strcpy(buf, buf2+maxi);
        }
        else break;
    }
    j=MAX_MSG;
    if(inrooms[myroom].msgs[(MAX_MSG-1+inrooms[myroom].msgi)%MAX_MSG][0]==0)
    for(i=0;i<MAX_MSG;i++)
        if(inrooms[myroom].msgs[(i+inrooms[myroom].msgi)%MAX_MSG][0]==0) {
            j=(i+inrooms[myroom].msgi)%MAX_MSG;
            break;
        }
    if(j==MAX_MSG) {
        strcpy(inrooms[myroom].msgs[inrooms[myroom].msgi], buf);
        if(u==-1)
            inrooms[myroom].msgpid[inrooms[myroom].msgi] = -1;
        else
            inrooms[myroom].msgpid[inrooms[myroom].msgi] = inrooms[myroom].peoples[u].pid;
        inrooms[myroom].msgi = (inrooms[myroom].msgi+1)%MAX_MSG;
    }
    else {
        strcpy(inrooms[myroom].msgs[j], buf);
        if(u==-1)
            inrooms[myroom].msgpid[j] = u;
        else
            inrooms[myroom].msgpid[j] = inrooms[myroom].peoples[u].pid;
    }
}

void kill_msg(int u)
{
    int i,j,k;
    char buf[80];
    for(i=0;i<MAX_PEOPLE;i++)
    if(inrooms[myroom].peoples[i].style!=-1)
    if(u==-1||i==u) {
        j=kill(inrooms[myroom].peoples[i].pid, SIGUSR1);
        if(j==-1) {
            sprintf(buf, "%s������", inrooms[myroom].peoples[i].nick);
            send_msg(-1, buf);
            start_change_inroom();
            inrooms[myroom].peoples[i].style=-1;
            rooms[myroom].people--;
            if(inrooms[myroom].peoples[i].flag&PEOPLE_ROOMOP) {
                for(k=0;k<MAX_PEOPLE;k++) 
                if(inrooms[myroom].peoples[k].style!=-1&&!(inrooms[myroom].peoples[k].flag&PEOPLE_SPECTATOR))
                {
                    inrooms[myroom].peoples[k].flag|=PEOPLE_ROOMOP;
                    sprintf(buf, "%s��Ϊ�·���", inrooms[myroom].peoples[k].nick);
                    send_msg(-1, buf);
                    break;
                }
            }
            end_change_inroom();
            i=-1;
        }
    }
}

int add_room(struct room_struct * r)
{
    int i,j;
    for(i=0;i<MAX_ROOM;i++) 
    if(rooms[i].style==1) {
        if(!strcmp(rooms[i].name, r->name))
            return -1;
        if(!strcmp(rooms[i].creator, currentuser->userid))
            return -1;
    }
    for(i=0;i<MAX_ROOM;i++)
    if(rooms[i].style==-1) {
        memcpy(rooms+i, r, sizeof(struct room_struct));
        inrooms[i].status = INROOM_STOP;
        inrooms[i].killernum = 0;
        inrooms[i].msgi = 0;
        inrooms[i].policenum = 0;
        inrooms[i].w = 0;
        for(j=0;j<MAX_MSG;j++)
            inrooms[i].msgs[j][0]=0;
        for(j=0;j<MAX_PEOPLE;j++)
            inrooms[i].peoples[j].style = -1;
        return 0;
    }
    return -1;
}

/*
int del_room(struct room_struct * r)
{
    int i, j;
    for(i=0;i<*roomst;i++)
    if(!strcmp(rooms[i].name, r->name)) {
        rooms[i].name[0]=0;
        break;
    }
    return 0;
}
*/

void clear_room()
{
    int i;
    for(i=0;i<MAX_ROOM;i++)
        if((rooms[i].style!=-1) && (rooms[i].people==0))
            rooms[i].style=-1;
}

int can_see(struct room_struct * r)
{
    if(r->style==-1) return 0;
//    if(r->level&currentuser.userlevel!=r->level) return 0;
    if(r->style!=1) return 0;
    if(r->flag&ROOM_SECRET&&!USERPERM(currentuser, PERM_SYSOP)) return 0;
    return 1;
}

int can_enter(struct room_struct * r)
{
    if(r->style==-1) return 0;
//    if(r->level&currentuser.userlevel!=r->level) return 0;
    if(r->style!=1) return 0;
    if(r->flag&ROOM_LOCKED&&!USERPERM(currentuser, PERM_SYSOP)) return 0;
    return 1;
}

int room_count()
{
    int i,j=0;
    for(i=0;i<MAX_ROOM;i++)
        if(can_see(rooms+i)) j++;
    return j;
}

int room_get(int w)
{
    int i,j=0;
    for(i=0;i<MAX_ROOM;i++) {
        if(can_see(rooms+i)) {
            if(w==j) return i;
            j++;
        }
    }
    return -1;
}

int find_room(char * s)
{
    int i;
    struct room_struct * r2;
    for(i=0;i<MAX_ROOM;i++) {
        r2 = rooms+i;
        if(!can_enter(r2)) continue;
        if(!strcmp(r2->name, s))
            return i;
    }
    return -1;
}

int selected = 0, ipage=0, jpage=0;

int getpeople(int i)
{
    int j, k=0;
    for(j=0;j<MAX_PEOPLE;j++) {
        if(inrooms[myroom].peoples[j].style==-1) continue;
        if(i==k) return j;
        k++;
    }
    return -1;
}

int get_msgt()
{
    int i,j=0,k;
    for(i=0;i<MAX_MSG;i++) {
        if(inrooms[myroom].msgs[(i+inrooms[myroom].msgi)%MAX_MSG][0]==0) break;
        k=inrooms[myroom].msgpid[(i+inrooms[myroom].msgi)%MAX_MSG];
        if(k==-1||k==uinfo.pid) j++;
    }
    return j;
}

char * get_msgs(int s)
{
    int i,j=0,k;
    char * ss;
    for(i=0;i<MAX_MSG;i++) {
        if(inrooms[myroom].msgs[(i+inrooms[myroom].msgi)%MAX_MSG][0]==0) break;
        k=inrooms[myroom].msgpid[(i+inrooms[myroom].msgi)%MAX_MSG];
        if(k==-1||k==uinfo.pid) {
            if(j==s) {
                ss = inrooms[myroom].msgs[(i+inrooms[myroom].msgi)%MAX_MSG];
                return ss;
            }
            j++;
        }
    }
    return NULL;
}

void save_msgs(char * s)
{
    FILE* fp;
    int i;
    fp=fopen(s, "w");
    if(fp==NULL) return;
    for(i=0;i<get_msgt();i++)
        fprintf(fp, "%s\n", get_msgs(i));
    fclose(fp);
}

void refreshit()
{
    int i, j, me, msgst, k, i0, m_col;
    char buf[30], buf2[30], buf3[30], disp[256];
    for (i = 0; i < t_lines - 1; i++) {
	move(i, 0);
	clrtoeol();
    }
    move(0, 0);
    prints
	("\033[44;33;1m ����:\033[36m%-12s\033[33m����:\033[36m%-40s\033[33m״̬:\033[36m%6s\033[m",
	 rooms[myroom].name, rooms[myroom].title,
	 (inrooms[myroom].status ==
	  INROOM_STOP ? "δ��ʼ" : (inrooms[myroom].status ==
				    INROOM_NIGHT ? "��ҹ��" : "�����")));
    clrtoeol();
    k_resetcolor();
    k_setfcolor(YELLOW, 1);
    move(1, 0);
    prints("\033[1;33m");
    prints
	("�q\033[1;32m���\033[33m�����������r�q\033[1;32mѶϢ\033[33m�������������������������������������������������������r");
    move(t_lines - 2, 0);
    prints
	("�t���������������s�t�����������������������������������������������������������s\033[m");

    me = mypos;
    msgst = get_msgt();

    for (i = 2; i <= t_lines - 3; i++) {
	strcpy(disp, "��");

	// Ҫ��ʾ�û� 
	if (ipage + i - 2 >= 0 && ipage + i - 2 < rooms[myroom].people) {
	    j = getpeople(ipage + i - 2);
	    if (j == -1)
		continue;
	    if (inrooms[myroom].status != INROOM_STOP	//!!!!!!!!!11
		 && inrooms[myroom].peoples[j].flag&PEOPLE_KILLER && 
        	   (inrooms[myroom].peoples[me].flag&PEOPLE_KILLER ||
	            inrooms[myroom].peoples[me].flag&PEOPLE_SPECTATOR ||
	            !(inrooms[myroom].peoples[j].flag&PEOPLE_ALIVE))) {
		    strcat(disp, "\033[31m*");
		} else 
        	    strcat(disp, " ");

	    if (inrooms[myroom].status != INROOM_STOP
		&& !(inrooms[myroom].peoples[j].flag & PEOPLE_ALIVE)
		&& !(inrooms[myroom].peoples[j].flag & PEOPLE_SPECTATOR)) {
		strcat(disp, "\033[34mX");
	    }	else if ((inrooms[myroom].peoples[j].flag & PEOPLE_SPECTATOR)) {
		strcat(disp, "\033[32mO");
	    }	else if (inrooms[myroom].status == INROOM_DAY ||
			 (inrooms[myroom].status == INROOM_NIGHT &&
			 (inrooms[myroom].peoples[me].flag & PEOPLE_KILLER
			  || inrooms[myroom].peoples[me].flag & PEOPLE_SPECTATOR
			  || inrooms[myroom].peoples[me].flag & PEOPLE_POLICE)))
		    if ((inrooms[myroom].peoples[j].flag & PEOPLE_ALIVE)
			&& (inrooms[myroom].peoples[j].vote != 0)) {
		    	strcat(disp, "\033[0;33mv\033[1m");
		    }
	    m_col = 4;
	    if (ipage + i - 2 == selected) {
		k_setfcolor(RED, 1);
		strcat(disp, "\033[31m");
	    } else
		strcat(disp, "\033[m");

        if (inrooms[myroom].peoples[j].nick[0] == '\0') 
            sprintf(buf, "%d %-9.9s", j + 1, inrooms[myroom].peoples[j].id);
        else 
    	    sprintf(buf, "%d %-9.9s", j + 1, inrooms[myroom].peoples[j].nick);
	    //cutHalf(buf);
	    //buf[strlen(buf) / 2] = 0;
	    //buf[12]=0;
	    if (inrooms[myroom].status == INROOM_DAY ||
		(inrooms[myroom].status == INROOM_NIGHT &&
		(inrooms[myroom].peoples[me].flag & PEOPLE_KILLER ||
		 inrooms[myroom].peoples[me].flag & PEOPLE_SPECTATOR ||
		 inrooms[myroom].peoples[me].flag & PEOPLE_POLICE))) {
		k = 0;
		for (i0 = 0; i0 < MAX_PEOPLE; i0++)
		    if (inrooms[myroom].peoples[i0].style != -1
			&& inrooms[myroom].peoples[i0].vote ==
			inrooms[myroom].peoples[j].pid)
			k++;
		if (k > 0) {
		    int j0 = 0;
		    if (k >= strlen(buf))
			k = strlen(buf);
		    for (i0 = 0; i0 < k; i0++) {
			if (j0)
			    j0 = 0;
			else if (buf[i0] < 0)
			    j0 = 1;
		    }
		    if (j0 && k < strlen(buf))
			k++;
		    strcpy(buf2, buf);
		    buf2[k] = 0;
		    strcpy(buf3, buf + k);
		    sprintf(buf, "\033[4m%s\033[m%s%s", buf2,
			    (ipage + i - 2 ==
			     selected) ? "\033[31;1m" : "", buf3);
		}
	    }
	   
	    m_col = strlen2(disp);
	    while (m_col < 4) {
		strcat(disp, " ");
		m_col++;
	    }
	    strcat(disp, buf);
	}
	m_col = strlen2(disp);
	while (m_col < 16) {
	    strcat(disp, " ");
	    m_col++;
	}
	strcat(disp, "\033[1;33m����\033[m");

	// ������ʾ����Ϣ
	if (msgst - 1 - (t_lines - 3 - i) - jpage >= 0) {
	    char *ss = get_msgs(msgst - 1 - (t_lines - 3 - i) - jpage);
	    if (ss && !strcmp(ss, "�㱻����"))
		kicked = 1;
	    if (ss)
		strcat(disp, ss);
	}

	m_col = strlen2(disp);
	while (m_col < 78) {
	    strcat(disp, " ");
	    m_col++;
	}
	strcat(disp, "\033[1;33m��");

	// ��ʾ
	move(i, 0);
	prints(disp);
    }

}


extern int RMSG;

void room_refresh(int signo)
{
    int y,x;
    signal(SIGUSR1, room_refresh);

    if(RMSG) return;
    if(rooms[myroom].style!=1) kicked = 1;
    
    getyx(&y, &x);
    refreshit();
    move(y, x);
    refresh();
}

void start_killergame()
{
    int i,j,totalk=0,total=0,totalc=0, me;
    char buf[80];
    me=mypos;
    for(i=0;i<MAX_PEOPLE;i++) 
    if(inrooms[myroom].peoples[i].style!=-1)
    {
        inrooms[myroom].peoples[i].flag &= ~PEOPLE_KILLER;
        inrooms[myroom].peoples[i].flag &= ~PEOPLE_POLICE;
        inrooms[myroom].peoples[i].vote = 0;
    }
    totalk=inrooms[myroom].killernum;
    totalc=inrooms[myroom].policenum;
    for(i=0;i<MAX_PEOPLE;i++)
        if(inrooms[myroom].peoples[i].style!=-1)
        if(!(inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR)) 
            total++;
    if(total<6) {
        send_msg(me, "\033[31;1m����6�˲μӲ��ܿ�ʼ��Ϸ\033[m");
        end_change_inroom();
        refreshit();
        return;
    }
    if(totalk==0) totalk=((double)total/6+0.5);
    if(totalk>=total/4) totalk=total/4;
    if(totalc>=total/6) totalc=total/6;
    if(totalk>total) {
        send_msg(me, "\033[31;1m����������Ҫ��Ļ�������,�޷���ʼ��Ϸ\033[m");
        end_change_inroom();
        refreshit();
        return;
    }
    if(totalc==0)
        sprintf(buf, "\033[31;1m��Ϸ��ʼ��! ��Ⱥ�г�����%d������\033[m", totalk);
    else
        sprintf(buf, "\033[31;1m��Ϸ��ʼ��! ��Ⱥ�г�����%d������, %d������\033[m", totalk, totalc);
    send_msg(-1, buf);
    for(i=0;i<totalk;i++) {
        do{
            j=rand()%MAX_PEOPLE;
        }while(inrooms[myroom].peoples[j].style==-1 || inrooms[myroom].peoples[j].flag&PEOPLE_KILLER || inrooms[myroom].peoples[j].flag&PEOPLE_SPECTATOR);
        inrooms[myroom].peoples[j].flag |= PEOPLE_KILLER;
        send_msg(j, "������һ���޳ܵĻ���");
        send_msg(j, "����ļ⵶(\033[31;1mCtrl+S\033[m)ѡ����Ҫ�к����˰�...");
    }
    for(i=0;i<totalc;i++) {
        do{
            j=rand()%MAX_PEOPLE;
        }while(inrooms[myroom].peoples[j].style==-1 || inrooms[myroom].peoples[j].flag&PEOPLE_KILLER || inrooms[myroom].peoples[j].flag&PEOPLE_POLICE || inrooms[myroom].peoples[j].flag&PEOPLE_SPECTATOR);
        inrooms[myroom].peoples[j].flag |= PEOPLE_POLICE;
        send_msg(j, "������һλ���ٵ����񾯲�");
        send_msg(j, "����������ľ���(\033[31;1mCtrl+T\033[m)ѡ���㻳�ɵ���...");
    }
    for(i=0;i<MAX_PEOPLE;i++) 
    if(inrooms[myroom].peoples[i].style!=-1)
    if(!(inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR))
    {
        inrooms[myroom].peoples[i].flag |= PEOPLE_ALIVE;
        if(!(inrooms[myroom].peoples[i].flag&PEOPLE_KILLER))
            send_msg(i, "����������...");
    }
    inrooms[myroom].status = INROOM_NIGHT;
    end_change_inroom();
    kill_msg(-1);
}

#define menust 8
int do_com_menu()
{
    char menus[menust][15] =
	{ "0-����", "1-�˳���Ϸ", "2-������", "3-����б�", "4-�Ļ���",
	"5-���÷���", "6-�����", "7-��ʼ��Ϸ"
    };
    int i, j, k, sel = 0, ch, max = 0, me;
    char buf[80], disp[128];
    if (inrooms[myroom].status != INROOM_STOP)
	strcpy(menus[7], "7-������Ϸ");
    do {
	k_resetcolor();
	move(t_lines - 1, 0);
	clrtoeol();
	j = mypos;
	disp[0] = '\0';
	for (i = 0; i < menust; i++)
	    if (inrooms[myroom].peoples[j].flag & PEOPLE_ROOMOP || i <= 3) {
		if (i == sel) {
		    strcat(disp, "\033[1;31m");
		    strcat(disp, menus[i]);
		    strcat(disp, "\033[m");
		} else
		    strcat(disp, menus[i]);
		strcat(disp, " ");
		if (i >= max - 1)
		    max = i + 1;
	    }
	prints(disp);


        ch=igetkey();
        if(kicked) return 0;
        switch(ch){
        case KEY_LEFT:
        case KEY_UP:
            sel--;
            if(sel<0) sel=max-1;
            break;
        case KEY_RIGHT:
        case KEY_DOWN:
            sel++;
            if(sel>=max) sel=0;
            break;
        case '\n':
        case '\r':
            switch(sel) {
                case 0:
                    return 0;
                case 1:
                    me=mypos;
                    if(inrooms[myroom].peoples[me].flag&PEOPLE_ALIVE&&!(inrooms[myroom].peoples[me].flag&PEOPLE_ROOMOP)&&inrooms[myroom].status!=INROOM_STOP) {
                        send_msg(me, "�㻹����Ϸ,�����˳�");
                        refreshit();
                        return 0;
                    }
                    return 1;
                case 2:
                    move(t_lines-1, 0);
                    k_resetcolor();
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "����������:", buf, 13, 1, 0);
                    if(kicked) return 0;
                    if(buf[0]) {
                        k=1;
                        for(j=0;j<strlen(buf);j++)
                            k=k&&(isprint2(buf[j]));
                        k=k&&(buf[0]!=' ');
                        k=k&&(buf[strlen(buf)-1]!=' ');
                        if(!k) {
                            move(t_lines-1,0);
                            k_resetcolor();
                            clrtoeol();
                            prints(" ���ֲ����Ϲ淶");
                            refresh();
                            sleep(1);
                            return 0;
                        }
                        me=mypos;
                        j=0;
                        for(i=0;i<MAX_PEOPLE;i++)
                            if(inrooms[myroom].peoples[i].style!=-1)
                            if(i!=me)
                            if(!strcmp(buf,inrooms[myroom].peoples[i].id) || !strcmp(buf,inrooms[myroom].peoples[i].nick)) j=1;
                        if(j) {
                            move(t_lines-1,0);
                            k_resetcolor();
                            clrtoeol();
                            prints(" �����������������");
                            refresh();
                            sleep(1);
                            return 0;
                        }
                        start_change_inroom();
                        me=mypos;
                        strcpy(inrooms[myroom].peoples[me].nick, buf);
                        end_change_inroom();
                        kill_msg(-1);
                    }
                    return 0;
                case 3:
                    me=mypos;
                    for(i=0;i<MAX_PEOPLE;i++) 
                    if(inrooms[myroom].peoples[i].style!=-1)
                    {
                        sprintf(buf, "%-12s  %s", inrooms[myroom].peoples[i].id, inrooms[myroom].peoples[i].nick);
                        send_msg(me, buf);
                    }
                    refreshit();
                    return 0;
                case 4:
                    move(t_lines-1, 0);
                    k_resetcolor();
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "�����뻰��:", buf, 31, 1, 1);
                    if(kicked) return 0;
                    if(buf[0]) {
                        start_change_inroom();
                        strcpy(rooms[myroom].title, buf);
                        end_change_inroom();
                        kill_msg(-1);
                    }
                    return 0;
                case 5:
                    move(t_lines-1, 0);
                    k_resetcolor();
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "�����뷿���������:", buf, 30, 1, 1);
                    if(kicked) return 0;
                    if(buf[0]) {
                        i=atoi(buf);
                        if(i>0&&i<=100) {
                            rooms[myroom].maxpeople = i;
                            sprintf(buf, "�������÷����������Ϊ%d", i);
                            send_msg(-1, buf);
                        }
                    }
                    move(t_lines-1, 0);
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "����Ϊ���ط���?\033[m[Y/N]", buf, 30, 1, 1);
                    if(kicked) return 0;
                    buf[0]=toupper(buf[0]);
                    if(buf[0]=='Y'||buf[0]=='N') {
                        if(buf[0]=='Y') rooms[myroom].flag|=ROOM_SECRET;
                        else rooms[myroom].flag&=~ROOM_SECRET;
                        sprintf(buf, "�������÷���Ϊ%s", (buf[0]=='Y')?"����":"������");
                        send_msg(-1, buf);
                    }
                    move(t_lines-1, 0);
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "����Ϊ��������?\033[m[Y/N]", buf, 30, 1,  1);
                    if(kicked) return 0;
                    buf[0]=toupper(buf[0]);
                    if(buf[0]=='Y'||buf[0]=='N') {
                        if(buf[0]=='Y') rooms[myroom].flag|=ROOM_LOCKED;
                        else rooms[myroom].flag&=~ROOM_LOCKED;
                        sprintf(buf, "�������÷���Ϊ%s", (buf[0]=='Y')?"����":"������");
                        send_msg(-1, buf);
                    }
                    move(t_lines-1, 0);
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "����Ϊ�ܾ��Թ��ߵķ���?\033[m[Y/N]", buf, 30, 1, 1);
                    if(kicked) return 0;
                    buf[0]=toupper(buf[0]);
                    if(buf[0]=='Y'||buf[0]=='N') {
                        if(buf[0]=='Y') rooms[myroom].flag|=ROOM_DENYSPEC;
                        else rooms[myroom].flag&=~ROOM_DENYSPEC;
                        sprintf(buf, "�������÷���Ϊ%s", (buf[0]=='Y')?"�ܾ��Թ�":"���ܾ��Թ�");
                        send_msg(-1, buf);
                    }
                    move(t_lines-1, 0);
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "���û��˵���Ŀ:", buf, 30, 1,  1);
                    if(kicked) return 0;
                    if(buf[0]) {
                        i=atoi(buf);
                        if(i>=0&&i<=10) {
                            inrooms[myroom].killernum = i;
                            sprintf(buf, "�������÷��仵����Ϊ%d", i);
                            send_msg(-1, buf);
                        }
                    }
                    move(t_lines-1, 0);
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "���þ������Ŀ:", buf, 30, 1,  1);
                    if(kicked) return 0;
                    if(buf[0]) {
                        i=atoi(buf);
                        if(i>=0&&i<=2) {
                            inrooms[myroom].policenum = i;
                            sprintf(buf, "�������÷��侯����Ϊ%d", i);
                            send_msg(-1, buf);
                        }
                    }
                    kill_msg(-1);
                    return 0;
                case 6:
                    move(t_lines-1, 0);
                    k_resetcolor();
                    clrtoeol();
                    k_getdata(t_lines-1, 0, "������Ҫ�ߵ�id:", buf, 30, 1,  1);
                    if(kicked) return 0;
                    if(buf[0]) {
                        for(i=0;i<MAX_PEOPLE;i++)
                            if(inrooms[myroom].peoples[i].style!=-1)
                            if(!strcmp(inrooms[myroom].peoples[i].id, buf)) break;
                        if(!strcmp(inrooms[myroom].peoples[i].id, buf) && inrooms[myroom].peoples[i].pid!=uinfo.pid) {
                            inrooms[myroom].peoples[i].flag&=~PEOPLE_ALIVE;
                            send_msg(i, "�㱻����");
                            kill_msg(i);
                            return 2;
                        }
                    }
                    return 0;
                case 7:
                    start_change_inroom();
                    if(inrooms[myroom].status == INROOM_STOP)
                        start_killergame();
                    else {
                        inrooms[myroom].status = INROOM_STOP;
                        send_msg(-1, "��Ϸ������ǿ�ƽ���");
                        end_change_inroom();
                        kill_msg(-1);
                    }
                    return 0;
            }
            break;
        default:
            for(i=0;i<max;i++)
                if(ch==menus[i][0]) sel=i;
            break;
        }
    }while(1);
}

void join_room(int w, int spec)
{
    char buf[200],buf2[200],buf3[200],msg[200],roomname[80];
    int i,j,k,me;
    clear();
    myroom = w;
    start_change_inroom();
    if(rooms[myroom].style!=1) {
        end_change_inroom();
        return;
    }
    strcpy(roomname, rooms[myroom].name);
    signal(SIGUSR1, room_refresh);
    i=0;
    while(inrooms[myroom].peoples[i].style!=-1) i++;
    mypos = i;
    inrooms[myroom].peoples[i].style = 0;
    inrooms[myroom].peoples[i].flag = 0;
    strcpy(inrooms[myroom].peoples[i].id, currentuser->userid);
    strcpy(inrooms[myroom].peoples[i].nick, currentuser->userid);
    inrooms[myroom].peoples[i].pid = uinfo.pid;
    if(rooms[myroom].people==0 && !strcmp(rooms[myroom].creator, currentuser->userid))
        inrooms[myroom].peoples[i].flag = PEOPLE_ROOMOP;
    if(spec) inrooms[myroom].peoples[i].flag|=PEOPLE_SPECTATOR;
    rooms[myroom].people++;
    end_change_inroom();

    kill_msg(-1);
    sprintf(buf, "\033[36m%s���뷿��\033[0m", currentuser->userid);
    send_msg(-1, buf);

    room_refresh(0);
    while(1){
        do{
            int ch;
            ch=-k_getdata(t_lines-1, 0, "����:", buf, 70, 1, 1);
            if(rooms[myroom].style!=1) kicked = 1;
            if(kicked) goto quitgame;
            if(ch==KEY_UP) {
                selected--;
                if(selected<0) selected = rooms[myroom].people-1;
                if(ipage>selected) ipage=selected;
                if(selected>ipage+t_lines-5) ipage=selected-(t_lines-5);
                refreshit();
            }
            else if(ch==KEY_DOWN) {
                selected++;
                if(selected>=rooms[myroom].people) selected=0;
                if(ipage>selected) ipage=selected;
                if(selected>ipage+t_lines-5) ipage=selected-(t_lines-5);
                refreshit();
            }
            else if(ch==KEY_PGUP) {
                jpage+=t_lines/2;
                refreshit();
            }
            else if(ch==KEY_PGDN) {
                jpage-=t_lines/2;
                if(jpage<=0) jpage=0;
                refreshit();
            }
            else if(ch==Ctrl('T')&&inrooms[myroom].status == INROOM_DAY) {
                int pid;
                int sel;
                sel = getpeople(selected);
                if(sel==-1) continue;
                me = mypos;
                pid = inrooms[myroom].peoples[me].pid;
                if(!(inrooms[myroom].peoples[me].flag&PEOPLE_POLICE))
                    continue;
                if(!(inrooms[myroom].peoples[me].flag&PEOPLE_ALIVE))
                    continue;
                if(inrooms[myroom].peoples[me].flag&PEOPLE_TESTED) {
                    send_msg(me, "\033[31;1m�������Ѿ�������\033[m");
                    refreshit();
                    continue;
                }
                if(inrooms[myroom].peoples[sel].flag&PEOPLE_SPECTATOR)
                    send_msg(me, "\033[31;1m�������Թ���\033[m");
                else if(!(inrooms[myroom].peoples[sel].flag&PEOPLE_ALIVE))
                    send_msg(me, "\033[31;1m��������\033[m");
                else if((inrooms[myroom].peoples[sel].flag&PEOPLE_KILLER)) {
                    inrooms[myroom].peoples[me].flag|=PEOPLE_TESTED;
                    send_msg(me, "\033[31;1m����������, ���ִ����ǻ���!!!\033[m");
                }
                else {
                    inrooms[myroom].peoples[me].flag|=PEOPLE_TESTED;
                    send_msg(me, "\033[31;1m����������, ���ִ����Ǻ���\033[m");
                }
                refreshit();
            }
            else if(ch==Ctrl('S')) {
                int pid;
                int sel;
                sel = getpeople(selected);
                if(sel==-1) continue;
                me=mypos;
                pid=inrooms[myroom].peoples[sel].pid;
                if(inrooms[myroom].peoples[me].vote==0)
                if(inrooms[myroom].peoples[me].flag & PEOPLE_ALIVE &&
                    ((inrooms[myroom].peoples[me].flag & PEOPLE_KILLER && inrooms[myroom].status==INROOM_NIGHT) ||
                    inrooms[myroom].status==INROOM_DAY)) {
                    if(inrooms[myroom].peoples[sel].flag&PEOPLE_ALIVE && 
                        !(inrooms[myroom].peoples[sel].flag&PEOPLE_SPECTATOR) &&
                        sel!=me) {
                        int i,j,t1,t2,t3;
                        sprintf(buf, "\033[32;1m%d %sͶ��%d %sһƱ\033[m", me+1, inrooms[myroom].peoples[me].nick,
                            sel+1, inrooms[myroom].peoples[sel].nick);
                        start_change_inroom();
                        inrooms[myroom].peoples[me].vote = pid;
                        end_change_inroom();
                        if(inrooms[myroom].status==INROOM_NIGHT) {
                            for(i=0;i<MAX_PEOPLE;i++)
                                if(inrooms[myroom].peoples[i].style!=-1)
                                if(inrooms[myroom].peoples[i].flag&PEOPLE_KILLER||
                                    inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR)
                                    send_msg(i, buf);
                        }
                        else {
                            send_msg(-1, buf);
                        }
checkvote:
                        t1=0; t2=0; t3=0;
                        for(i=0;i<MAX_PEOPLE;i++)
                            inrooms[myroom].peoples[i].vnum = 0;
                        for(i=0;i<MAX_PEOPLE;i++)
                        if(inrooms[myroom].peoples[i].style!=-1)
                        if(!(inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR) &&
                            inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE &&
                            (inrooms[myroom].peoples[i].flag&PEOPLE_KILLER||inrooms[myroom].status==INROOM_DAY)) {
                            for(j=0;j<MAX_PEOPLE;j++)
                                if(inrooms[myroom].peoples[j].style!=-1)
                                if(inrooms[myroom].peoples[j].pid == inrooms[myroom].peoples[i].vote)
                                    inrooms[myroom].peoples[j].vnum++;
                        }
                        for(i=0;i<MAX_PEOPLE;i++)
                        if(inrooms[myroom].peoples[i].style!=-1)
                        if(!(inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR) &&
                            inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE) {
                            if(inrooms[myroom].peoples[i].vnum>=t1) {
                                t2=t1; t1=inrooms[myroom].peoples[i].vnum;
                            }
                            else if(inrooms[myroom].peoples[i].vnum>=t2) {
                                t2=inrooms[myroom].peoples[i].vnum;
                            }
                        }
                        j=1;
                        for(i=0;i<MAX_PEOPLE;i++)
                        if(inrooms[myroom].peoples[i].style!=-1)
                        if(!(inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR) &&
                            inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE &&
                            (inrooms[myroom].peoples[i].flag&PEOPLE_KILLER||inrooms[myroom].status==INROOM_DAY))
                            if(inrooms[myroom].peoples[i].vote == 0) {
                                j=0;
                                t3++;
                            }
                        if(j || t1-t2>t3) {
                            int max=0, ok=0, maxi=0, maxpid=0;	//!!!
                            for(i=0;i<MAX_PEOPLE;i++)
                                inrooms[myroom].peoples[i].vnum = 0;
                            for(i=0;i<MAX_PEOPLE;i++)
                            if(inrooms[myroom].peoples[i].style!=-1)
                            if(!(inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR) &&
                                inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE &&
                                (inrooms[myroom].peoples[i].flag&PEOPLE_KILLER||inrooms[myroom].status==INROOM_DAY)) {
                                for(j=0;j<MAX_PEOPLE;j++)
                                if(inrooms[myroom].peoples[j].style!=-1)
                                    if(inrooms[myroom].peoples[j].pid == inrooms[myroom].peoples[i].vote)
                                        inrooms[myroom].peoples[j].vnum++;
                            }
                            sprintf(buf, "ͶƱ���:");
                            if(inrooms[myroom].status==INROOM_NIGHT) {
                                for(j=0;j<MAX_PEOPLE;j++)
                                    if(inrooms[myroom].peoples[j].style!=-1)
                                    if(inrooms[myroom].peoples[j].flag&PEOPLE_KILLER||
                                        inrooms[myroom].peoples[j].flag&PEOPLE_SPECTATOR)
                                        send_msg(j, buf);
                            }
                            else {
                                send_msg(-1, buf);
                            }
                            for(i=0;i<MAX_PEOPLE;i++)
                            if(inrooms[myroom].peoples[i].style!=-1)
                            if(!(inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR) &&
                                inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE) {
                                sprintf(buf, "%s��ͶƱ��: %d Ʊ", 
                                    inrooms[myroom].peoples[i].nick,
                                    inrooms[myroom].peoples[i].vnum);
                                if(inrooms[myroom].peoples[i].vnum==max)
                                    ok=0;
                                if(inrooms[myroom].peoples[i].vnum>max) {
                                    max=inrooms[myroom].peoples[i].vnum;
                                    ok=1;
                                    maxi=i;
                                    maxpid=inrooms[myroom].peoples[i].pid;
                                }
                                if(inrooms[myroom].status==INROOM_NIGHT) {
                                    for(j=0;j<MAX_PEOPLE;j++)
                                        if(inrooms[myroom].peoples[j].style!=-1)
                                        if(inrooms[myroom].peoples[j].flag&PEOPLE_KILLER||
                                            inrooms[myroom].peoples[j].flag&PEOPLE_SPECTATOR)
                                            send_msg(j, buf);
                                }
                                else {
                                    send_msg(-1, buf);
                                }
                            }
                            if(!ok) {
                                sprintf(buf, "���Ʊ����ͬ,������Э�̽��...");
                                if(inrooms[myroom].status==INROOM_NIGHT) {
                                    for(j=0;j<MAX_PEOPLE;j++)
                                        if(inrooms[myroom].peoples[j].style!=-1)
                                        if(inrooms[myroom].peoples[j].flag&PEOPLE_KILLER||
                                            inrooms[myroom].peoples[j].flag&PEOPLE_SPECTATOR)
                                            send_msg(j, buf);
                                }
                                else
                                    send_msg(-1, buf);
                                start_change_inroom();
                                for(j=0;j<MAX_PEOPLE;j++)
                                    inrooms[myroom].peoples[j].vote=0;
                                end_change_inroom();
                            }
                            else {
                                int a=0,b=0;
                                if(inrooms[myroom].status == INROOM_DAY)
                                    sprintf(buf, "�㱻��Ҵ�����!");
                                else
                                    sprintf(buf, "�㱻����ɱ����!");
                                send_msg(maxi, buf);
                                if(inrooms[myroom].status == INROOM_DAY) {
                                    if(inrooms[myroom].peoples[maxi].flag&PEOPLE_KILLER)
                                        sprintf(buf, "����%s��������!",
                                            inrooms[myroom].peoples[maxi].nick);
                                    else
                                        sprintf(buf, "����%s��������!",
                                            inrooms[myroom].peoples[maxi].nick);
                                }
                                else
                                    sprintf(buf, "%s��ɱ����!",
                                        inrooms[myroom].peoples[maxi].nick);
                                for(j=0;j<MAX_PEOPLE;j++)
                                    if(inrooms[myroom].peoples[j].style!=-1)
                                    if(j!=maxi)
                                        send_msg(j, buf);
                                start_change_inroom();
                                for(i=0;i<MAX_PEOPLE;i++)
                                    if(inrooms[myroom].peoples[i].style!=-1)
                                    if(inrooms[myroom].peoples[i].pid == maxpid)
                                        inrooms[myroom].peoples[i].flag &= ~PEOPLE_ALIVE;
                                for(i=0;i<MAX_PEOPLE;i++)
                                    if(inrooms[myroom].peoples[i].style!=-1)
                                    if(inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE) {
                                        if(inrooms[myroom].peoples[i].flag&PEOPLE_KILLER) a++;
                                        else b++;
                                    }
                                if(a>0&&a>=b-1&&inrooms[myroom].status==INROOM_DAY) {
                                    inrooms[myroom].status = INROOM_STOP;
                                    send_msg(-1, "���˻����ʤ��...");
   				    //save_result(1);
                                    for(j=0;j<MAX_PEOPLE;j++)
                                    if(inrooms[myroom].peoples[j].style!=-1)
                                    if(inrooms[myroom].peoples[j].flag&PEOPLE_KILLER &&
                                        inrooms[myroom].peoples[j].flag&PEOPLE_ALIVE) {
                                        sprintf(buf, "ԭ��%s�ǻ���!",
                                            inrooms[myroom].peoples[j].nick);
                                        send_msg(-1, buf);
                                    }
                                }
                                else if(a==0) {
                                    inrooms[myroom].status = INROOM_STOP;
                                    send_msg(-1, "���л��˶��������ˣ����˻����ʤ��...");
                                    //save_result(0);
                                }
                                else if(inrooms[myroom].status == INROOM_DAY) {
                                    inrooms[myroom].status = INROOM_NIGHT;
                                    send_msg(-1, "�ֲ���ҹɫ�ֽ�����...");
                                    for(i=0;i<MAX_PEOPLE;i++) 
                                        if(inrooms[myroom].peoples[i].style!=-1)
                                        if(inrooms[myroom].peoples[i].flag&PEOPLE_KILLER&&inrooms[myroom].peoples[i].flag&PEOPLE_ALIVE)
                                            send_msg(i, "��ץ����ı���ʱ����\033[31;1mCtrl+S\033[mɱ��...");
                                }
                                else {
                                    for(i=0;i<MAX_PEOPLE;i++)
                                        inrooms[myroom].peoples[i].flag&=(~PEOPLE_TESTED);
                                    inrooms[myroom].status = INROOM_DAY;
                                    send_msg(-1, "������...");
                                }
                                for(i=0;i<MAX_PEOPLE;i++)
                                    inrooms[myroom].peoples[i].vote = 0;
                                end_change_inroom();
                            }
                        }
                        kill_msg(-1);
                    }
                    else {
                        if(sel==me)
                            send_msg(me, "\033[31;1m�㲻��ѡ����ɱ\033[m");
                        else if(!(inrooms[myroom].peoples[sel].flag&PEOPLE_ALIVE))
                            send_msg(me, "\033[31;1m��������\033[m");
                        else
                            send_msg(me, "\033[31;1m�������Թ���\033[m");
                        refreshit();
                    }
                }
            }
            else if(ch<=0&&!buf[0]) {
                j=do_com_menu();
                if(kicked) goto quitgame;
                if(j==1) goto quitgame;
                if(j==2) if(inrooms[myroom].status!=INROOM_STOP) goto checkvote;
            }
            else if(ch<=0){
                break;
            }
        }while(1);
        start_change_inroom();
        me=mypos;
        strcpy(msg, buf);
        if(msg[0]=='/'&&msg[1]=='/') {
            i=2;
            while(msg[i]!=' '&&i<strlen(msg)) i++;
            strcpy(buf, msg+2);
            buf[i-2]=0;
            while(msg[i]==' '&&i<strlen(msg)) i++;
            buf2[0]=0; buf3[0]=0;
            if(msg[i-1]==' '&&i<strlen(msg)) {
                k=i;
                while(msg[k]!=' '&&k<strlen(msg)) k++;
                strcpy(buf2, msg+i);
                buf2[k-i]=0;
                i=k;
                while(msg[i]==' '&&i<strlen(msg)) i++;
                if(msg[i-1]==' '&&i<strlen(msg)) {
                    k=i;
                    while(msg[k]!=' '&&k<strlen(msg)) k++;
                    strcpy(buf3, msg+i);
                    buf3[k-i]=0;
                }
            }
            k=1;
            for(i=0;;i++) {
                if(!party_data[i].verb) break;
                if(!strcasecmp(party_data[i].verb, buf)) {
                    k=0;
                    sprintf(buf, "%s \033[1;37m%s\033[0m %s", party_data[i].part1_msg, buf2[0]?buf2:"���", party_data[i].part2_msg);
                    break;
                }
            }
            if(k)
            for(i=0;;i++) {
                if(!speak_data[i].verb) break;
                if(!strcasecmp(speak_data[i].verb, buf)) {
                    k=0;
                    sprintf(buf, "%s: %s", speak_data[i].part1_msg, buf2);
                    break;
                }
            }
            if(k)
            for(i=0;;i++) {
                if(!condition_data[i].verb) break;
                if(!strcasecmp(condition_data[i].verb, buf)) {
                    k=0;
                    sprintf(buf, "%s", condition_data[i].part1_msg);
                    break;
                }
            }

            if(k) continue;
            strcpy(buf2, buf);
            sprintf(buf, "\033[1m%d %s\033[m %s", me+1, inrooms[myroom].peoples[me].nick, buf2);
        }
        else {
            strcpy(buf2, buf);
            sprintf(buf, "%d %s: %s", me+1, inrooms[myroom].peoples[me].nick, buf2);
        }
        if(inrooms[myroom].status==INROOM_NIGHT) {
            if(inrooms[myroom].peoples[me].flag&PEOPLE_KILLER)
            for(i=0;i<MAX_PEOPLE;i++) 
            if(inrooms[myroom].peoples[i].style!=-1)
            {
                if(inrooms[myroom].peoples[i].flag&PEOPLE_KILLER||
                    inrooms[myroom].peoples[i].flag&PEOPLE_SPECTATOR) {
                    send_msg(i, buf);
                }
            }
        }
        else {
            if(!(inrooms[myroom].peoples[me].flag&PEOPLE_SPECTATOR))
            send_msg(-1, buf);
        }
        end_change_inroom();
        kill_msg(-1);
    }

quitgame:
    start_change_inroom();
    me=mypos;
    if(inrooms[myroom].peoples[me].flag&PEOPLE_ROOMOP) {
        for(i=0;i<MAX_PEOPLE;i++)
        if(inrooms[myroom].peoples[i].style!=-1)
        if(i!=me) {
            send_msg(i, "�㱻����");
        }
        rooms[myroom].style = -1;
        end_change_inroom();
        for(i=0;i<MAX_PEOPLE;i++)
            if(inrooms[myroom].peoples[i].style!=-1)
            if(i!=me)
                kill_msg(i);
        goto quitgame2;
    }
    inrooms[myroom].peoples[me].style=-1;
    rooms[myroom].people--;
    end_change_inroom();

    if(inrooms[myroom].peoples[me].flag&PEOPLE_KILLER)
        sprintf(buf, "\033[34mɱ��%sǱ����...\033[0m", inrooms[myroom].peoples[me].id);
    else
        sprintf(buf, "\033[36m%s�뿪����\033[0m", inrooms[myroom].peoples[me].id);
    send_msg(-1, buf);
quitgame2:
    kicked=0;
    k_getdata(t_lines-1, 0, "�Ļر���ȫ����Ϣ��?[y/N]", buf3, 3, 1, 1);
    if(toupper(buf3[0])=='Y') {
        sprintf(buf, "bbstmpfs/tmp/%d.msg", rand());
        save_msgs(buf);
        sprintf(buf2, "\"%s\"��ɱ�˼�¼", roomname);
        mail_file(buf, currentuser->userid, buf2, currentuser->userid);
	unlink(buf);
    }
//    signal(SIGUSR1, talk_request);
}

static int room_list_refresh(struct _select_def *conf)
{
    clear();
    docmdtitle("[��Ϸ��ѡ��]",
              "  �˳�[\033[1;32m��\033[0;37m,\033[1;32me\033[0;37m] ����[\033[1;32mEnter\033[0;37m] ѡ��[\033[1;32m��\033[0;37m,\033[1;32m��\033[0;37m] ���[\033[1;32ma\033[0;37m] ����[\033[1;32mJ\033[0;37m] \033[m      ����: \033[31;1mbad@smth.org\033[m");
    move(2, 0);
    prints("\033[0;1;37;44m    %4s %-14s %-12s %4s %4s %6s %-20s\033[m", "���", "��Ϸ������", "������", "����", "���", "������", "����");
    clrtoeol();
    k_resetcolor();
    update_endline();
    return SHOW_CONTINUE;
}

static int room_list_show(struct _select_def *conf, int i)
{
    struct room_struct * r;
    int j = room_get(i-1);
    if(j!=-1) {
        r=rooms+j;
        prints("  %3d  %-14s %-12s %3d  %3d  %2s%2s%2s %-36s", i, r->name, r->creator, r->people, r->maxpeople, (r->flag&ROOM_LOCKED)?"��":"", (r->flag&ROOM_SECRET)?"��":"", (!(r->flag&ROOM_DENYSPEC))?"��":"", r->title);
    }
    return SHOW_CONTINUE;
}

static int room_list_select(struct _select_def *conf)
{
    struct room_struct * r, * r2;
    int i, j;
    char ans[4];

    i = room_get(conf->pos-1);
    if(i==-1) return SHOW_CONTINUE;
    r = rooms+i;
    j=find_room(r->name);
    if(j==-1) {
        move(0, 0);
        clrtoeol();
        prints(" �÷����ѱ�����!");
        refresh(); sleep(1);
        return SHOW_REFRESH;
    }
    r2 = rooms+j;
    if(r2->people>=r2->maxpeople&&!USERPERM(currentuser, PERM_SYSOP)) {
        move(0, 0);
        clrtoeol();
        prints(" �÷�����������");
        refresh(); sleep(1);
        return SHOW_REFRESH;
    }
    k_getdata(0, 0, "�Ƿ����Թ�����ݽ���?\033[m[y/N]", ans, 3, 1,  1);
    if(toupper(ans[0])=='Y'&&r2->flag&ROOM_DENYSPEC&&!USERPERM(currentuser, PERM_SYSOP)) {
        move(0, 0);
        clrtoeol();
        prints(" �÷���ܾ��Թ���");
        refresh(); sleep(1);
        return SHOW_REFRESH;
    }
    join_room(find_room(r2->name), toupper(ans[0])=='Y');
    return SHOW_DIRCHANGE;
}

static int room_list_k_getdata(struct _select_def *conf, int pos, int len)
{
    clear_room();
    conf->item_count = room_count();
    return SHOW_CONTINUE;
}

static int room_list_prekey(struct _select_def *conf, int *key)
{
    switch (*key) {
    case 'e':
    case 'q':
        *key = KEY_LEFT;
        break;
    case 'p':
    case 'k':
        *key = KEY_UP;
        break;
    case ' ':
    case 'N':
        *key = KEY_PGDN;
        break;
    case 'n':
    case 'j':
        *key = KEY_DOWN;
        break;
    }
    return SHOW_CONTINUE;
}

static int room_list_key(struct _select_def *conf, int key)
{
    struct room_struct r, *r2;
    int i,j;
    char name[40], ans[4];
    switch(key) {
    case 'a':
        strcpy(r.creator, currentuser->userid);
        k_getdata(0, 0, "������:", name, 13, 1, 1);
        if(!name[0]) return SHOW_REFRESH;
        if(name[0]==' '||name[strlen(name)-1]==' ') {
            move(0, 0);
            clrtoeol();
            prints(" ��������ͷ��β����Ϊ�ո�");
            refresh(); sleep(1);
            return SHOW_CONTINUE;
        }
        strcpy(r.name, name);
        r.style = 1;
        r.flag = 0;
        r.people = 0;
        r.maxpeople = 100;
        strcpy(r.title, "��ɱ��ɱ��ɱɱɱ");
        if(add_room(&r)==-1) {
            move(0, 0);
            clrtoeol();
            prints(" ��һ�����ֵķ�����!");
            refresh(); sleep(1);
            return SHOW_REFRESH;
        }
        join_room(find_room(r.name), 0);
        return SHOW_DIRCHANGE;
    case 'J':
        k_getdata(0, 0, "������:", name, 12, 1, 1);
        if(!name[0]) return SHOW_REFRESH;
        if((i=find_room(name))==-1) {
            move(0, 0);
            clrtoeol();
            prints(" û���ҵ��÷���!");
            refresh(); sleep(1);
            return SHOW_REFRESH;
        }
        r2 = rooms+i;
        if(r2->people>=r2->maxpeople&&!USERPERM(currentuser, PERM_SYSOP)) {
            move(0, 0);
            clrtoeol();
            prints(" �÷�����������");
            refresh(); sleep(1);
            return SHOW_REFRESH;
        }
        k_getdata(0, 0, "�Ƿ����Թ�����ݽ���?\033[m[y/N]", ans, 3, 1, 1);
        if(toupper(ans[0])=='Y'&&r2->flag&ROOM_DENYSPEC&&!USERPERM(currentuser, PERM_SYSOP)) {
            move(0, 0);
            clrtoeol();
            prints(" �÷���ܾ��Թ���");
            refresh(); sleep(1);
            return SHOW_REFRESH;
        }
        join_room(find_room(name), toupper(ans[0])=='Y');
        return SHOW_DIRCHANGE;
    case 'K':
        if(!USERPERM(currentuser, PERM_SYSOP)) return SHOW_CONTINUE;
        i = room_get(conf->pos-1);
        if(i!=-1) {
            r2 = rooms+i;
            r2->style = -1;
            for(j=0;j<MAX_PEOPLE;j++)
            if(inrooms[i].peoples[j].style!=-1)
                kill(inrooms[i].peoples[j].pid, SIGUSR1);
        }
        return SHOW_DIRCHANGE;
    }
    return SHOW_CONTINUE;
}
/*
void show_top_board()
{
    FILE* fp;
    char buf[80];
    int i,j,x,y;
    clear();
    for(i=1;i<=6;i++) {
        sprintf(buf, "etc/killer.%d", i);
        fp=fopen(buf, "r");
        if(!fp) return;
        for(j=0;j<7;j++) {
            if(feof(fp)) break;
            y=(i-1)%3*8+j; x=(i-1)/3*40;
            if(fgets(buf, 80, fp)==NULL) break;
            move(y, x);
            k_resetcolor();
            if(j==2) k_setfcolor(RED, 1);
            prints("%s", buf);
        }
        fclose(fp);
    }
    pressanykey();
}
*/

int choose_room()
{
    struct _select_def grouplist_conf;
    int i;
    POINT *pts;

    clear_room();
    bzero(&grouplist_conf, sizeof(struct _select_def));
    grouplist_conf.item_count = room_count();
    if (grouplist_conf.item_count == 0) {
        grouplist_conf.item_count = 1;
    }
    pts = (POINT *) malloc(sizeof(POINT) * BBS_PAGESIZE);
    for (i = 0; i < BBS_PAGESIZE; i++) {
        pts[i].x = 2;
        pts[i].y = i + 3;
    }
    grouplist_conf.item_per_page = BBS_PAGESIZE;
    grouplist_conf.flag = LF_VSCROLL | LF_BELL | LF_LOOP | LF_MULTIPAGE;
    grouplist_conf.prompt = "��";
    grouplist_conf.item_pos = pts;
    grouplist_conf.arg = NULL;
    grouplist_conf.title_pos.x = 0;
    grouplist_conf.title_pos.y = 0;
    grouplist_conf.pos = 1;     /* initialize cursor on the first mailgroup */
    grouplist_conf.page_pos = 1;        /* initialize page to the first one */

    grouplist_conf.on_select = room_list_select;
    grouplist_conf.show_data = room_list_show;
    grouplist_conf.pre_key_command = room_list_prekey;
    grouplist_conf.key_command = room_list_key;
    grouplist_conf.show_title = room_list_refresh;
    grouplist_conf.get_data = room_list_k_getdata;
    //show_top_board();
    list_select_loop(&grouplist_conf);
    free(pts);
    return 0;
}

void* killer_get_shm(key_t shmkey, size_t shmsize, int *create) {
	int shmid;
	void *shmptr = (void*)-1;

	shmid = shmget(shmkey, shmsize, 0);
	if(shmid<0) {
		shmid = shmget(shmkey, shmsize, IPC_CREAT | 0644);
		*create = 1;
		shmptr = shmat(shmid, NULL, 0);
		memset(shmptr, 0, shmsize);
	} else {
		*create = 0;
		shmptr = shmat(shmid, NULL, 0);
	}
	return shmptr;
}


int killer_main()
{
    int i,oldmode;
    void *shm, *shm2;

    shm = killer_get_shm(getBBSKey(GAMEROOM_SHM), sizeof(struct room_struct)*MAX_ROOM, &i);
    rooms = shm;
    if(i) {
        for(i=0;i<MAX_ROOM;i++) {
            rooms[i].style=-1;
            rooms[i].w = 0;
            rooms[i].level = 0;
        }
    }
    shm2=killer_get_shm(getBBSKey(KILLER_SHM), sizeof(struct inroom_struct)*MAX_ROOM, &i);
    inrooms = shm2;
    if(i) {
        for(i=0;i<MAX_ROOM;i++)
            inrooms[i].w = 0;
    }
    oldmode = uinfo.mode;
    modify_user_mode(KILLER);
    choose_room();
    modify_user_mode(oldmode);
    return shmdt(shm) | shmdt(shm2);
}
