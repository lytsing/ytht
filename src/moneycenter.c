#include <sys/mman.h>		// for mmap
#include "bbs.h"
#include "bbstelnet.h"

#define MONEYCENTER_VERSION         1
#define DIR_MC          MY_BBS_HOME "/etc/moneyCenter/"
#define LOGFILE		DIR_MC "log"			//����ϵͳ��¼ add by yxk
#define DIR_STOCK MY_BBS_HOME "/etc/moneyCenter/stock/"
#define DIR_MC_TEMP     MY_BBS_HOME "/bbstmpfs/tmp/"
#define MC_BOSS_FILE    DIR_MC "mc_boss"
// �ؿ���ÿ�����͵ĺؿ���һ����Ŀ¼, �ؿ���ȡ{1, 2, ... , n} 
#define DIR_SHOP        DIR_MC "/cardshop/"
//���ֽ������ 
#define PRIZE_PER       1000000	// �̶�����
#define MAX_POOL_MONEY  5000000	// ��󽱽�
#define MAX_MONEY_NUM 300000000
#define STOCK_NUM            10	// ���������й�Ʊ��

#define MONEY_NAME "�����"	//�磺"��Ϳ��"
#define CENTER_NAME MY_BBS_NAME	//�磺"һ����Ϳ"
#define ROBUNION	DIR_MC "robmember"		//�ڰ��Ա���� add by yxk
#define BEGGAR		DIR_MC "begmember"		//ؤ���Ա���� add by yxk
#define POLICE		DIR_MC "policemen"		//�����Ա����
#define CRIMINAL	DIR_MC "criminals_list"		//ͨ�������� add by yxk
#define BADID		DIR_MC "bannedID"		//��ֹ������� add by yxk
#define MAX_ONLINE 300		//��������¼�����
#define BRICK_PRICE 200		//שͷ����
#define PAYDAY 5		//����������
#define LISTNUM 60		//���а���¼��Ŀ
//#define MONKEY_UNIT 250		//��һ��Monkey_business����Ҫ��Ǯ add by yxk

struct LotteryRecord {		//��Ʊ��ע��¼ 
	char userid[IDLEN + 1];
	int betCount[5];
};

struct myStockUnit {		//���˳��й�Ʊ��¼��Ԫ 
	time_t timeStamp;
	short stockid, status;
	int num;
};

struct BoardStock {
	char boardname[24];
	time_t timeStamp;
	int totalStockNum;
	int remainNum;
	int tradeNum;
	float weekPrice[7];
	float todayPrice[4];
	float high, low;
	short sellerNum, buyerNum;
	int status;
	int holderNum;
	int boardscore;
	int unused[5];
};				// 128 bytes per board's stock record 

struct TradeRecord {
	char tradeType, stockid, invalid, temp;
	char userid[IDLEN + 1], nouse[3];
	float price;
	int num;
	int unused;
};				// 32 bytes, ���׵� 

struct mcUserInfo {
	char version, mutex, temp[2];
	int cash, credit, loan, interest;
	short robExp, policeExp, begExp, guard;	//robExp ��ʶ begExp �� guard �ǹ� 
	time_t aliveTime, freeTime;
	time_t loanTime, backTime, depositTime;
	time_t lastSalary, lastActiveTime;
	struct myStockUnit stock[STOCK_NUM];
	int soldExp;
	int health, luck;	//helath ���� luck ��Ʒ 
	int Actived;		//�ж�����������¼���������֮һ 
	int safemoney;
	time_t secureTime;
	time_t insuranceTime;
	int antiban;
	int unused[12];
};				// 256 bytes �������� 

struct MC_Env {
	char version, closed, stockOpen, stockTime;
	unsigned char transferRate, depositRate, loanRate, xRate;
	int prize777, prize367, prizeSoccer;
	time_t start367, end367;
	time_t soccerStart, soccerEnd;
	time_t salaryStart, salaryEnd;
	time_t lastUpdateStock;
	int stockPoint[7];
	int tradeNum[7];
	long long Treasury;	// add by yxk
	time_t openTime;
	long long AllUserMoney;	// add by yxk
	int userNum;		// add by yxk
	int unused[29];
};				// 256 bytes moneycenter environment 

struct MC_Env *mcEnv;
struct mcUserInfo *myInfo;

#define MIN_XY(x, y) ((x) > (y) ? (y) : (x))
#define MAX_XY(x, y) ((x) > (y) ? (x) : (y))

static void moneycenter_welcome(void);
static void moneycenter_byebye(void);
static int initData(int type, char *filepath);
static void *loadData(char *filepath, void *buffer, size_t filesize);
static void saveData(void *buffer, size_t filesize);
static void money_show_stat(char *position);
static void nomoney_show_stat(char *position);
static int money_bank(void);
static int money_lottery(void);
static int money_shop(void);
static int money_robber(void);
static int money_stock(void);
static int money_gamble(void);
static int money_dice(void);
static int money_777(void);
static int money_admin();
static int money_cop();
static int money_beggar();
static int check_allow_in();
static int money_hall();
static int getOkUser(char *msg, char *uident, int line, int col);
static int userInputValue(int line, int col, char *act, char *name, int inf,
			  int sup);
static void showAt(int line, int col, char *str, int wait);
static int newStock(void *stockmem, int n);
static void deleteList(short stockid);
static int updateStockEnv(void *stockMem, int n, int flag);
static void whoTakeCharge(int pos, char *boss);
//static int ismaster(char *uid); 
static void randomevent(void);				//add by koyu
static void update_health(void);			//add by koyu
static void show_top(void);				//add by yxk
static int game_prize(void);				//add by yxk
static void choujiang(char *recfile, char *gametype);	//add by yxk
static void check_top(void);				//add by yxk
static int after_tax(int income);			//add by yxk
static int money_big(void);
static void banID(void);				//add by yxk
void mcuZero(char *uident);				//add by yxk
static void antiban(void);				//add by yxk
static void forcetax(void);				//add by yxk
static void all_user_money(void);			//add by yxk
static void show_info(void);				//add by yxk
static void quitsociety(int type);			//add by yxk
static int secureMoney(int type);			//add by yxk
static void mcLog(char *action, long long num, char *object);	//add by yxk
static void newSalary(void);
static void monkey_business(void);			//add by yxk
static void policeCheck(void);				//add by yxk
static int makeSalary(void);
static void sendSalary(void);

int
moneycenter()
{
	int ch = 9, quit = 0, retv;
	void *buffer_env = NULL, *buffer_me = NULL;
	char filepath[256];

	modify_user_mode(MONEY);
	strcpy(currboard, "millionaire");	// for deliverreport() 
	moneycenter_welcome();
// ����ȫ�ֲ������ҵ����� 
	sprintf(filepath, "%s", DIR_MC "mc.env");
	if (!file_exist(filepath))
		initData(0, filepath);
	mcEnv = loadData(filepath, buffer_env, sizeof (struct MC_Env));
	if (mcEnv == (void *) -1)
		return -1;
	newSalary();
	sethomefile(filepath, currentuser->userid, "mc.save");
	if (!file_exist(filepath))
		initData(1, filepath);
	myInfo = loadData(filepath, buffer_me, sizeof (struct mcUserInfo));
	if (myInfo == (void *) -1)
		goto MCENV;
// ����Ƿ���Խ��� 
	retv = check_allow_in();
	mcLog("�����������", retv, "");
	if (retv == 0)
		goto UNMAP;
	if (retv == -1)
		goto MUTEX;
//myInfo->aliveTime = time(NULL); //�Զ���������ת�Ƶ��ӽ�վ��ʼ main.c 

	if (makeSalary() > 0 &&
		time(NULL) > mcEnv->salaryStart
		&& myInfo->lastSalary < mcEnv->salaryStart)
		sendSalary();
	
	while (!quit) {
		check_top(); //check top 10 Rich, Rob, Beg; add by yxk
		forcetax(); //rich man pay tax; add by yxk
		if (!(random() % (MAX_ONLINE/50)) && myInfo->Actived > MAX_ONLINE/5 && (ch >= '0')
		    && (ch <= '8')) {
			clear();
			move(4, 4);
			prints("ͻȻ֮�䣬����������ҵ���ת����������");
			myInfo->Actived = 0;
			pressanykey();
			randomevent();
		}
		clear();
		nomoney_show_stat("ʮ��·��");
		move(6, 0);
		prints("\033[33;41m " MY_BBS_NAME " \033[m\n\n");
		prints
		    ("\033[1;33m        �x�x�x ������         �x�x�u          �x�x�u\n"
		     "\033[1;33m          ��  ��������  �x�x  �� | ��   �x�x  �� | ��    \033[31m  �x�x�x�x�x\033[m\n"
		     "\033[1;33m          ��  �� �� ��  �� �� �� | ��   �� �� �� | ��    \033[31m���������稇\033[m\n"
		     "\033[1;33m        ���� �x�u ��    �����x�� �񨐨� �����x�� �񨐨�  \033[31m  ����������\033[m");
		move(t_lines - 2, 0);
		prints
		    ("\033[1;44m ��Ҫȥ \033[1;46m [0]������� [1]�������� [2]��Ʊ���� [3]�ٱ���� [4]����\033[m");
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m        \033[1;46m [5]�����ĳ� [6]�ڰ��ܲ� [7]ؤ������ [8]�ٻ���˾ [Q]�뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '0':
			money_hall();
			break;
		case '1':
			money_bank();
			break;
		case '2':
			money_lottery();
			break;
		case '3':
			money_stock();
			break;
		case '4':
			money_cop();
			break;
		case '5':
			money_gamble();
			break;
		case '6':
			money_robber();
			break;
		case '7':
			money_beggar();
			break;
		case '8':
			money_shop();
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
	}
	check_top();
      UNMAP:myInfo->mutex = 0;
      MUTEX:saveData(myInfo, sizeof (struct mcUserInfo));
      MCENV:saveData(mcEnv, sizeof (struct MC_Env));
	moneycenter_byebye();
	sleep(1);
	return 0;
}

static void
forcetax()
{
	int tax_paid, no_tax_money;
	no_tax_money = 1000000 + countexp(currentuser) * 1000;
        if (myInfo->cash > no_tax_money && !(random() % (MAX_ONLINE / 2))) {
		tax_paid = myInfo->cash - no_tax_money -
			   after_tax(myInfo->cash - no_tax_money);
		myInfo->cash -= tax_paid;
		clear();
		move(4, 4);
		if(askyn("�����ֽ���࣬��Ҫ��˰����Ҫ����", YEA, NA) == NA) {
			prints("\n    ��͵˰©˰��������%d������Ƿ˰��������30���ӣ�", tax_paid);
			myInfo->freeTime = time(NULL) + 1800;
			myInfo->mutex = 0;
			saveData(myInfo, sizeof (struct mcUserInfo));
			saveData(mcEnv, sizeof (struct MC_Env));
			pressreturn();
			Q_Goodbye();
		}
		//myInfo->health = 100;
		myInfo->robExp += tax_paid / 110000;
		myInfo->begExp += tax_paid / 110000;
		myInfo->luck += tax_paid / 110000;
		move(6, 4);
		prints("������%d��˰����ʶ��������Ʒ������%d�㣡", 
				tax_paid, tax_paid / 110000);
		pressanykey();
	}
	if (myInfo->credit > no_tax_money && !(random() % (MAX_ONLINE / 2))) {
		tax_paid = myInfo->credit - no_tax_money -
			   after_tax(myInfo->credit - no_tax_money);
		myInfo->credit -= tax_paid;
		clear();
		move(4, 4);
		if(askyn("���Ĵ����࣬��Ҫ��˰����Ҫ����", YEA, NA) == NA) {
			prints("\n    ��͵˰©˰��������%d������Ƿ˰��������30���ӣ�", tax_paid);
			myInfo->freeTime = time(NULL) + 1800;
			myInfo->mutex = 0;                                                                                  
			saveData(myInfo, sizeof (struct mcUserInfo));
			saveData(mcEnv, sizeof (struct MC_Env));
			pressreturn();
			Q_Goodbye();                                                                                       
		}
		//myInfo->health = 100;
		myInfo->robExp += tax_paid / 110000;
		myInfo->begExp += tax_paid / 110000;
		myInfo->luck += tax_paid / 110000;
		move(6, 4);
		prints("������%d��˰����ʶ��������Ʒ������%d�㣡", 
				tax_paid, tax_paid / 110000);
		pressanykey();
	}
	return;
}

/*   �������� add by koyu */
static void
update_health()
{
	time_t curtime;
	int add_health;

	curtime = time(NULL);
	add_health = (curtime - myInfo->aliveTime) / 20;	//ÿ20��������1 
	if (myInfo->health < 0)
		myInfo->health = 0;
	myInfo->health = MIN_XY((myInfo->health + add_health), 100);
	myInfo->aliveTime += add_health * 20;

//��Ʒ[-100, 100]
	if (myInfo->luck > 100)
		myInfo->luck = 100;
	if (myInfo->luck < -100)
		myInfo->luck = -100;
//��ʶ������30000. add by yxk
	if (myInfo->robExp > 30000 || myInfo->robExp < -10000)
		myInfo->robExp = 30000;
	if (myInfo->begExp > 30000 || myInfo->begExp < -10000)
		myInfo->begExp = 30000;
//��Ǯ����20��. add by yxk
	if (myInfo->cash > 2000000000) {
		mcEnv->Treasury += myInfo->cash - 2000000000;
		myInfo->cash = 2000000000;
	}
	if (myInfo->cash < -1000000000) {
		mcEnv->Treasury += 294967296 + myInfo->cash;
		mcEnv->Treasury += 2000000000;
		myInfo->cash = 2000000000;
	}
	if (myInfo->credit > 2000000000) {
		mcEnv->Treasury += myInfo->credit - 2000000000;
		myInfo->credit = 2000000000;
	}
	if (myInfo->credit < -1000000000) {
		mcEnv->Treasury += 294967296 + myInfo->credit;
		mcEnv->Treasury += 2000000000;
		myInfo->credit = 2000000000;
	}
	
	return;
}

/*  ����ж���Ҫ������������������0��������ʾ��Ϣ����1  add by koyu */
static int
check_health(int need_health, int x, int y, char *str, int wait)
{
	if (myInfo->health >= need_health)
		return 0;
	else {
		showAt(x, y, str, wait);
		return 1;
	}
}

static void
showAt(int line, int col, char *str, int wait)
{
	move(line, col);
	prints("%s", str);
	if (wait)
		pressanykey();
}

static void
moneycenter_welcome()
{
	char buf[256];
	
	clear();
	move(6, 0);
	prints("              \033[1;32m����һ��ש���ɵ����磡\n\n");

	prints("              \033[1;32m����һ������ð�յ����磡\033[m\n\n");

	prints("              \033[1;31m������������棡\033[m\n\n");

	prints("              \033[1;31m�����ǿ��Ϊ����\033[m\n\n");

	prints
	    ("              \033[1;35m���������ѡ�Ǯ��Ϊ����֮�� ʤ������ҳ���\033[m\n\n\n");

	prints
	    ("                    \033[0;1;33m��%s 2005�������绶ӭ����  \033[0m",
	     CENTER_NAME);

	pressanykey();
	
	clear();
	move(2, 0);
	sprintf(buf, "vote/millionaire/notes");
	if (file_isfile(buf))
		ansimore2stuff(buf, NA, 2, 24);
	pressanykey();
}

static void
check_top()
{
	int n, n2, topMoney[LISTNUM], tmpM;
	short topRob[LISTNUM], topBeg[LISTNUM], tmpR, tmpB;
	char topIDM[LISTNUM][20], topIDR[LISTNUM][20], topIDB[LISTNUM][20], tmpID[20];
	FILE *fp, *fpnew;
	
	//��ʼ��
	for (n = 0; n < LISTNUM; n++){
		strcpy(topIDM[n], "null.");
		strcpy(topIDR[n], "null.");
		strcpy(topIDB[n], "null.");
		topMoney[n] = 0;
		topRob[n] = 0;
		topBeg[n] = 0;
	}
	
	tmpM = myInfo->cash + myInfo->credit + myInfo->interest + myInfo->safemoney;
	tmpR = myInfo->robExp;
	tmpB = myInfo->begExp;
	strcpy(tmpID, currentuser->userid);
	
	//��¼�ļ�������
	if ((fp = fopen(DIR_MC "top", "r")) == NULL){
                fpnew = fopen(DIR_MC "top.new", "w");
		flock(fileno(fpnew), LOCK_EX);
		
		strcpy(topIDM[0], tmpID);
		topMoney[0] = tmpM;
		for (n = 0; n < LISTNUM; n++)
			fprintf(fpnew, "%s %d\n", topIDM[n], topMoney[n]);
		
		strcpy(topIDR[0], tmpID);
		topRob[0] = tmpR;
		for (n = 0; n < LISTNUM; n++)
			fprintf(fpnew, "%s %d\n", topIDR[n], topRob[n]);
		
		strcpy(topIDB[0], tmpID);
		topBeg[0] = tmpB;
		for (n = 0; n < LISTNUM; n++)
			fprintf(fpnew, "%s %d\n", topIDB[n], topBeg[n]);
		
		flock(fileno(fpnew), LOCK_UN);
		fclose(fpnew);
		
		if ((fp=fopen(DIR_MC "top", "r")) == NULL)
			rename(DIR_MC "top.new", DIR_MC "top");
		else {
			fclose(fp);
			deliverreport("ϵͳ����",
					"\033[1;31m������¼�ļ��Ѿ����ڣ�\033[m\n");
		}
		return;
	}
	
	//��¼�ļ�����
	flock(fileno(fp), LOCK_EX);
	//����¼
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %d\n", topIDM[n], &topMoney[n]);
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %hd\n", topIDR[n], &topRob[n]);
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %hd\n", topIDB[n], &topBeg[n]);
	//ID���ϰ�
	for (n = 0; n < LISTNUM; n++) {
		if (!strcmp(topIDM[n], tmpID)){
			topMoney[n] = tmpM;
			tmpM = -1;
		}
		if (!strcmp(topIDR[n], tmpID)){
			topRob[n] = tmpR;
			tmpR = -1;
		}
		if (!strcmp(topIDB[n], tmpID)){
			topBeg[n] = tmpB;
			tmpB = -1;
		}
	}
	//IDδ�ϰ�
	if (tmpM > topMoney[LISTNUM - 1]) {
		strcpy(topIDM[LISTNUM - 1], tmpID);
		topMoney[LISTNUM - 1] = tmpM;
	}
	if (tmpR > topRob[LISTNUM - 1]) {
		strcpy(topIDR[LISTNUM - 1], tmpID);
		topRob[LISTNUM - 1] = tmpR;
	}
	if (tmpB > topBeg[LISTNUM - 1]) {
		strcpy(topIDB[LISTNUM - 1], tmpID);
		topBeg[LISTNUM - 1] = tmpB;
	}
	//����	
	tmpM = myInfo->cash + myInfo->credit + myInfo->interest + myInfo->safemoney;
	tmpR = myInfo->robExp;
	tmpB = myInfo->begExp;
	for (n = 0; n< LISTNUM - 1; n++)
		for (n2 = n+1; n2 < LISTNUM; n2++){
			if (topMoney[n] < topMoney[n2]){
				strcpy(tmpID, topIDM[n]);
				strcpy(topIDM[n], topIDM[n2]);
				strcpy(topIDM[n2], tmpID);
				tmpM = topMoney[n];
				topMoney[n] = topMoney[n2];
				topMoney[n2] = tmpM;
			}
			if (topRob[n] < topRob[n2]){
				strcpy(tmpID, topIDR[n]);
				strcpy(topIDR[n], topIDR[n2]);
				strcpy(topIDR[n2], tmpID);
				tmpR = topRob[n];
				topRob[n] = topRob[n2];
				topRob[n2] = tmpR;
			}
			if (topBeg[n] < topBeg[n2]){
				strcpy(tmpID, topIDB[n]);
				strcpy(topIDB[n], topIDB[n2]);
				strcpy(topIDB[n2], tmpID);
				tmpB = topBeg[n];
				topBeg[n] = topBeg[n2];
				topBeg[n2] = tmpB;
			}
		}
	//д�����ļ�
	fpnew = fopen(DIR_MC "top.new", "w");
	flock(fileno(fpnew), LOCK_EX);
	
	for (n = 0; n < LISTNUM; n++)
		fprintf(fpnew, "%s %d\n", topIDM[n], topMoney[n]);
	for (n = 0; n < LISTNUM; n++)
		fprintf(fpnew, "%s %d\n", topIDR[n], topRob[n]);
	for (n = 0; n < LISTNUM; n++)
		fprintf(fpnew, "%s %d\n", topIDB[n], topBeg[n]);
	
	flock(fileno(fpnew), LOCK_UN);
	fclose(fpnew);
	
	flock(fileno(fp), LOCK_UN);
	fclose(fp);
	
	rename(DIR_MC "top.new", DIR_MC "top");
	return;
}

static void
moneycenter_byebye()
{
	clear();
	showAt(10, 14, "\033[1;32m�ڴ����ٴι��ٴ������磬"
	       "����ʵ���������롣\033[m", YEA);
	mcLog("�˳�����", 0, "");
}

static void
show_top()
{
	int m, n, topMoney[LISTNUM], screen_num;
	short topRob[LISTNUM], topBeg[LISTNUM];
	char topIDM[LISTNUM][20], topIDR[LISTNUM][20], topIDB[LISTNUM][20];
	FILE *fp;
	
	fp = fopen(DIR_MC "top", "r");
	flock(fileno(fp), LOCK_EX);
	
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %d\n", topIDM[n], &topMoney[n]);
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %hd\n", topIDR[n], &topRob[n]);
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %hd\n", topIDB[n], &topBeg[n]);
	
	flock(fileno(fp), LOCK_UN);
	fclose(fp);
	
	screen_num = (LISTNUM + 1) / 20;
	for (m = 0; m <= screen_num -1; m++) {
		clear();
		prints("\033[1;44;37m  " MY_BBS_NAME " BBS    \033[32m--== ��  ��  �� ==-- "
		       "\033[33m--== ��  ʶ  �� ==-- \033[36m--== ��  ��  �� ==--\033[m\r\n");
		prints("\033[1;41;37m   ����           ����       ���ʲ�         ����    "
				"��ʶ         ����    �� \033[m\r\n");
	
		for (n = 0; n < 20; n++)
			prints("\033[1;37m%6d\033[32m%18s\033[33m%11d\033[32m%15s"
				"\033[33m%6d\033[32m%15s\033[33m%6d\033[m\r\n", 
				m*20+n+1, topIDM[m*20+n], topMoney[m*20+n],
				topIDR[m*20+n], topRob[m*20+n], topIDB[m*20+n], topBeg[m*20+n]);
		prints("\033[1;41;33m                    ���ǵ�%2d������������鿴��һ��"
			"                            \033[m\r\n", m+1);
		pressanykey();
	}
}

static int
getOkUser(char *msg, char *uident, int line, int col)
{				// ���������Чid�ŵ�uident��, �ɹ�����1, ���򷵻�0 

	move(line, col);
	usercomplete(msg, uident);
	if (uident[0] == '\0')
		return 0;
	if (!getuser(uident, NULL)) {
		showAt(line + 1, col, "�����ʹ���ߴ���...", YEA);
		return 0;
	}
	return 1;
}

static int
initData(int type, char *filepath)
{
	int fd;
	struct MC_Env mce;
	struct mcUserInfo ui;
	struct BoardStock bs;
	void *ptr = NULL;
	size_t filesize = 0;

	if (type == 0) {	//��ʼ��ȫ������ 
		filesize = sizeof (struct MC_Env);
		bzero(&mce, filesize);
		mce.version = MONEYCENTER_VERSION;
		mce.transferRate = 100;
		mce.depositRate = 100;
		mce.loanRate = 100;
		ptr = &mce;
		mce.Treasury = 10000000;
		mce.AllUserMoney = 0;
		mce.userNum = 0;
		mkdir(DIR_MC, 0770);
	} else if (type == 1) {	//��ʼ���������� 
		filesize = sizeof (struct mcUserInfo);
		bzero(&ui, filesize);
		ui.version = MONEYCENTER_VERSION;
		ptr = &ui;
		ui.credit = 50000;
		mcEnv->Treasury -= 50000;
		addtofile(DIR_MC "mc_user", filepath);
	} else {		//��ʼ������ 
		filesize = sizeof (struct BoardStock);
		bzero(&bs, filesize);
		ptr = &bs;
		mkdir(DIR_STOCK, 0770);
	}
	if ((fd = open(filepath, O_CREAT | O_EXCL | O_WRONLY, 0660)) == -1)
		return -1;
	write(fd, ptr, filesize);
	close(fd);
	return 0;
}

static void *
loadData(char *filepath, void *buffer, size_t filesize)
{
	int fd;

	if ((fd = open(filepath, O_RDWR, 0660)) == -1)
		return (void *) -1;
	buffer = mmap(0, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	return buffer;
}

static void
saveData(void *buffer, size_t filesize)
{
	if (buffer != NULL)
		munmap(buffer, filesize);
}

static void
whoTakeCharge(int pos, char *boss)
{
	const char feaStr[][20] =
	    { "admin", "bank", "lottery", "gambling", "gang",
		"beggar", "stock", "shop", "police"
	};
	if (readstrvalue(MC_BOSS_FILE, feaStr[pos], boss, IDLEN + 1) != 0)
		*boss = '\0';
}

#if 0
static int
ismaster(char *uid)		//admin return -1, master return 1, none 0 
{
	int i, ret = 0;
	char boss[IDLEN + 1];

	for (i = 0; i <= 8; i++) {
		whoTakeCharge(i, boss);
		if (!strcmp(currentuser->userid, boss)) {
			if (i == 0)
				ret = -1;
			else
				ret = 1;
		}
	}

	return ret;
}
#endif

static int
userInputValue(int line, int col, char *act, char *name, int inf, int sup)
{
	char buf[STRLEN], content[STRLEN];
	int num;

	snprintf(content, STRLEN - 1, "%s����%s��[%d--%d]", act, name, inf,
		 sup);
	getdata(line, col, content, buf, 10, DOECHO, YEA);
	num = atoi(buf);
	num = MAX_XY(num, inf);
	num = MIN_XY(num, sup);
	move(line + 1, col);
	snprintf(content, STRLEN - 1, "ȷ��%s %d %s��", act, num, name);
	if (askyn(content, NA, NA) == NA)
		return -1;
	return num;
}

#define EVENT_NUM (sizeof(rd_event)/sizeof(rd_event[0]))

static void
randomevent()
{
	int num, total, rat;
	char title[STRLEN], buf[256];
	struct st_Event {
		char desc1[STRLEN], desc2[STRLEN], desc3[STRLEN];
		int type, bonus;	//type:1 �ֽ� 2 ��ʶ  3 �� 4 ���� 5 ���� 6 ��� etc 
	} rd_event[] = {	//ע������˳����ƷԽ�ã���������Ŀ�����Խ�� 
		{
		"��������" MY_BBS_NAME, "���в�������", "", 7, 5}, {
		"���" MY_BBS_NAME "��������", "�����", "�⳥��", 6, 2}, {
		"���������", "�õ���", "�Ĵ��", 6, 5}, {
		"����С����", "�õ���", "�Ľ���", 0, 5}, {
		"��ðȥУҽԺ", "�������������ٵ�", "��", 5, -20}, {
		"���Ӣ������Ͽ", "��ʶ������", "��", 2, 15}, {
		"���������ڵ�����ƨ", "�������ٵ�", "", 5, -40}, {
		"�õ��׽", "�������", "��", 3, 15}, {
		"�����԰׳�", "�������ٵ�", "", 5, -100}, {
		"���������FR jj", "��ʶ����", "��", 2, 10}, {
		"�������г�", "����", "���ֽ�", 1, -50000}, {
		"���������FR jj", "ѧϰ�赸���������", "��", 3, 10}, {
		"����������", "��Ʒ����", "��", 4, -5}, {
		"���������¸ұ��", "��ʶ����", "��", 2, 10}, {
		"���������¸ұ��", "Ϊ�˲ɹ����߻���", "���ֽ�", 1, -100000},{
		"�õ������", "�������", "��", 3, 10}, {
		"�ǵ��ڼ䵹����", "��Ʒ����", "��", 4, -10}, {
		"������Ϊ", "��ʶ����", "��", 2, 5}, {
		"͵��mmϴ��", "��Ʒ����", "��", 4, -10}, {
		"���ú�ѧϰ", "�ӿδ�����������", "��", 3, 5}, {
		"̰С�������˼ٻ�", "��ʧ", "���ֽ�", 1, -200000}, {
		"��ðȥУҽԺ", "��ʶ����", "��", 2, 5}, {
		"�����³�Passat", "����", "", 1, -200000}, {
		"�������г�", "�������", "��", 3, 5}, {
		"һ�Ķ�������", "��Ʒ����", "��", 4, -20}, {
		"Т����ĸ", "��Ʒ����", "��", 4, 20}, {
		"����ײ�ϵ��߸�", "��������", "��", 3, -5}, {
		"�ǵ��ڼ䵹����", "׬��", "����֮��", 1, 200000}, {
		"Υ�±�������ס", "��ʶ������", "��", 2, -5}, {
		"Э������ץס�ٷ�", "���", "����", 1, 200000}, {
		"�����蹷", "��ҧ�˺���������", "��", 3, -5}, {
		"�����ջ�", "��Ʒ����", "��", 4, 10}, {
		"���ø����ܱ�ץ����", "��ʶ������", "��", 2, -5}, {
		"Ӣ�۾���", "��Ʒ����", "��", 4, 10}, {
		"�ڹ������ϱ���ɧ��", "��ʶ������", "��", 2, -10}, {
		"��Ǯ��", "������", "͵͵�Ž����Լ��ڴ���", 1, 100000}, {
		"��С��å����", "������", "��", 3, -10}, {
		"ʰ����", "��Ʒ����", "��", 4, 5}, {
		"���ϻؼ�·�ϱ�����", "������", "��", 3, -10}, {
		"����Ʒ", "׬��", "", 1, 50000}, {
		"���ڰ���", "��ʶ������", "��", 2, -10}, {
		"����ǧ���˲�", "�������ӵ�", "", 5, 100}, {
		"������Ů�������", "��ʶ������", "��", 2, -15}, {
		"���´�����", "�������ӵ�", "", 5, 40}, {
		"�����ֳ�����ˤ��", "������", "��", 3, -15}, {
		"��ֶ�������", "�������ӵ�", "��", 5, 20}, {
		"����С˥��", "��ʧ��", "���ֽ�", 0, -5}, {
		"������˥��", "����һ��", "�Ĵ���", 6, -5}, {
		"��������" MY_BBS_NAME, "������", "�Ĵ��", 6, -2}, {
		"��ͼ������" MY_BBS_NAME, "ȫ��ָ���½�", "", 7, -5}
	};

	money_show_stat("��������");

	mcLog("������������", 0, "");
	move(4, 4);
	prints("��֪�����˶�ã����������˹����������Լ�����һ�����صĵط���\n"
	       "    ������֣�ͻȻ����һ����������˵��������ӭ�����������磡��\n"
	       "    ��ش���ȷһ������󣬾ͻ�����һ������¼���ף����ˣ�");
	pressreturn();
	money_show_stat("��������");
	if (check_health
	    (40, 12, 4,
	     "��Ŷ�������ڵ���������������ô�̼������飬�������˰ɡ�����", YEA))
		return;

	if (show_cake()) {
		prints("����Ǹ������˰�...\n");
		pressreturn();
		return;
	}

	clear();
	update_health();
	myInfo->health -= 10;
	money_show_stat("��������");

	rat = EVENT_NUM * myInfo->luck * 3 / 800;
	getrandomint(&num);
	num = num % (EVENT_NUM - abs(rat));
	if (rat < 0)
		num -= rat;

	num = abs(num) % EVENT_NUM;	//���漸������ʲô��������������ڰ�
	
	switch (rd_event[num].type) {
	case 0:		//��Ǯ�ٷֱ� 
		total =
		    MIN_XY(myInfo->cash / rd_event[num].bonus,
			   MAX_MONEY_NUM / 2);
		move(6, 4);
		if (total < 0)
			total = MAX_XY(total, -myInfo->cash);
		else
			total = MIN_XY(total, mcEnv->Treasury / 2);
		mcEnv->Treasury -= total;
		myInfo->cash += total;
		prints(total > 0 ? "\033[1;31m��%s��%s%d%s%s��\033[m" :
		       "\033[1;34m��%s��%s%d%s%s��\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), MONEY_NAME,
		       rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 100000)
			break;
		sprintf(title, "���¼���%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s��%s%d%s%s��\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			MONEY_NAME, rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 1:		//��Ǯ���� 
		if (myInfo->cash >= rd_event[num].bonus / 2)
			total = rd_event[num].bonus;
		else
			total = rd_event[num].bonus / 10;
		if (total < 0)
			total = MAX_XY(total, -myInfo->cash);
		else
			total = MIN_XY(total, mcEnv->Treasury / 2);
		mcEnv->Treasury -= total;
		move(6, 4);
		myInfo->cash += total;
                prints(total > 0 ? "\033[1;31m��%s��%s%d%s%s��\033[m" :
		       "\033[1;34m��%s��%s%d%s%s��\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), MONEY_NAME,
		       rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 100000)
			break;
		sprintf(title, "���¼���%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s��%s%d%s%s��\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			MONEY_NAME, rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 2:		//��ʶ 
		total = rd_event[num].bonus;
		move(6, 4);
		myInfo->robExp = MAX_XY(0, (myInfo->robExp + total));
                prints(total > 0 ? "\033[1;31m��%s��%s%d%s��\033[m" :
		       "\033[1;34m��%s��%s%d%s��\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 10)
			break;
		sprintf(title, "���¼���%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s��%s%d%s��\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 3:		//�� 
		total = rd_event[num].bonus;
		move(6, 4);
		myInfo->begExp = MAX_XY(0, (myInfo->begExp + total));
                prints(total > 0 ? "\033[1;31m��%s��%s%d%s��\033[m" :
		       "\033[1;34m��%s��%s%d%s��\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 10)
			break;
		sprintf(title, "���¼���%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s��%s%d%s��\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 4:		//��Ʒ 
		total = rd_event[num].bonus;
		move(6, 4);
		myInfo->luck = MAX_XY(-100, (myInfo->luck + total));
                prints(total > 0 ? "\033[1;31m��%s��%s%d%s��\033[m" :
		       "\033[1;34m��%s��%s%d%s��\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		//sprintf(title, "���¼���%s%s", currentuser->userid,
		//	rd_event[num].desc1);
		//sprintf(buf, "%s%s��%s%d%s��", currentuser->userid,
		//	rd_event[num].desc1, rd_event[num].desc2, abs(total),
		//	rd_event[num].desc3);
		break;
	case 5:		//���� 
		update_health();
		total = MIN_XY(100, rd_event[num].bonus + myInfo->health);
		move(6, 4);
		myInfo->health = MAX_XY(total, 0);
                prints(rd_event[num].bonus > 0 ? "\033[1;31m��%s��%s%d%s��\033[m" :
		       "\033[1;34m��%s��%s%d%s��\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, myInfo->health, rd_event[num].desc3);
		mcLog(rd_event[num].desc2, myInfo->health, rd_event[num].desc3);
		//sprintf(title, "���¼���%s%s", currentuser->userid,
		//	rd_event[num].desc1);
		//sprintf(buf, "%s%s��%s%d%s��", currentuser->userid,
		//	rd_event[num].desc1, rd_event[num].desc2,
		//	myInfo->health, rd_event[num].desc3);
		break;
	case 6:		//���ٷֱ� 
		if (rd_event[num].bonus < 0)
			//total = MAX_XY(-MAX_MONEY_NUM, myInfo->credit / rd_event[num].bonus);
			total = MAX_XY(-2000000, myInfo->credit / rd_event[num].bonus);
		else
			//total = MIN_XY(MAX_MONEY_NUM, myInfo->credit / rd_event[num].bonus);
			total = MIN_XY(2000000, myInfo->credit / rd_event[num].bonus);
		if (total < 0)
			total = MAX_XY(total, -myInfo->credit);
		else
			total = MIN_XY(total, mcEnv->Treasury / 2);
		mcEnv->Treasury -= total;
		move(6, 4);
		myInfo->credit += total;
                prints(total > 0 ? "\033[1;31m��%s��%s%d%s%s��\033[m" :
		       "\033[1;34m��%s��%s%d%s%s��\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), MONEY_NAME,
		       rd_event[num].desc3);
		if (rd_event[num].bonus <= 3) {
			myInfo->health = 0;
			move(7, 4);
			prints("��������ô����£���æ�������ˡ�");
		}
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 100000)
			break;
		sprintf(title, "���¼���%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s��%s%d%s%s��\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			MONEY_NAME, rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 7:		//ȫ��
		total = MIN_XY(myInfo->cash / rd_event[num].bonus, 2000000);
		total = MIN_XY(total, mcEnv->Treasury / 2);
		myInfo->cash += total;
		mcEnv->Treasury -= total;
		mcLog(rd_event[num].desc2, total, "�ֽ�");
		total = MIN_XY(myInfo->credit / rd_event[num].bonus, 2000000);
		total = MIN_XY(total, mcEnv->Treasury / 2);
		myInfo->credit += total;
		mcEnv->Treasury -= total;
		mcLog(rd_event[num].desc2, total, "���");
		total = MIN_XY(myInfo->robExp / rd_event[num].bonus, 30);
		myInfo->robExp += total;
		mcLog(rd_event[num].desc2, total, "��ʶ");
		total = MIN_XY(myInfo->begExp / rd_event[num].bonus, 30);
		myInfo->begExp += total;
		mcLog(rd_event[num].desc2, total, "��");
		total = abs(myInfo->luck) / rd_event[num].bonus;
		myInfo->luck += total;
		mcLog(rd_event[num].desc2, total, "��Ʒ");
		myInfo->health = 0;
		move(6, 4);
                prints(total > 0 ? "\033[1;31m��%s��%s��\033[m" :
		       "\033[1;34m��%s��%s��\033[m", rd_event[num].desc1,
		       rd_event[num].desc2);
		prints("\n    ������ô����£���æ�������ˡ�");
		sprintf(title, "���¼���%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s��%s��\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2);
		deliverreport(title, buf);
		break;
	}
	pressanykey();
	sleep(1);
	return;

}

static int
makeInterest(int basicMoney, time_t lastTime, float rate)
{				// ������Ϣ 
	int calHour, Interest;
	time_t currTime = time(NULL);

	if (lastTime > 0 && currTime > lastTime) {
		calHour = (currTime - lastTime) / 3600;
		calHour = MIN_XY(calHour, 87600);
		Interest = basicMoney * rate * calHour / 24;
		if(Interest > 0)
			mcLog("���", Interest, "��Ϣ");
		return Interest;
	}
	return 0;
}

static void
money_show_stat(char *position)
{
	clear();
	if (chkmail()) {
		move(0, 30);
		prints("\033[1;5;36m[�����ż�]\033[m");
	}
	if (time(NULL) < myInfo->freeTime) {
		move(12, 4);
		prints("    �������ϴ������·��ˣ��㱻����ץ���ˣ�");
		myInfo->mutex = 0;
		saveData(myInfo, sizeof (struct mcUserInfo));
		saveData(mcEnv, sizeof (struct MC_Env));
		pressreturn();
		Q_Goodbye();
		mcLog("������ץ��", 0, "");
	}

	move(1, 0);
	update_health();
	prints
	    ("���Ĵ��ţ�\033[1;33m%s\033[m      ��ʶ \033[1;33m%d\033[m  �� \033[1;33m%d\033[m  ��Ʒ \033[1;33m%d\033[m  ���� \033[1;33m%d\033[m \n",
	     currentuser->userid, myInfo->robExp, myInfo->begExp, myInfo->luck,
	     myInfo->health);
	prints("�����ϴ��� \033[1;31m%d\033[m %s��", myInfo->cash, MONEY_NAME);
	prints("��� \033[1;31m%d\033[m %s����ǰλ�� \033[1;33m%s\033[m",
	       myInfo->credit, MONEY_NAME, position);
	move(3, 0);
	prints
	    ("\033[1m--------------------------------------------------------------------------------\033[m");
}

static void
nomoney_show_stat(char *position)
{
	clear();
	if (chkmail()) {
		move(0, 30);
		prints("\033[1;5;36m[�����ż�]\033[m");
	}
	if (time(NULL) < myInfo->freeTime) {
		move(12, 4);
		prints("    �������ϴ������·��ˣ��㱻����ץ���ˣ�");
		myInfo->mutex = 0;
		saveData(myInfo, sizeof (struct mcUserInfo));
		saveData(mcEnv, sizeof (struct MC_Env));
		pressreturn();
		mcLog("������ץ��", 0, "");
		Q_Goodbye();
	}
	update_health();
	if (myInfo->luck > 100)
		myInfo->luck = 100;
	if (myInfo->luck < -100)
		myInfo->luck = -100;
	move(1, 0);
	prints
	    ("���Ĵ��ţ�\033[1;33m%s\033[m      ��ʶ \033[1;33m%d\033[m  "
	     "�� \033[1;33m%d\033[m  ��Ʒ \033[1;33m%d\033[m  ���� \033[1;33m%d\033[m \n",
	     currentuser->userid, myInfo->robExp, myInfo->begExp, myInfo->luck,
	     myInfo->health);
	prints
	    ("\033[1;32m��ӭ����%s�������磬��ǰλ����\033[0m \033[1;33m%s\033[0m",
	     CENTER_NAME, position);
	move(3, 0);
	prints
	    ("\033[1m--------------------------------------------------------------------------------\033[m");
}

static int
check_allow_in()
{
	time_t currTime = time(NULL);
	char uident[IDLEN+1];
	int day, hour, minute, num;

	clear();
	move(10, 10);
	if (mcEnv->closed) {	/* ��������ر� */
		showAt(10, 10, "��������ر���...���Ժ�����", YEA);
		whoTakeCharge(0, uident);
		if (!USERPERM(currentuser, PERM_SYSOP) && 
			strcmp(currentuser->userid, uident))
			return 0;
		move(12, 10);
		if(askyn("����Ҫ�������������й��������", NA, NA) == YEA)
			money_admin();
		return 0;
	}
	if (mcEnv->openTime > currentuser->lastlogin) {
		showAt(10, 4, "�����޸��˴��룬����Ҫ�˳����д��������µ�¼����������������", YEA);
		return 0;
	}
		
	if (myInfo->mutex++ && count_uindex_telnet(usernum) > 1) {	// ����ര��, ͬʱ������� 
		showAt(10, 10, "���Ѿ��ڴ�����������!", YEA);
		return -1;
	}
/* ���ﱻ��� */
	clrtoeol();
	if (currTime < myInfo->freeTime) {
		day = (myInfo->freeTime - currTime) / 86400;
		hour = (myInfo->freeTime - currTime) % 86400 / 3600;
		minute = (myInfo->freeTime - currTime) % 3600 / 60 + 1;
		if (seek_in_file(POLICE, currentuser->userid))
			prints("��ִ���������˻���Ҫ����%d��%dСʱ%d���ӡ�",
			       day, hour, minute);
		else {
			prints("�㱻%s�������ˡ�����%d��%dСʱ%d���ӵļ����",
			       CENTER_NAME, day, hour, minute);
			move(12, 0);
			if (askyn("    ��Ҫ��¸������", NA, NA) == NA) {
				whoTakeCharge(0, uident);
				if (!USERPERM(currentuser, PERM_SYSOP) &&
						strcmp(currentuser->userid, uident))
					return 0;
				move(14, 10);
				if(askyn("����Ҫ�������������й��������", NA, NA) == YEA)
					money_admin();									
				return 0;
			}
			num = userInputValue(14, 4, "��¸��","��" MONEY_NAME,
					1, myInfo->credit / 10000);
			if (num < 1) {
				showAt(16, 4, "�㲻���ˡ���", YEA);
				return 0;
			}
			if (num * 10000 > myInfo->credit) {
				showAt(16, 4, "��û����ô���", NA);
				return 0;
			}
			myInfo->credit -= num * 10000;
			mcEnv->Treasury += num * 10000;
			myInfo->freeTime -= num * 60;
			mcLog("��¸", num, "�� ������");
			if (currTime > myInfo->freeTime) {
				myInfo->freeTime = 0;
				showAt(16, 4, "����������Ļ�¸��͵͵������ˡ�", YEA);
				return 1;
			}
			if (currTime < myInfo->freeTime) {
				move(16, 4);
				prints("����������Ļ�¸����ļ��ʱ��������%d����", num);
			}
		}
		pressanykey();
		return 0;
	} else if (currTime > myInfo->freeTime && myInfo->freeTime > 0) {
		myInfo->freeTime = 0;
		if (seek_in_file(POLICE, currentuser->userid))
			showAt(10, 10, "��ϲ��������Ժ��", YEA);
		else
			showAt(10, 10, "�����������ϲ�����»�����ɣ�", YEA);
	}
	clrtoeol();
/* Ƿ��� */
	if (currTime > myInfo->backTime && myInfo->backTime > 0) {
		if (askyn("��Ƿ���еĴ�����ˣ��Ͻ����ɣ�", YEA, NA) == NA)
			return 0;
		money_bank();
		return 0;
	}
// ��������
	if(seek_in_file(BADID, currentuser->userid)) {
		showAt(10, 10, "�����Ҵ�������������򣬱�ȡ���������������ʸ�\n"
			       "          ��������ܹ���ϵ��", YEA);
		return 0;
	}
	return 1;
}

static void
newSalary()
{
	time_t currTime = time(NULL);
	char buf[256];

	if (currTime > mcEnv->salaryEnd) {
		mcEnv->salaryStart = currTime;
		mcEnv->salaryEnd = currTime + PAYDAY * 86400;
		sprintf(buf, "    ����%d���ڵ�%s������ȡ��������Ϊ������\n",
				PAYDAY, CENTER_NAME);
		deliverreport("�����С���վ����Ա��ȡ����", buf);
		mcLog("���Ź���", 0, "");
	}
	return;
}

static int
makeSalary()
{
	int salary = 0;
	float convertRate;
	if (currentuser->userlevel & PERM_OBOARDS ||
	    currentuser->userlevel & PERM_ACCOUNTS ||
	    currentuser->userlevel & PERM_ARBITRATE ||
	    currentuser->userlevel & PERM_SPECIAL4 ||
	    currentuser->userlevel & PERM_ACBOARD)
		salary = 2;
	if (currentuser->userlevel & PERM_SYSOP)
		salary = 3;
	if (currentuser->userlevel & PERM_BOARDS)
		salary +=
		    MIN_XY(getbmnum(currentuser->userid), 3);
	convertRate = MIN_XY(bbsinfo.utmpshm->mc.ave_score / 1000.0 + 1, 10) * 100000;
	salary *= convertRate;
	return salary/100 * 100;
}

static int
positionChange(int pos, char *boss, char *posStr, int type)
{
	char head[16], in[16], end[16];
	char buf[STRLEN], title[STRLEN], letter[2 * STRLEN];
	char posDesc[][20] =
	    { "���������ܹ�", "�����г�", "���ʹ�˾����", "�ĳ�����",
		"�ڰ����", "ؤ�����", "֤�����ϯ",
		"�̳�����", "������"
	};
	char ps[][STRLEN] =
	    { "������������������Ȩı˽����Ϊ��վ������ҵ�ķ�չ�Ϲ����ᡣ",
		"�����������һֱ�����Ĺ�����ʾ��л��ף�Ժ�˳����"
	};
	if (type == 0) {
		strcpy(head, "����");
		strcpy(in, "Ϊ");
		strcpy(end, "");
	} else {
		strcpy(head, "��ȥ");
		strcpy(in, "��");
		strcpy(end, "ְ��");
	}
	move(20, 4);
	snprintf(title, STRLEN - 1, "���ܹܡ�%s%s%s%s%s", head, boss, in,
		 posDesc[pos], end);
	sprintf(genbuf, "ȷ��Ҫ %s ��", (title + 8));	//��ȥ [����] 
	if (askyn(genbuf, YEA, NA) == NA)
		return 0;
	sprintf(genbuf, "%s %s", posStr, boss);
	if (type == 0) {
		addtofile(MC_BOSS_FILE, genbuf);
		sprintf(letter, "    %s\n", ps[0]);
	} else {
		getdata(21, 4, "ԭ��", buf, 40, DOECHO, YEA);
		sprintf(letter, "    ԭ��%s\n\n%s\n", buf, ps[1]);
		del_from_file(MC_BOSS_FILE, posStr);
	}
	deliverreport(title, letter);
	system_mail_buf(letter, strlen(letter), boss, title,
			currentuser->userid);
	sprintf(genbuf, "���� %s Ϊ %s", boss, posDesc[pos]);
	mcLog(genbuf, 0, "");
	showAt(22, 4, "�������", YEA);
	return 1;
}

// ------------------------   ����   ----------------------- // 
static int
setBankRate(int rateType)
{
	char buf[STRLEN], strRate[10];
	char rateDesc[][10] = { "�����", "������", "ת�ʷ�" };
	unsigned char rate;

	sprintf(buf, "�趨�µ�%s��[10-250]: ", rateDesc[rateType]);
	getdata(12, 4, buf, strRate, 4, DOECHO, YEA);
	rate = atoi(strRate);
	if (rate < 10 || rate > 250) {
		showAt(13, 4, "����������Χ!", YEA);
		return 1;
	}
	move(13, 4);
	sprintf(buf, "�µ�%s����%.2f����ȷ����", rateDesc[rateType],
		rate / 100.0);
	if (askyn(buf, NA, NA) == NA)
		return 1;
	if (rateType == 0)
		mcEnv->depositRate = rate;
	else if (rateType == 1)
		mcEnv->loanRate = rate;
	else
		mcEnv->transferRate = rate;
	update_health();
	myInfo->health--;
	myInfo->Actived++;
	sprintf(genbuf, "    �µ�%s��Ϊ %.2f�� ��\n", rateDesc[rateType],
		rate / 100.0);
	sprintf(buf, "�����С�%s���е���%s��", CENTER_NAME, rateDesc[rateType]);
	deliverreport(buf, genbuf);
	sprintf(buf, "����%s��Ϊ", rateDesc[rateType]);
	mcLog(buf, (int)rate, "/ 10000");
	showAt(14, 4, "�������", YEA);
	return 0;
}

static int
bank_saving()
{
	char ch, quit = 0, buf[STRLEN], getInterest;
	float rate = mcEnv->depositRate / 10000.0;
	int num, total_num, cal_Interest;

	money_show_stat("���д����");
	sprintf(buf, "������ʣ��գ�Ϊ %.2f��", rate * 100);
	showAt(4, 4, buf, NA);
	move(t_lines - 1, 0);
	prints("\033[1;44m ѡ�� \033[1;46m [1]��� [2]ȡ�� ");
	if(seek_in_file(ROBUNION, currentuser->userid))
		prints("[3] �ڰﱣ���� ");
	if(seek_in_file(BEGGAR, currentuser->userid))
		prints("[3] ؤ��С��� ");
	if(seek_in_file(POLICE, currentuser->userid))
		prints("[3] ����˽��Ǯ ");
	prints("[Q]�뿪\033[m");
	ch = igetkey();
	switch (ch) {
	case '1':
		if (check_health(1, 12, 4, "�������������ˣ�", YEA))
			break;
		num =
		    userInputValue(6, 4, "��", MONEY_NAME, 1000, MAX_MONEY_NUM);
		if (num == -1)
			break;
		if (myInfo->cash < num) {
			showAt(8, 4, "��û����ô��Ǯ���Դ档", YEA);
			break;
		}
		myInfo->cash -= num;
/* ����ԭ�ȴ�����Ϣ */
		cal_Interest = makeInterest(myInfo->credit, myInfo->depositTime, rate);
		myInfo->interest += cal_Interest;
		mcEnv->Treasury -= cal_Interest;
/* �µĴ�ʼʱ�� */
		myInfo->depositTime = time(NULL);
		myInfo->credit += num;
		update_health();
		myInfo->health--;
		myInfo->Actived++;
		move(8, 4);
		prints("���׳ɹ��������ڴ��� %d %s����Ϣ���� %d %s��",
		       myInfo->credit, MONEY_NAME, myInfo->interest,
		       MONEY_NAME);
		pressanykey();
		mcLog("���", num, "");
		break;
	case '2':
		if (check_health(1, 12, 4, "�������������ˣ�", YEA))
			break;
		num =
		    userInputValue(6, 4, "ȡ", MONEY_NAME, 1000, MAX_MONEY_NUM);
		if (num == -1)
			break;
		if (num > myInfo->credit) {
			showAt(8, 4, "��û����ô���", YEA);
			break;
		}
		
		cal_Interest = makeInterest(num, myInfo->depositTime, rate);
		myInfo->interest += cal_Interest;
		mcEnv->Treasury -= cal_Interest;
		move(8, 4);
		sprintf(genbuf, "�Ƿ�ȡ�� %d %s�Ĵ����Ϣ", 
				myInfo->interest, MONEY_NAME);
		if (askyn(genbuf, NA, NA) == YEA) {
/* ������Ϣ */
			total_num = num + myInfo->interest;
			myInfo->interest = 0;
			getInterest = 1;
		} else {
			total_num = num;
			getInterest = 0;
		}
		myInfo->credit -= num;
		myInfo->cash += total_num;
		update_health();
		myInfo->health--;
		myInfo->Actived++;
		move(9, 4);
		prints("���׳ɹ��������ڴ��� %d %s�������Ϣ���� %d %s��",
		       myInfo->credit, MONEY_NAME, myInfo->interest,
		       MONEY_NAME);
		pressanykey();
		mcLog("ȡ��", total_num, "");
		break;
	case '3':
		if(seek_in_file(ROBUNION, currentuser->userid))
			while(1) {
				if (secureMoney(0) == 1)
					break;
			}
		if(seek_in_file(BEGGAR, currentuser->userid))
			while(1) {
				if (secureMoney(1) == 1)
					break;
			}
		if(seek_in_file(POLICE, currentuser->userid))
			while(1) {
				if (secureMoney(2) == 1)
					break;
			}
		break;
	case 'Q':
	case 'q':
		quit = 1;
		break;
	}
	return quit;
}

static int
bank_loan()
{
	char ch, quit = 0, buf[STRLEN];
	float rate = mcEnv->loanRate / 10000.0;
	int num, total_num, hour, maxLoanMoney;
	time_t currTime = time(NULL);

	money_show_stat("���д����");
	sprintf(buf, "�������ʣ��գ�Ϊ %.2f��", rate * 100);
	showAt(4, 4, buf, NA);
	move(5, 4);
	hour = (myInfo->backTime - currTime) / 3600 + 1;
	total_num =
	    myInfo->loan + makeInterest(myInfo->loan, myInfo->loanTime, rate);
	if (myInfo->loan > 0) {
		prints("������ %d %s����ǰ��Ϣ���� %d %s���ൽ�� %d Сʱ��",
		       myInfo->loan, MONEY_NAME, total_num, MONEY_NAME, hour);
	} else
		prints("��Ŀǰû�д��");
	move(t_lines - 1, 0);
	prints("\033[1;44m ѡ�� \033[1;46m [1]���� [2]���� [Q]�뿪\033[m");
	ch = igetkey();
	switch (ch) {
	case '1':
		if (check_health(1, 12, 4, "�������������ˣ�", YEA))
			break;
		maxLoanMoney = MIN_XY(countexp(currentuser) * 500, mcEnv->Treasury);
		move(6, 4);
		if (maxLoanMoney < 1000) {
			showAt(8, 4, "�Բ�������û�д�����ʸ�", YEA);
			break;
		}
		prints("�������еĹ涨����Ŀǰ������������� %d %s��",
		       maxLoanMoney, MONEY_NAME);
		num =
		    userInputValue(7, 4, "��", MONEY_NAME, 1000, maxLoanMoney);
		if (num == -1)
			break;
		if (myInfo->loan > 0) {
			showAt(8, 4, "���Ȼ�����", YEA);
			break;
		}
		if (num > maxLoanMoney) {
			showAt(8, 4, "�Բ�����Ҫ�����Ľ������й涨��",
			       YEA);
			break;
		}
		while (1) {
			getdata(8, 4, "��Ҫ��������죿[3-30]: ", buf,
				3, DOECHO, YEA);
			if (atoi(buf) >= 3 && atoi(buf) <= 30)
				break;
		}
		myInfo->loanTime = currTime;
		myInfo->backTime = currTime + atoi(buf) * 86400;
		mcEnv->Treasury -= num;
		myInfo->loan += num;
		myInfo->cash += num;
		update_health();
		myInfo->health--;
		myInfo->Actived++;
		showAt(9, 4, "���Ĵ��������Ѿ���ɡ��뵽�ڻ��", YEA);
		mcLog("����", num, "");
		break;
	case '2':
		if (check_health(1, 12, 4, "�������������ˣ�", YEA))
			break;
		if (myInfo->loan == 0) {
			showAt(6, 4, "���Ǵ��˰ɣ�û���ҵ����Ĵ����¼����",
			       YEA);
			break;
		}
		if (time(NULL) < myInfo->loanTime + 86400*3) {
			move(6, 4);
			if (askyn("��Ҫ��ǰ����������(����ȡ1%������)", NA, NA) == NA)
				break;
			total_num *= 1.01;
			move(5, 4);
			prints("������ %d %s����ǰ��Ϣ���� %d %s���ൽ�� %d Сʱ��",
				myInfo->loan, MONEY_NAME, total_num, MONEY_NAME, hour);
		} else {
			move(6, 4);
			if (askyn("��Ҫ���ڳ���������", NA, NA) == NA)
				break;
		}
		if (myInfo->cash < total_num) {
			showAt(7, 4, "�Բ�������Ǯ�����������", YEA);
			break;
		}
		myInfo->cash -= total_num;
		mcEnv->Treasury += total_num;
		myInfo->loan = 0;
		myInfo->loanTime = 0;
		myInfo->backTime = 0;
		update_health();
		myInfo->health--;
		myInfo->Actived++;
		showAt(7, 4, "���Ĵ����Ѿ����塣�����ּ����������ĳ��š�", YEA);
		mcLog("����", total_num, "");
		break;
	case 'q':
	case 'Q':
		quit = 1;
		break;
	}
	return quit;
}

#if 0
static int
bank_sploan()
{
	money_show_stat("������ҵ����Ǣ̸��");
	showAt(6, 4,
	       "\033[1;33m��ҵ������Ϊ�˻�����ʱ���ʽ���ת��������õġ�\n    ���������ͨ������ص��ǽ�����Ϣ�ߣ�Ƿ���ͷ��ء�\n\n    ��ҵ����Ŀͻ��Ƕĳ����̵�Ⱦ�Ӫ��ҵ��а��ˣ��ڰ�ؤ������Ÿ����ˡ�\n\n\033[1;32m    ����������ϸ�ﻮ�С�����\033[m",
	       YEA);
	return 1;
}
#endif

static void
moneytransfer()
{
	int ch, num, total_num, cal_interest;
	char uident[IDLEN + 1], title[STRLEN], buf[256];
	float transferRate = mcEnv->transferRate / 10000.0;
	float rate = mcEnv->depositRate / 10000.0;
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;

	money_show_stat("����ת�˴���");
	if (check_health(1, 12, 4, "�������������ˣ�", YEA))
		return;
	move(4, 4);
	sprintf(genbuf,
		"��Сת�˽�� 1000 %s�������� %.2f�� ",
		MONEY_NAME, transferRate * 100);
	prints("%s", genbuf);
	if (!getOkUser("ת�˸�˭��", uident, 5, 4)) {
		showAt(12, 4, "���޴���", YEA);
		return;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(12, 4, "�ˣ����ǡ���ɶ�أ���", YEA);
		return;
	}
	num = userInputValue(6, 4, "ת��", MONEY_NAME, 1000, MAX_MONEY_NUM);
	if (num == -1)
		return;
	total_num = num + num * transferRate;
	prints("\n    ��ѡ�񣺴�[1]�ֽ� [2]��� ת�ʣ�");
	ch = igetkey();
	sethomefile(buf, uident, "mc.save");
	if (!file_exist(buf))
		initData(1, buf);
	mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
	if (mcuInfo == (void *) -1)
		return;

	switch (ch) {
	case '1':
		if (myInfo->cash < total_num) {
			move(12, 4);
			prints("�����ֽ𲻹����������Ѵ˴ν��׹��� %d %s",
			       total_num, MONEY_NAME);
			pressanykey();
			return;
		}
		myInfo->cash -= total_num;
		cal_interest= makeInterest(mcuInfo->credit, mcuInfo->depositTime, rate);
		mcuInfo->interest += cal_interest;
		mcuInfo->depositTime = time(NULL);
		mcuInfo->credit += num;
		mcEnv->Treasury += num * transferRate - cal_interest;
		myInfo->Actived++;
		break;
	default:
		if (myInfo->credit < total_num) {
			move(12, 4);
			prints("���Ĵ������������Ѵ˴ν��׹��� %d %s",
			       total_num, MONEY_NAME);
			pressanykey();
			return;
		}
		myInfo->credit -= total_num;
		cal_interest = makeInterest(total_num, myInfo->depositTime, rate);
		myInfo->interest += cal_interest;
		mcEnv->Treasury -= cal_interest;
		cal_interest = makeInterest(mcuInfo->credit, mcuInfo->depositTime, rate);
		mcuInfo->interest += cal_interest;
		mcuInfo->depositTime = time(NULL);
		mcuInfo->credit += num;
		mcEnv->Treasury += num * transferRate - cal_interest;
		myInfo->Actived++;
		break;
	}

	update_health();
	myInfo->health--;
	saveData(mcuInfo, sizeof (struct mcUserInfo));
	sleep(1);
	sprintf(title, "�������� %s ������Ǯ����", currentuser->userid);
	sprintf(buf, "%s ͨ��%s���и���ת���� %d %s������ա�",
		currentuser->userid, CENTER_NAME, num, MONEY_NAME);
	system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
	if (num >= 1000000) {
		sprintf(title, "�����С�%s����ʽ�ת��", currentuser->userid);
		sprintf(buf, "    %s ͨ��%s����ת���� %d %s ��%s��\n",
			currentuser->userid, CENTER_NAME, num, MONEY_NAME,
			uident);
		deliverreport(title, buf);
	}
	sprintf(buf, "��%sת��%d%s�ɹ��������Ѿ�֪ͨ���������ѡ�",
		(ch == '1') ? "�ֽ�" : "���", num, MONEY_NAME);
	showAt(12, 4, buf, YEA);
	mcLog("ת��", num, uident);
	return;
}

static int
secureMoney(int type)
{
	char ch;
	int inputNum, max_money, quit = 0;
	struct tm Tsecure, Tnow;
	time_t currTime;
	
	if (myInfo->safemoney % 500000 != 0)
		myInfo->safemoney = 0;
	max_money = countexp(currentuser)/50 * 500000;
	move(6, 0);
	switch(type) {
	case 0:
		money_show_stat("�ڰﱣ����");
		prints("    ����ڱ����������Ǯ���ᱻ͵����\n");
		break;
	case 1:
		money_show_stat("ؤ��С���");
		prints("    ����С��������Ǯ����͵����\n");
		break;
	case 2:
		money_show_stat("����˽��Ǯ");
		prints("    ����Ҳ��Ҫ˽��Ǯ��������\n");
		break;
	}
	prints("    ÿ��ÿ��ֻ�ܴ�50�򣬲�����ȡ10����������\n"
	       "    ��ı��������� %10d �� %s\n"
	       "    ������ܴ�     %10d �� %s",
	       myInfo->safemoney/10000, MONEY_NAME, max_money/10000, MONEY_NAME);
	move(t_lines-1, 0);
	prints("\033[1;44m ѡ�� \033[1;46m [1] ��Ǯ [2] ȡǮ [Q] �뿪\033[m");
	ch = igetkey();
	switch (ch) {
	case '1':
		update_health();
		if (check_health(1, 10, 4, "��û���㹻�������ˣ�", YEA))
			break;
		localtime_r(&myInfo->secureTime, &Tsecure);
		currTime = time(NULL);
		localtime_r(&currTime, &Tnow);
		if (Tsecure.tm_yday == Tnow.tm_yday &&
		    Tsecure.tm_year == Tnow.tm_year) {
			showAt(10, 4, "������Ѿ����50���ˣ�", YEA);
			break;
		}
		if (myInfo->safemoney + 500000 > max_money) {
			showAt(10, 4, "���������ڵĵ�λֻ�ܲ���ôЩǮ�ˡ�", YEA);
			break;
		}
		move(10, 4);
		if (askyn("��ȷʵҪ��50����", NA, NA) == NA)
			break;
		if (myInfo->cash < 550000) {
			showAt(12, 4, "��û�д����ֽ�", YEA);
			break;
		}
		myInfo->cash -= 550000;
		myInfo->safemoney += 500000;
		mcEnv->Treasury += 50000;
		myInfo->Actived++;
		myInfo->health--;
		myInfo->secureTime = time(NULL);
		showAt(12, 4, "���׳ɹ���\n", YEA);
		mcLog("����", 50, "�� ��ȫǮ");
		break;
	case '2':
		update_health();
		if (check_health(1, 10, 4, "��û���㹻�������ˣ�", YEA))
			break;
		inputNum = userInputValue(10, 4, "��Ҫȡ", "��" MONEY_NAME "��",
				50, myInfo->safemoney / 500000 * 50);
		if (inputNum < 0) {
			showAt(12, 4, "�㲻����ȡǮ�ˡ�", YEA);
			break;
		}
		if (inputNum * 10000 > myInfo->safemoney) {
			showAt(12, 4, "��û����ô��Ǯ��", YEA);
			break;
		}
		if (inputNum % 50 != 0) {
			showAt(12, 4, "ֻ��ȡ50�����������", YEA);
			break;
		}
		myInfo->cash += inputNum * 10000;
		myInfo->safemoney -= inputNum * 10000;
		myInfo->Actived++;
		myInfo->health--;
		showAt(14, 4, "���׳ɹ���\n", YEA);
		mcLog("ȡ��", inputNum, "�� ��ȫǮ");
		break;
	case 'Q':
	case 'q':
		quit = 1;
		break;
	}
	return quit;
}
static int
after_tax(int income)
{
	int after;
	after = income;
	if (income >   10000)
		after =   10000 + (income -   10000)* 0.95;
	if (income >  100000)
		after =   95500 + (income -  100000)* 0.9;
	if (income >  500000)
		after =  455500 + (income -  500000)* 0.8;
	if (income > 1000000)
		after =  855500 + (income - 1000000)* 0.7;
	if (income > 5000000)
		after = 3655500 + (income - 5000000)* 0.6;
	mcEnv->Treasury += income - after;
	if (income > after)
		mcLog("������", income - after, "��˰");
	return after;
}

static int
money_bank()
{
	int ch, quit = 0,insurance;
	char uident[IDLEN + 1], buf[256];

	while (!quit) {
		sprintf(buf, "%s����", CENTER_NAME);
		money_show_stat(buf);
		move(8, 16);
		prints("%s���л�ӭ���Ĺ��٣�", CENTER_NAME);
		move(t_lines - 1, 0);
		prints("\033[1;44m ѡ�� \033[1;46m [1]ת�� [2]����"
		       " [3]���� [4]���� [5]���� [6]�г��칫�� [Q]�뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			moneytransfer();
			break;
		case '2':
			while (1) {
				if (bank_saving() == 1)
					break;
			}
			break;
		case '3':
			while (1) {
				if (bank_loan() == 1)
					break;
			}
			break;
#if 0
		case '4':
			while (1) {
				if (bank_sploan() == 1)
					break;
			}
			break;
#endif
		case '4':
			sendSalary();
			break;
		case '5':
			money_show_stat("���չ�˾");
			move(6, 4);
			if (askyn("��Ҫ������", NA, NA) == NA)
				break;
			if (myInfo->cash < 10000) {
				showAt(7, 4, "��û�д����ֽ�", YEA);
				break;
			}
			insurance = userInputValue(7, 4, "��ҪͶ��", "����", 1, myInfo->cash / 10000);
			if (insurance < 1) {
				showAt(9, 4, "�㲻����Ͷ���ˡ�", YEA);
				break;
			}
			myInfo->cash -= insurance * 10000;
			mcEnv->Treasury += insurance * 10000;
			myInfo->insuranceTime = time(NULL) + insurance * 60;
			move(10, 4);
			prints("��Ͷ����%dСʱ%d����", insurance / 60, insurance % 60);
			mcLog("Ͷ��", insurance, "����");
			pressanykey();
			break;
		case '6':
			money_show_stat("�г��칫��");
			move(6, 4);
			whoTakeCharge(1, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			prints("��ѡ���������:");
			move(7, 0);
			sprintf(genbuf, "      1. ����������ʡ�Ŀǰ����: %.2f��\n"
					"      2. �����������ʡ�Ŀǰ����: %.2f��\n"
					"      3. ����ת�ʷ��ʡ�Ŀǰ����: %.2f��\n"
					"      Q. �˳�", mcEnv->depositRate / 100.0, 
					mcEnv->loanRate / 100.0, mcEnv->transferRate / 100.0);
			prints(genbuf);
			ch = igetkey();
			switch (ch) {
			case '1':
			case '2':
			case '3':
				update_health();
				if (check_health
				    (1, 12, 4, "������̫�����ˣ���Ϣһ�°ɣ�",
				     YEA))
					break;
				setBankRate(ch - '1');
				break;
			}
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
		limit_cpu();
	}
	return 0;
}

static void
sendSalary()
{
	int salary;
	money_show_stat("���й��ʴ��촰��");
	if (check_health(1, 12, 4, "�������������ˣ�", YEA))
		return;;
	salary = makeSalary();
	if (salary == 0) {
		showAt(10, 10, "�����Ǳ�վ����Ա��û�й��ʡ�",
				YEA);
		return;
	}
	if (mcEnv->salaryStart == 0
			|| time(NULL) > mcEnv->salaryEnd) {
		showAt(10, 10, "�Բ������л�û���յ����ʻ��", YEA);
		return;
	}
	if (myInfo->lastSalary >= mcEnv->salaryStart) {
		sprintf(genbuf, "���Ѿ�����������������ڷܹ����ɣ�\n"
				"          �´η��������ڣ�%16s",
				ctime(&mcEnv->salaryEnd));
		showAt(10, 10, genbuf, YEA);
		return;
	}
	//if (mcEnv->Treasury < salary){
	//      showAt(10, 10, "�Բ�����������û���ֽ������Ժ�������", YEA);
	//      return;
	//      }

	move(6, 4);
	sprintf(genbuf, "�����µĹ��� %d %s�Ѿ��������С�������ȡ��",
			salary, MONEY_NAME);
	if (askyn(genbuf, NA, NA) == NA)
		return;
	myInfo->lastSalary = mcEnv->salaryEnd;
	myInfo->cash += salary;
	mcEnv->Treasury -= salary;
	update_health();
	myInfo->health--;
	myInfo->Actived++;
	showAt(8, 4, "���������Ĺ��ʡ���л���������Ĺ���!",
			YEA);
	mcLog("��ȡ", salary, "�Ĺ���");
	return;
}

// ------------------------  ��Ʊ  -------------------------- // 
//part one: 36_7 
static int
valid367Bet(char *buf)
{
	int i, j, temp[7], slot = 0;

	if (strlen(buf) != 20)	//  ���ȱ���Ϊ20= 2 * 7 + 6 
		return 0;
	for (i = 0; i < 20; i += 3) {	//  ������ʽ������ȷ 
		if (i + 2 != 20)
			if (buf[i + 2] != '-')	// �ָ�������ȷ 
				return 0;
		if (!isdigit(buf[i]) || !isdigit(buf[i + 1]))	// �������� 
			return 0;
		temp[slot] = (buf[i] - '0') * 10 + (buf[i + 1] - '0');
		if (temp[slot] > 36)
			return 0;
		slot++;
	}
	for (i = 0; i < 7; i++) {	// �������ظ� 
		for (j = 0; j < 7; j++)
			if (temp[j] == temp[i] && i != j)
				return 0;
	}
	return 1;
}

static int
parse367Bet(char *prizeSeq, char *bet, struct LotteryRecord *LR)
{
	int i, j, count = 0;
	int len = strlen(prizeSeq);

	if (strlen(bet) != len)
		return 0;
	for (i = 0; i + 1 < len; i = i + 3) {
		for (j = 0; j + 1 < len; j = j + 3) {
			if (bet[i] == prizeSeq[j]
			    && bet[i + 1] == prizeSeq[j + 1])
				count++;
		}
	}
	if (count >= 3 && count <= 7)
		LR->betCount[7 - count] = 1;
	return count;
}

static void
make367Seq(char *prizeSeq)
{
	int i, j, num, slot = 0, success;
	int temp[7];

	for (i = 0; i < 7; i++) {
		do {		//  ���ֲ�����ͬ 
			success = 1;
			num = 1 + rand() % 36;
			for (j = 0; j <= slot; j++) {
				if (num == temp[j]) {
					success = 0;
					break;
				}
			}
			if (success)
				temp[slot++] = num;
		}
		while (!success);
		prizeSeq[3 * i] = (char) (num / 10 + '0');
		prizeSeq[3 * i + 1] = (char) (num % 10 + '0');
		if (i != 6)
			prizeSeq[3 * i + 2] = '-';
		else
			prizeSeq[3 * i + 2] = '\0';
	}
	sprintf(genbuf, "    ��������ǣ�  %s  �����н�����\n", prizeSeq);
	deliverreport("����Ʊ������36ѡ7��Ʊҡ�����", genbuf);
}

//part two: soccer 
static int
validSoccerBet(char *buf)
{
	int i, count = 0, meetSeperator = 1;
	int first = 0, second = 0;

	if (buf[0] == '\0')
		return 0;
	for (i = 0; i < strlen(buf); i++) {
		if (buf[i] == '-') {
			if (meetSeperator == 1)	//�����������-���϶�����ȷ 
				return 0;
			count = 0;
			meetSeperator = 1;
		} else {
			if (buf[i] != '3' && buf[i] != '1' && buf[i] != '0')
				return 0;
			count++;
			if (count > 3)
				return 0;
			if (count == 1) {
				first = buf[i];
			} else if (count == 2) {
				if (buf[i] == first)	//�غ� 
					return 0;
				second = buf[i];
			} else if (count == 3) {
				if (buf[i] == first || buf[i] == second)	//�غ� 
					return 0;
			}
			meetSeperator = 0;
		}
	}
	if (buf[strlen(buf) - 1] == '-')
		return 0;
	return 1;
}

static int
computeSum(char *complexBet)
{				//���㸴ʽע������ 
	int i, countNum = 0, total = 1;
	int len = strlen(complexBet);

	for (i = 0; i < len; i++) {
		if (complexBet[i] == '-') {
			total *= countNum;
			countNum = 0;
		} else
			countNum++;
	}
	total *= countNum;	// �ٳ������һ����Ԫ 
	return total;
}

static int
makeSoccerPrize(char *bet, char *prizeSeq)
{
	int i, diff = 0;
	int n1 = strlen(bet);
	int n2 = strlen(prizeSeq);

	if (n1 != n2)
		return 10;	// ���н� 
	for (i = 0; i < n1; i++) {
		if (bet[i] != prizeSeq[i])
			diff++;
	}
	return diff;
}

static void
parseSoccerBet(char *prizeSeq, char *complexBet, struct LotteryRecord *LR)
{				// ����һ����ʽ��ע,���潱�������LR 
	int i, j, simple = 1, meet = 0, count = 0;
	int firstDivEnd, firstDivStart;
	int len = strlen(complexBet);

	firstDivEnd = len;
	for (i = 1; i < len; i += 2) {
		if (complexBet[i] != '-') {
			simple = 0;
			break;
		}
	}
	if (simple) {		//�򵥱�׼��ʽ 
		int diff;
		char buf[STRLEN];

		for (i = 0, j = 0; i < len; i++) {
			if (complexBet[i] != '-')
				buf[j++] = complexBet[i];
		}
		buf[j] = '\0';
		diff = makeSoccerPrize(prizeSeq, buf);
		if (diff <= 4 && diff >= 0)
			LR->betCount[diff]++;
	} else {
		for (i = 0; i < len; i++) {	//Ѱ�ҵ�һ����ʽ��Ԫ 
			if (complexBet[i] == '-') {
				if (count > 1 && !meet) {
					firstDivEnd = i;
					break;
				} else
					count = 0;
			} else
				count++;
		}
		firstDivStart = firstDivEnd - count;
		firstDivEnd--;

		for (i = 0; i < count; i++) {	//��ÿһ��Ҫ��ֵĵ�Ԫ��Ԫ�� 
			int slot = 0;
			char temp[STRLEN];

//�õ�ǰ��Ĳ��� 
			if (firstDivStart != 0) {
				for (j = 0; j < firstDivStart; j++, slot++)
					temp[slot] = complexBet[j];
			}
			temp[slot] = complexBet[firstDivStart + i];
			slot++;
//�õ�����Ĳ��� 
			for (j = firstDivEnd + 1; j < len; j++, slot++) {
				temp[slot] = complexBet[j];
			}
			temp[slot] = '\0';
//��ÿһ����֣����еݹ���� 
			parseSoccerBet(prizeSeq, temp, LR);
		}

	}
}

//part three: misc 
static int
createLottery(int prizeMode)
{
	char buf[STRLEN];
	char lotteryDesc[][16] = { "-", "36ѡ7��Ʊ", "���" };
	int day;
	time_t *startTime = 0, *endTime = 0, currTime = time(NULL);

	if (prizeMode == 1) {
		startTime = &(mcEnv->start367);
		endTime = &(mcEnv->end367);
	} else {
		startTime = &(mcEnv->soccerStart);
		endTime = &(mcEnv->soccerEnd);
	}
	update_health();
	if (check_health(1, 12, 4, "�������������ˣ�", YEA))
		return 1;
	move(12, 4);
	if (currTime < *endTime) {
		prints("%s�������ڻ��Ƚ��С�", lotteryDesc[prizeMode]);
		pressanykey();
		return 1;
	}
	prints("�½�%s", lotteryDesc[prizeMode]);
	while (1) {
		getdata(14, 4, "��Ʊ��������[1-7]: ", buf, 2, DOECHO, YEA);
		day = atoi(buf);
		if (day >= 1 && day <= 7)
			break;
	}
	*startTime = currTime;
	*endTime = currTime + day * 86400;
	update_health();
	myInfo->health--;
	myInfo->Actived++;
	sprintf(genbuf, "    ���ڲ�Ʊ���� %d ��󿪽�����ӭ���ӻԾ����\n", day);
	sprintf(buf, "����Ʊ����һ��%s��ʼ����", lotteryDesc[prizeMode]);
	deliverreport(buf, genbuf);
	showAt(15, 4, "�����ɹ����뵽ʱ������", YEA);
	mcLog("�½�", 0, lotteryDesc[prizeMode]);
	return 0;
}

static void
savePrizeList(int prizeMode, struct LotteryRecord LR,
	      struct LotteryRecord *totalCount)
{				//���н������userid��������ʱ�ļ� 

	FILE *fp;
	struct LotteryRecord *LR_curr;
	int i, n = 0, miss = 1;
	void *prizeListMem;

	if (prizeMode == 1)
		sprintf(genbuf, "%s", DIR_MC_TEMP "36_7_prizeList");
	else
		sprintf(genbuf, "%s", DIR_MC_TEMP "soccer_prizeList");
	n = get_num_records(genbuf, sizeof (struct LotteryRecord));
	prizeListMem = malloc(sizeof (struct LotteryRecord) * (n + 1));
	if (prizeListMem == NULL)
		return;
	memset(prizeListMem, 0, sizeof (struct LotteryRecord) * (n + 1));
	if (file_exist(genbuf)) {
		if ((fp = fopen(genbuf, "r")) == NULL) {
			free(prizeListMem);
			return;
		}
		fread(prizeListMem, sizeof (struct LotteryRecord), n, fp);
		fclose(fp);
	}
	for (i = 0; i < n; i++) {
		LR_curr = prizeListMem + i * sizeof (struct LotteryRecord);
		if (!strcmp(LR_curr->userid, LR.userid)) {	// ���userid�Ѿ����� 
			for (i = 0; i < 5; i++)
				LR_curr->betCount[i] += LR.betCount[i];
			miss = 0;
			break;
		}
	}
	if (miss)		//userid��¼������, add 
		memcpy(prizeListMem + n * sizeof (struct LotteryRecord), &LR,
		       sizeof (struct LotteryRecord));
	if ((fp = fopen(genbuf, "w")) == NULL) {
		free(prizeListMem);
		return;
	}
	n = miss ? (n + 1) : n;
	fwrite(prizeListMem, sizeof (struct LotteryRecord), n, fp);
	fclose(fp);
	free(prizeListMem);
// ȫ��ͳ���ۼ� 
	for (i = 0; i < 5; i++)
		totalCount->betCount[i] += LR.betCount[i];
	return;
}

static int
sendPrizeMail(struct LotteryRecord *LR, struct LotteryRecord *totalCount)
{
	int i, totalMoney, perPrize, myPrizeMoney;
	char title[STRLEN];
	char *prizeName[] = { "NULL", "36ѡ7", "���", NULL };
	char *prizeClass[] = { "�ص�", "һ��", "����", "����", "��ο", NULL };
	float prizeRate[] = { 0.60, 0.20, 0.10, 0.05, 0.02 };
	int prizeMode = strcmp(totalCount->userid, "36_7") == 0 ? 1 : 2;
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;

	sethomefile(genbuf, LR->userid, "mc.save");
	if (!file_exist(genbuf))
		initData(1, genbuf);
	mcuInfo = loadData(genbuf, buffer, sizeof (struct mcUserInfo));
	if (mcuInfo == (void *) -1)
		return -1;
	if (prizeMode == 1)
		totalMoney = mcEnv->prize367 + PRIZE_PER;
	else
		totalMoney = mcEnv->prizeSoccer + PRIZE_PER;
	for (i = 0; i < 5; i++) {	// ��ÿ���н���userid,�����ֽ��� 
		if (LR->betCount[i] > 0 && totalCount->betCount[i] > 0) {
			perPrize =
			    prizeRate[i] * totalMoney / totalCount->betCount[i];
			myPrizeMoney = perPrize * LR->betCount[i];
			mcuInfo->cash += myPrizeMoney;
			mcEnv->Treasury -= myPrizeMoney;
			sprintf(genbuf,
				"��һ������ %d ע,�õ��� %d %s�Ľ��𡣹�ϲ��",
				LR->betCount[i], myPrizeMoney, MONEY_NAME);
			sprintf(title, "��ϲ�����%s%s����",
				prizeName[prizeMode], prizeClass[i]);
			system_mail_buf(genbuf, strlen(genbuf), LR->userid,
					title, currentuser->userid);
		}
	}
	saveData(mcuInfo, sizeof (struct mcUserInfo));
	return 0;
}

static int
doOpenLottery(int prizeMode, char *prizeSeq)
{
	FILE *fp;
	char line[256], buf[STRLEN], title[STRLEN];
	char *bet, *userid;
	int totalMoney, remainMoney, i;
	struct LotteryRecord LR, totalCount;
	char *prizeName[] = { "NULL", "36ѡ7", "���", NULL };
	char *prizeClass[] = { "�ص�", "һ��", "����", "����", "��ο", NULL };
	float prizeRate[] = { 0.60, 0.20, 0.10, 0.05, 0.02 };

	if (prizeMode == 1) {
		make367Seq(prizeSeq);	//�������� 
		totalMoney = mcEnv->prize367 + PRIZE_PER;
		fp = fopen(DIR_MC "36_7_list", "r");
	} else {
		totalMoney = mcEnv->prizeSoccer + PRIZE_PER;
		fp = fopen(DIR_MC "soccer_list", "r");
	}
	totalMoney = MIN_XY(totalMoney, MAX_POOL_MONEY);
	if (fp == NULL)
		return -1;
//   ---------------------���㽱��----------------------- 

	memset(&totalCount, 0, sizeof (struct LotteryRecord));	// �ܼƳ�ʼ�� 
	while (fgets(line, 255, fp)) {
		userid = strtok(line, " ");
		bet = strtok(NULL, "\n");
		if (!userid || !bet) {
			continue;
		}
		memset(&LR, 0, sizeof (struct LotteryRecord));
		strcpy(LR.userid, userid);
// ������ע,�н����������LR�� 
		if (prizeMode == 1)
			parse367Bet(prizeSeq, bet, &LR);
		else
			parseSoccerBet(prizeSeq, bet, &LR);
		for (i = 0; i < 5; i++) {	// ����Ƿ��н� 
			if (LR.betCount[i] > 0) {
				savePrizeList(prizeMode, LR, &totalCount);
				break;
			}
		}
	}
	fclose(fp);
//  ------------------------ ���� --------------------- 
	remainMoney = totalMoney;
	if (prizeMode == 1) {
		sprintf(genbuf, "%s", DIR_MC_TEMP "36_7_prizeList");
		strcpy(totalCount.userid, "36_7");
	} else {
		sprintf(genbuf, "%s", DIR_MC_TEMP "soccer_prizeList");
		strcpy(totalCount.userid, "soccer_");

// ����ǰ�����ɵ��н��ļ�,��ÿ���н�ID����. ÿ��ID��һ����¼ 
// ͬʱ�ܼ��ۼ� 
		new_apply_record(genbuf, sizeof (struct LotteryRecord),
				 (void *) sendPrizeMail, &totalCount);

//  ---------------------- ����֪ͨ --------------------- 
		for (i = 0; i < 5; i++) {
			if (totalCount.betCount[i] > 0) {
				sprintf(title, "����Ʊ������%s%s���������",
					prizeName[prizeMode], prizeClass[i]);
				sprintf(buf, "    ����ע��: %d\n��ע����: %d\n",
					totalCount.betCount[i],
					(int) (prizeRate[i] * totalMoney /
					       totalCount.betCount[i]));
				deliverreport(title, buf);
				remainMoney -= totalMoney * prizeRate[i];
			}
		}
	}
// ��ɨս�� 
	if (prizeMode == 1) {
		mcEnv->prize367 = remainMoney;
		unlink(DIR_MC "36_7_list");
		unlink(DIR_MC_TEMP "36_7_prizeList");
	} else {
		mcEnv->prizeSoccer = remainMoney;
		unlink(DIR_MC "soccer_list");
		unlink(DIR_MC_TEMP "soccer_prizeList");
	}
	return 0;
}

static int
tryOpenPrize(int prizeMode)
{
	char buf[STRLEN];
	int flag;
	time_t startTime, endTime;

	update_health();
	if (check_health(1, 12, 4, "�������������ˣ�", YEA))
		return 1;
	if (prizeMode == 1) {
		startTime = mcEnv->start367;
		endTime = mcEnv->end367;
	} else {
		startTime = mcEnv->soccerStart;
		endTime = mcEnv->soccerEnd;
	}
	if (startTime == 0) {
		showAt(t_lines - 5, 4, "û���ҵ��ò�Ʊ�ļ�¼...", YEA);
		return -1;
	}
	if (time(NULL) < endTime) {
		showAt(t_lines - 5, 4, "��û�е�������ʱ�䰡!", YEA);
		return -1;
	}
	if (prizeMode == 1)
		buf[0] = '\0';
	else {
		getdata(t_lines - 5, 4,
			"������ҽ�����(���� - )[��\033[1;33mENTER\033[m����]: ",
			buf, 55, DOECHO, YEA);
		if (buf[0] == '\0')
			return 0;
	}
	flag = doOpenLottery(prizeMode, buf);
	move(t_lines - 4, 4);
	if (flag == 0)
		prints("�����ɹ���");
	else
		prints("�����������...");
	update_health();
	myInfo->health--;
	myInfo->Actived++;
	pressanykey();
	mcLog("����", flag, (prizeMode == 1)? "36ѡ7" : "���" );
	return 0;
}

static int
buyLottery(int type)
{
	int i, j, needMoney, perMoney = 1000;
	int retv, maxBufLen, num, num367[7], trytime = 0, right = 0;
	int *poolMoney;
	char letter[128], buf[128], filepath[256];
	time_t startTime, endTime;
	char *desc[] = { "36ѡ7", "����" };

	money_show_stat("��Ʊ����");
	if (check_health(1, 12, 4, "�������������ˣ�", YEA))
		return 0;
	if (type == 0) {	//36ѡ7 
		sprintf(filepath, "%s", DIR_MC "36_7_list");
		maxBufLen = 21;
		poolMoney = &(mcEnv->prize367);
		startTime = mcEnv->start367;
		endTime = mcEnv->end367;
	} else {
		sprintf(filepath, "%s", DIR_MC "soccer_list");
		maxBufLen = 61;
		poolMoney = &(mcEnv->prizeSoccer);
		startTime = mcEnv->soccerStart;
		endTime = mcEnv->soccerEnd;
	}
	if (startTime == 0) {
		sprintf(buf, "��Ǹ����һ�ڵ�%s��Ʊ��δ��ʼ���ۡ�", desc[type]);
		showAt(4, 4, buf, YEA);
		return 0;
	}
	if (time(NULL) >= endTime) {
		showAt(4, 4, "��Ǹ�����ڲ�Ʊ�������Ѿ���������ȴ�������", YEA);
		return 0;
	}
	if (type == 0)
		showAt(4, 4, "��ע��������01��36��ѡ��7������(�����ظ�)�����ּ���-���������磺\n"
			"              08-13-01-25-34-17-18 01-12-23-34-32-21-10\n"
			"    ��Ʊ���ں󿪽�������3����3���������ֲ��н���", NA);
	else
		showAt(4, 0,
		       "    ����ʤ/ƽ/���ֱ�ΪΪ3/1/0������������-������\n"
		       "    ֧�ָ�ʽ��ע�����磺 1-310-1-01-3-30", NA);
	move(7, 4);
	prints("��ǰ����أ�\033[1;31m%d\033[m   �̶�����\033[1;31m%d\033[m",
	       *poolMoney, PRIZE_PER);
	sprintf(genbuf, "ÿע %d %s��ȷ����ע��", perMoney, MONEY_NAME);
	move(9, 4);
	if (askyn(genbuf, NA, NA) == NA)
		return 0;
	while (1) {
		if (type == 0) {
			if(getdata(10, 4, "����д��ע��(ֱ�Ӱ��س�����ϵͳ�Զ�����): ",
				buf, maxBufLen, DOECHO, YEA) < 20) {
				while (!right) {
					for (i=0;i<7;i++)
						num367[i] = random() % 36 + 101;
					right = 1;
					for (i=1;i<7;i++)
						for (j=0;j<i;j++)
							if(num367[i] == num367[j])
								right = 0;
				}
				sprintf(buf,"%3d%3d%3d%3d%3d%3d%3d", num367[0], num367[1],
					num367[2], num367[3], num367[4], num367[5], num367[6]);
				buf[3]='-';buf[6]='-';buf[9]='-';
				buf[12]='-';buf[15]='-';buf[18]='-';
				strcpy(buf,buf+1); buf[20]='\0';
			}
		} else
			getdata(10, 4, "����д��ע��: ", buf, maxBufLen, DOECHO, YEA);
		showAt(11, 4, buf, NA);
		if (askyn("\n    ���������һע��", YEA, NA) == NA) {
			if (trytime++ == 5)
				break;
			right = 0;
			continue;
		}
		if (type == 0)
			retv = valid367Bet(buf);
		else
			retv = validSoccerBet(buf);
		if (retv == 0) {
			showAt(13, 4, "�Բ���������ע����д�ò���ม�", YEA);
			if (trytime++ == 5)
				break;
			right = 0;
			continue;
		}
		if (type == 0) {
			num = 1;
			needMoney = perMoney;
		} else {
			num = computeSum(buf);
			needMoney = num * perMoney;
		}
		if (myInfo->cash < needMoney) {
			showAt(13, 4, "�Բ�������Ǯ������", YEA);
			return 0;
		}
		myInfo->cash -= needMoney;
		*poolMoney += needMoney;
		update_health();
		myInfo->health--;
		myInfo->Actived++;
		sprintf(genbuf, "%s %s", currentuser->userid, buf);
		if (type == 0)
			addtofile(DIR_MC "36_7_list", genbuf);
		else
			addtofile(DIR_MC "soccer_list", genbuf);
		sprintf(letter, "��������һע%s��Ʊ��ע���ǣ�%s��", desc[type],
			buf);
		system_mail_buf(letter, strlen(letter), currentuser->userid,
				"��Ʊ���Ĺ���ƾ֤", currentuser->userid);
		clrtoeol();
		sprintf(buf, "�ɹ����� %d ע%s��Ʊ ��ף���д󽱣�", num,
			desc[type]);
		sleep(1);
		showAt(13, 4, buf, YEA);
		mcLog("����", num, desc[type]);
		break;
	}
	return 1;
}

static int
money_lottery()
{
	char ch, quit = 0, quitRoom = 0;
	char uident[IDLEN + 1];

	while (!quit) {
		nomoney_show_stat("��Ʊ����");
		showAt(5, 4, "Ŀǰ�������ֲ�Ʊ: 36ѡ7 �� �����Ʊ����ӭ����",
		       NA);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1]36ѡ7 [2]��� [3]������ [Q]�뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
		case '2':
			buyLottery(ch - '1');
			break;
		case '3':
			nomoney_show_stat("���ʹ�˾������");
			whoTakeCharge(2, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			quitRoom = 0;
			while (!quitRoom) {
				nomoney_show_stat("���ʹ�˾������");
				move(5, 0);
				prints("    ������Ʊ:    1.  36ѡ7    %s",
				       file_isfile(DIR_MC "36_7_list")?"�ѽ���\n":"\n");
				prints("                 2.  �����Ʊ %s",
				       file_isfile(DIR_MC "soccer_list")?"�ѽ���\n":"\n");
				prints("    ��    ��:    3.  36ѡ7    ����ʱ�䣺%s",
					ctime(&mcEnv->end367));
				prints("                 4.  �����Ʊ ����ʱ�䣺%s",
					ctime(&mcEnv->soccerEnd));
				prints("                 Q.  �˳�");
				move(10, 4);
				prints("��ѡ��Ҫ�����Ĵ���:");
				ch = igetkey();
				switch (ch) {
				case '1':
				case '2':
					createLottery(ch - '0');
					break;
				case '3':
				case '4':
					tryOpenPrize(ch - '2');
					break;
				case 'q':
				case 'Q':
					quitRoom = 1;
					break;
				}
			}
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
		limit_cpu();
	}
	return 0;
}

// --------------------------    �ĳ�    ----------------------- // 
static int
money_dice()
{
	int i, ch, quit = 0, target, win, num, sum;
	unsigned int t[3];

	while (!quit) {
		money_show_stat("�ĳ�������");
		move(4, 0);
		prints("    �ִ�С���ţ�4-10����С��11-17��Ϊ��\n"
		       "    ��ѺС��С������һ���ʽ�Ѻ��ľ�ȫ��ׯ�ҡ�\n"
		       "    ׯ��Ҫ��ҡ��ȫ�����������ӵ���һ������ͨ�Դ�С�ҡ�\n"
		       "    \033[1;31m�����׬���������⣬�����֣�Ը�ķ���!\033[m");
		move(t_lines - 1, 0);
		prints("\033[1;44m ѡ�� \033[1;46m [1]��ע [2]VIP [Q]�뿪\033[m");
		win = 0;
		ch = igetkey();
		switch (ch) {
		case '1':
			update_health();
			if (check_health(1, 12, 4, "�������������ˣ�", YEA))
				break;
			num =
			    userInputValue(9, 4, "ѹ", MONEY_NAME, 1000,
					   200000);
			if (num == -1)
				break;
			getdata(11, 4, "ѹ��(L)����С(S)��[L]", genbuf, 3,
				DOECHO, YEA);
			if (genbuf[0] == 'S' || genbuf[0] == 's')
				target = 1;
			else
				target = 0;
			sprintf(genbuf,
				"�� \033[1;31m%d\033[m %s�� \033[1;31m%s\033[m��ȷ��ô��",
				num, MONEY_NAME, target ? "С" : "��");
			move(12, 4);
			if (askyn(genbuf, NA, NA) == NA)
				break;
			move(13, 4);
			if (myInfo->cash < num) {
				showAt(13, 4, "ȥȥȥ��û��ô��Ǯ��ʲô�ң�",
				       YEA);
				break;
			}
			myInfo->cash -= num;
			update_health();
			myInfo->health--;
			myInfo->Actived += 2;
			for (i = 0; i < 3; i++) {
				getrandomint(&t[i]);
				t[i] = t[i] % 6 + 1;
			}
			sum = t[0] + t[1] + t[2];
			if ((t[0] == t[1]) && (t[1] == t[2])) {
				mcEnv->prize777 += after_tax(num);
				sprintf(genbuf, "\033[1;32mׯ��ͨɱ��\033[m");
				mcLog("����������ͨɱ������", num, "");
			} else if (sum <= 10) {
				sprintf(genbuf, "%d �㣬\033[1;32mС\033[m",
					sum);
				if (target == 1)
					win = 1;
			} else {
				sprintf(genbuf, "%d �㣬\033[1;32m��\033[m",
					sum);
				if (target == 0)
					win = 1;
			}
			sleep(1);
			prints("���˿���~~  %d %d %d  %s", t[0], t[1], t[2],
			       genbuf);
			move(14, 4);
			if (win) {
				myInfo->cash += 2 * num;
				mcEnv->prize777 -= num;
				prints("��ϲ��������һ�Ѱɣ�");
				mcLog("������Ӯ��", num, "");
			} else {
				mcEnv->prize777 += after_tax(num);
				prints("û�й�ϵ�������Ӯ...");
				mcLog("����������", num, "");
			}
			pressanykey();
			break;
		case '2':
			money_big();
			break;
		case 'Q':
		case 'q':
			quit = 1;
			break;
		}
		limit_cpu();
	}
	return 0;
}

static int
calc777(int t1, int t2, int t3)
{
	if ((t1 % 2 == 0) && (t2 % 2 == 0) && (t3 % 2 == 0))
		return 2;
	if ((t1 % 2 == 0) && (t2 % 2 == 0) && (t3 == 1))
		return 3;
	if ((t1 % 2 == 0) && (t2 == 1) && (t3 == 1))
		return 4;
	if ((t1 == 1) && (t2 == 1) && (t3 % 2 == 0))
		return 4;
	if ((t1 % 2 == 0) && (t2 == 3) && (t3 == 3))
		return 6;
	if ((t1 == 3) && (t2 == 3) && (t3 % 2 == 0))
		return 6;
	if ((t1 == 1) && (t2 == 1) && (t3 == 1))
		return 11;
	if ((t1 == 3) && (t2 == 3) && (t3 == 3))
		return 21;
	if ((t1 == 5) && (t2 == 5) && (t3 == 5))
		return 41;
	if ((t1 == 5) && (t2 == 7) && (t3 == 7))
		return 61;
	if ((t1 == 7) && (t2 == 7) && (t3 == 7))
		return 81;
	return 0;
}

#if 0
static int
get_addedlevel(int level, int base, int num)
{
	if (num - (level + 1) * (level + 1) * base >
	    (level + 2) * (level + 2) * base) {
		level++;
		level = get_addedlevel(level, base, num - level * level * base);
	}
	return level;
}
#endif

static int
money_777()
{
	int i, ch, quit = 0, bid, winrate, num = 0, tax = 0;
	unsigned int t[3];
	char n[9] = "-R-B-6-7";
	char title[STRLEN], buf[256];

	while (!quit) {
		money_show_stat("�ĳ�777");
		if (mcEnv->prize777 < 0) {
			mcEnv->Treasury += mcEnv->prize777;
			mcEnv->prize777 = 0;
		}
		if (mcEnv->prize777 > 0 && mcEnv->Treasury < 0) {
			if (mcEnv->prize777 >= abs(mcEnv->Treasury)) {
				mcEnv->prize777 += mcEnv->Treasury;
				mcEnv->Treasury = 0;
			}
			if (mcEnv->prize777 < abs(mcEnv->Treasury)) {
				mcEnv->Treasury += mcEnv->prize777;
				mcEnv->prize777 = 0;
			}
		}
		if (check_health(1, 12, 4, "�������������ˣ�", YEA))
			break;
		if (!(random() % MAX_ONLINE/5)) {
			policeCheck();
			break;
		}

		move(6, 4);
		prints("--R 1:2    -RR 1:3    RR- 1:3    -BB 1:5    BB- 1:5");
		move(7, 4);
		prints("RRR 1:10   BBB 1:20   666 1:40   677 1:60   --- 1:1");
		move(8, 4);
		prints("777 1:80 (�л���Ӯ�õ�ǰ�ۼƻ����һ�룬��಻����100��)");
		move(9, 4);
		prints
		    ("Ŀǰ�ۻ�������: %d  ��Ӯ��ô��ѹ200���л���ม�",
		     mcEnv->prize777);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1] ѹ50 [2] ѹ200 [3] ע�뽱�� [Q]�뿪\033[m");
		ch = igetkey();
		if (ch == 'q' || ch == 'Q')
			break;
		if (ch == '1') {
			if (mcEnv->prize777 < 10000) {
				showAt(11, 4,
				       "Ŀǰ777�����������ȴ�ע�뽱��",
				       YEA);
				return 0;
			}
			bid = 50;
		} else if (ch == '2') {
			if (mcEnv->prize777 < 10000) {
				showAt(11, 4,
				       "Ŀǰ777�����������ȴ�ע�뽱��",
				       YEA);
				return 0;
			}
			bid = 200;
		} else if (ch == '3') {
			if (myInfo->cash < 10000) {
				showAt(11, 4, "������ֽ�̫���ˣ�cmft", YEA);
				continue;
			}
			num =
			    userInputValue(11, 4, "ע��", MONEY_NAME, 10000,
					   myInfo->cash);
			if (num <= 0)
				continue;
			myInfo->cash -= num;
			mcEnv->prize777 += after_tax(num);
			myInfo->luck = MIN_XY(100, myInfo->luck + num / 10000);
			showAt(15, 4, "ע�뽱��ɹ���", YEA);
			mcLog("��777����Ʒ������", num, "�ֽ�");
			continue;
		} else
			continue;
		if (myInfo->cash < bid) {
			showAt(11, 4, "ûǮ�ͱ�����...", YEA);
			continue;
		}
		myInfo->cash -= bid;
		update_health();
		myInfo->health--;
		myInfo->Actived += 2;
		for (i = 0; i < 3; i++) {
			getrandomint(&t[i]);
			t[i] = t[i] % 8;
			move(11, 20 + 2 * i);
			prints("%c", n[t[i]]);
			refresh();
			sleep(1);
		}
		winrate = calc777(t[0], t[1], t[2]);
		if (winrate <= 0) {
			mcEnv->prize777 += after_tax(bid);
			showAt(12, 4,
			       "���ˣ���ע�����ۻ������츣���˵����츣�Լ���",
			       YEA);
			continue;
		}
		mcEnv->prize777 -= bid * (winrate - 1);
		myInfo->cash += bid * winrate;
		move(12, 4);
		prints("��Ӯ�� %d %s", bid * (winrate - 1), MONEY_NAME);
		if (winrate == 81 && bid == 200) {
			num = MIN_XY(mcEnv->prize777 / 2, 1000000);
//			num = MAX_XY(num, random() % (myInfo->luck + 101) * 5000 );
			tax = num - after_tax(num);
			myInfo->cash += num - tax;
			mcEnv->prize777 -= num;
			move(12, 4);
			prints("\033[1;5;33m��ϲ����ô󽱣�������Ҫ��˰����\033[0m");
			sprintf(title, "���ĳ���%s Ӯ��777�󽱣�",
				currentuser->userid);
			sprintf(buf, "    �ĳ�������Ϣ��%s Ӯ��777�� %d %s!\n"
					"    ��˰��ʵ�� %d %s\n",
				currentuser->userid, num, MONEY_NAME, 
				num - tax, MONEY_NAME);
			deliverreport(title, buf);
			mcLog("Ӯ��", num, "777��");
		}
		pressanykey();
	}
	return 0;
}

static int
money_big()
{
	int i, ch, quit = 0, target, win, num, sum;
	unsigned int t[3];
	char buf[256];

	if (myInfo->cash < 200000) {
		showAt(13, 4,
		       "����б�۳���һ�ۣ����ְ�����ס����һ��������Ҳ��������ң������ͷ�󣡣���",
		       YEA);
		return 0;
	}
	move(7, 4);
	sprintf(buf, "�볡��\033[1;32m10��\033[m%s����ȷ��Ҫ��ȥ��",
		MONEY_NAME);
	if (askyn(buf, NA, NA) == NA) {
		prints("\n�ݰݣ�С����");
		return 0;
	}
	myInfo->cash -= 100000;
	mcEnv->prize777 += after_tax(100000);
	clear();
	money_show_stat("�ĳ�������");
	move(4, 0);
	prints
	    ("    �����Ǵ����ң�����Ķ�ע������ĸ�Ҫ�����Ը��ӵĴ̼�����\n"
	     "    ��ط�һ����Ǯ��Ȩ�Ĳ��ܽ��������Ͻ�Ѻ�ɣ�\n"
	     "    Ŷ�����ˣ�Ӯ�˵Ļ�Ҫ��ȡ10����С�ѵġ�");
	pressanykey();

	while (!quit) {
		clear();
		money_show_stat("�ĳ�������");
		move(4, 0);
		prints("    ��������飺�ִ�С���ţ�4-10����С��11-17��Ϊ��\n"
		       "    ��ѺС��С������һ���ʽ�Ѻ��ľ�ȫ��ׯ�ҡ�\n"
		       "    ׯ��Ҫ��ҡ��ȫ�����������ӵ���һ������ͨ�Դ�С�ҡ�\n"
		       "    \033[1;31m�����׬���������⣬�����֣�Ը�ķ���!\033[m");
		move(t_lines - 1, 0);
		prints("\033[1;44m ѡ�� \033[1;46m [1]��ע [Q]�뿪\033[m");
		win = 0;
		ch = igetkey();
		switch (ch) {
		case '1':
			update_health();
			if (check_health(3, 12, 4, "�������������ˣ�", YEA))
				break;
			sprintf(buf, "\033[1;32m��\033[m%s", MONEY_NAME);
			num = userInputValue(9, 4, "ѹ", buf, 10, 1000);
			if (num == -1)
				break;
			getdata(11, 4, "ѹ��(L)����С(S)��[L]", genbuf, 3,
				DOECHO, YEA);
			if (genbuf[0] == 'S' || genbuf[0] == 's')
				target = 1;
			else
				target = 0;
			sprintf(genbuf,
				"�� \033[1;31m%d\033[m \033[1;32m��\033[m%s�� \033[1;31m%s\033[m��ȷ��ô��",
				num, MONEY_NAME, target ? "С" : "��");
			move(12, 4);
			if (askyn(genbuf, NA, NA) == NA)
				break;
			move(13, 4);
			if (myInfo->cash < (num * 10000)) {
				showAt(13, 4, "ȥȥȥ��û��ô��Ǯ��ʲô�ң�",
				       YEA);
				break;
			}
			myInfo->cash -= (num * 10000);
			myInfo->health -= 3;
			myInfo->Actived += 3;
			for (i = 0; i < 3; i++) {
				getrandomint(&t[i]);
				t[i] = t[i] % 6 + 1;
			}
			sum = t[0] + t[1] + t[2];
			if ((t[0] == t[1]) && (t[1] == t[2])) {
				mcEnv->prize777 += after_tax(MIN_XY(num * 10000, 10000000));
				sprintf(genbuf, "\033[1;32mׯ��ͨɱ��\033[m");
				mcLog("�ڶĳ����ұ�ͨɱ������", num, "��");
			} else if (sum <= 10) {
				sprintf(genbuf, "%d �㣬\033[1;32mС\033[m",
					sum);
				if (target == 1)
					win = 1;
			} else {
				sprintf(genbuf, "%d �㣬\033[1;32m��\033[m",
					sum);
				if (target == 0)
					win = 1;
			}
			sleep(1);
			prints("���˿���~~  %d %d %d  %s", t[0], t[1], t[2],
			       genbuf);
			move(14, 4);
			if (win) {
				myInfo->cash += 19000 * num;
				mcEnv->prize777 -= 10000 * num;
				mcEnv->Treasury += 1000 * num;
				prints("��ϲ������ȡС��%d%s~~ ����һ�Ѱɣ�",
				       1000 * num, MONEY_NAME);
				mcLog("�ڶĳ�����Ӯ��", num, "��");
			} else{
				mcEnv->prize777 += after_tax(10000 * num);
				prints("û�й�ϵ�������Ӯ�����ٲ��ø�С����...");
				mcLog("�ڶĳ���������", num, "��");
			}
			pressanykey();
			break;
		case 'Q':
		case 'q':
			quit = 1;
			break;
		}
		limit_cpu();
	}
	return 0;

}

static int
money_gamble()
{
	int ch, quit = 0;
	char uident[IDLEN + 1];

	while (!quit) {
		money_show_stat("�ĳ�����");
		move(6, 4);
		prints("%s�ĳ���������𣬴�Ҿ��˰���", CENTER_NAME);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1]���� [2]777 [3]MONKEY [4]��Ϸ�齱 [5]������ [6]����칫��[Q]�뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			money_dice();
			break;
		case '2':
			money_777();
			break;
		case '3':
			monkey_business();
			break;
		case '4':
			game_prize();
			break;
		case '5':
			//guess_number();
			break;
		case '6':
			whoTakeCharge(3, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			nomoney_show_stat("�ĳ�����칫��");
			showAt(12, 4, "\033[1;32m���ڽ����С�\033[m", YEA);
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
	}
	return 0;
}

//  ----------------------------    ����    --------------------------  // 
static int
forceGetMoney(int type)
{
	int money, cost_health;
	time_t currtime;
	char uident[IDLEN + 1], buf[256], place[STRLEN];
	char *actionDesc[] = { "����", "����", "����", "���ֿտ�", NULL };
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;

	if ((type == 2 && myInfo->robExp < 50)
	    || (type == 3 && myInfo->begExp < 50)) {
		move(10, 8);
		prints("�㻹û���㹻��%s����%s��ô������顣",
		       type % 2 ? "��" : "��ʶ", actionDesc[type]);
		pressanykey();
		return 0;
	}
	move(4, 4);
	switch (type) {
	case 0:
		strcpy(place, "������");
		break;
	case 1:
		strcpy(place, "���ĵض�");
		break;
	case 2:
		strcpy(place, "����С��");
		break;
	case 3:
		strcpy(place, "��ҵ��");
		break;
	}
	prints("������%s��%s��ʵ����%s�ĺõط���", CENTER_NAME, place,
	       actionDesc[type]);
	if (!getOkUser("��Ҫ��˭���֣�", uident, 6, 4)) {
		move(7, 4);
		prints("���޴���");
		pressanykey();
		return 0;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(7, 4, "ţħ���������š��������񾭲�������", YEA);
		return 0;
	}
	move(7, 4);
	if ((type % 2 == 0 && !seek_in_file(ROBUNION, currentuser->userid))
	    || (type % 2 == 1 && !seek_in_file(BEGGAR, currentuser->userid))) {
		prints("��ô����Ҳ�����ǻ�%s���˰���", actionDesc[type]);
		pressanykey();
		return 0;
	}
	if (!t_search(uident, NA, 1)) {
		if (type == 0)
			prints("�㿴�����˰ɣ��չ�����˲���%s����", uident);
		else if (type == 1)
			prints("%s���ڼң������˰�����Ҳû��Ӧ��", uident);
		else if (type == 2)
			prints("����С��ȷʵ���尡����Ȼһ���˶�û�С�������");
		else
			prints("ʮ��·�����������������˰���û�ҵ�Ҫ�ҵ�%s��",
			       uident);
		pressanykey();
		return 0;
	}
	cost_health = 5 + (type % 2 * 10);
	if (check_health(cost_health, 12, 4, "��������ô���������°���", YEA))
		return 0;

	currtime = time(NULL);
	if (currtime < 1200 + myInfo->lastActiveTime) {
		if (type % 2 == 0)
			prints
			    ("������껵�£��˼Ҽ������¡�%sһ�������ԶԶ�Ķ㿪�ˣ�ѹ����������ߡ�",
			     uident);
		else if (type == 1)
			prints
			    ("%sŭ���ɶ������������Ҫ���ģ������ˣ������������",
			     uident);
		else		//���ֿտ� 
			prints
			    ("����Ҫ���֣��������������˺����������ҵ�Ǯ�������ˣ��� \n    %sһ�������ϰ�Ǯ������ס�ˡ���",
			     uident);
		pressanykey();
		return 0;
	}
	myInfo->lastActiveTime = currtime;
	update_health();
	myInfo->health -= cost_health;
	myInfo->Actived += 5;
	myInfo->luck--;
	switch (type) {
	case 0:
		prints("���������ָ�׵�, ����%s�ٺټ�Ц��: ����ȡ��·�ѣ���\n",
		       uident);
		break;
	case 1:
		prints
		    ("�����%s�޺����������õ�廨�ɡ����Ѿ��ü�����û�Է��ˣ��Һö�������\n",
		     uident);
		break;
	case 2:
		prints
		    ("������Ƭ������%s��ݺݵĺ�������IC~IP~IQ~~����ͳͳ���������룡����\n",
		     uident);
		break;
	case 3:
		prints
		    ("�㲻����ɫ�Ŀ���%s����֪�����İ������������¶�������\n",
		     uident);
		break;
	}

	sleep(1);
	sethomefile(buf, uident, "mc.save");
	if (!file_exist(buf))
		initData(1, buf);
	mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
	if (mcuInfo == (void *) -1)
		return 0;

	if (random() % 2 || mcuInfo->luck - myInfo->luck >= 100) {
		if (type % 2 == 1) {
			prints("    %sһ�Ű����߿�������������л���һ��ȥ����\n",
			       uident);
		} else {
			prints
			    ("    %s����һ�Ű����߷ɡ��ߵ����������Ʋ���������ٵģ�һ�㼼��������û�С���\n",
			     uident);
		}
		prints("\n    ����һ�죬�Ͻ���������ܿ���");
		pressanykey();
		return 0;
	}

	money = MIN_XY(mcuInfo->cash / 10, 100000);
	move(8, 4);
	if (money == 0) {
		prints("%s����ûǮ������������浹ù...", uident);
		goto UNMAP;
	}
	if (type == 0) {
		if (random() % 3 && currtime > mcuInfo->insuranceTime) {
			mcuInfo->cash -= money;
			myInfo->cash += money;
			myInfo->robExp += 1;
			prints
			    ("%s�ŵ�ֱ�����, �Ͻ��������ó� %d %s���㡣\n\n\033[32m    ��ĵ�ʶ�����ˣ�\033[m",
			     uident, money, MONEY_NAME);
			if (myInfo->luck > -100) {
				myInfo->luck -= 1;
				prints("\n\033[31m    �����Ʒ�����ˣ�");
			}
			sprintf(genbuf, "�㱻%s ������ %d%s��̫�����ˡ�",
				currentuser->userid, money, MONEY_NAME);
			sprintf(buf, "���⵽����");
			mcLog("������", money, uident);
		} else {
			prints("�����о�������ˣ������������\n");
			pressanykey();
			return 0;
		}

	} else if (type == 1) {
		if (random() % 3 && currtime > mcuInfo->insuranceTime) {
			mcuInfo->cash -= money;
			myInfo->cash += money;
			myInfo->begExp += 1;
			prints
			    ("%s��Ȧ��ʱ���ˣ��Ͻ��������ó� %d %s��������ӹ�һ�䡣\n\n\032[m    ���������ˣ�\033[m",
			     uident, money, MONEY_NAME);
			if (myInfo->luck > -100) {
				myInfo->luck -= 1;
				prints("\n\033[31m    �����Ʒ�����ˣ�");
			}
			sprintf(genbuf,
				"��һʱ���ģ�����%d%s��%s�����˶仨 ���������ǹ�β�Ͳݡ�����",
				money, MONEY_NAME, currentuser->userid);
			sprintf(buf, "����������С��");
			mcLog("����׬��", money, uident);
		} else {
			prints("���ۣ��ǹ����ˣ����ܰ���\n");
			pressanykey();
			return 0;
		}
	} else if (type == 2) {
		if (((myInfo->robExp >= (mcuInfo->robExp * 5 + 50)) || (random() % 4)) 
				&& currtime > mcuInfo->insuranceTime) {
			money = MIN_XY(mcuInfo->cash / 2, 10000000);
			mcuInfo->cash -= money;
			myInfo->cash += money;
			myInfo->robExp += 5;
			prints
			    ("%s�ŵð����ϵ�%d%sȫ���˳������޵���������ǧ����ɫ������\n\n\033[32m    ��ĵ�ʶ�����ˣ�\033[m",
			     uident, money, MONEY_NAME);
			if (myInfo->luck > -100) {
				myInfo->luck = MAX_XY((myInfo->luck - 5), -100);
				prints("\n\033[31m    �����Ʒ�����ˣ�");
			}
			sprintf(genbuf,
				"�������ٷˣ��� %s ���� %d%s�������������ᰡ��\n",
				currentuser->userid, money, MONEY_NAME);
			sprintf(buf, "�㱻����");
			mcLog("���ٵ�", money, uident);
		} else {
			money = MIN_XY(myInfo->cash / 2, 10000000);
			mcuInfo->cash += money;
			myInfo->cash -= money;
			myInfo->robExp = MAX_XY((myInfo->robExp - 10), 0);
			prints
			    ("%s���Ų�æ�ĴӶ����ͳ�һ����ǹ����ס�����ţ��������Ǯ�������ɡ�\n\n    ����������֮ͽ����ʧ��%d%s...\n\n\033[31m    ��ĵ�ʶ�����\033[m",
			     uident, money, MONEY_NAME);
			sprintf(genbuf,
				"�ٷ� %s ����ڳԺڣ����� %d%s����ϲ�㡫\n",
				currentuser->userid, money, MONEY_NAME);
			sprintf(buf, "��ڳԺڳɹ�");
			mcLog("���������ڳԺڣ���ʧ", money, uident);
		}
	} else {
		if (((myInfo->begExp >= (mcuInfo->begExp * 5 + 50)) || (random() % 4))
				&& currtime > mcuInfo->insuranceTime){
			money = MIN_XY(mcuInfo->cash / 2, 10000000);
			mcuInfo->cash -= money;
			myInfo->cash += money;
			myInfo->begExp += 5;
			prints
			    ("�������ɹ��ˣ����õ�Ƭ��%s���¶�����һ������,͵��%d%s��\n\n\033[32m    ���������ˣ�\033[m",
			     uident, money, MONEY_NAME);
			if (myInfo->luck > -100) {
				myInfo->luck = MAX_XY((myInfo->luck - 5), -100);
				prints("\n\033[31m    �����Ʒ�����ˣ�");
			}
			sprintf(genbuf,
				"��ȥ��������������������ڵ�ӰԺ��͵�� %d%s�������������ᰡ��\n",
				money, MONEY_NAME);
			sprintf(buf, "����������");
			mcLog("͵��", money, uident);
		} else {
			money = MIN_XY(myInfo->cash / 2, 10000000);
			mcuInfo->cash += money;
			myInfo->cash -= money;
			myInfo->begExp = MAX_XY((myInfo->begExp - 10), 0);
			prints
			    ("�������ɹ��ˣ����õ�Ƭ��%s���¶�����һ�����ӣ�͵��...�ף�\n    һ����������21������ȱʲô������������\n    ���������壬��ʧ��%d%s...\n\n\033[31m    ������󽵣�\033[m",
			     uident, money, MONEY_NAME);
			sprintf(genbuf,
				"%s ͵�����ɷ�ʴ���ף�����˳��ǣ������ %d%s���ٺ١�\n",
				currentuser->userid, money, MONEY_NAME);
			sprintf(buf, "�ƻ���ľ�ɹ�");
			mcLog("�����������壬��ʧ", money, uident);
		}
	}

	system_mail_buf(genbuf, strlen(genbuf), uident, buf,
			currentuser->userid);
      UNMAP:saveData(mcuInfo, sizeof (struct mcUserInfo));
	pressanykey();
	return 1;
}

static int
RobPeople(int type)
{
	int transcash, transcredit, transrob, transbeg;
	time_t currtime;
	char uident[IDLEN + 1], buf[256], title[70], content[256];
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;						
	
	switch (type) {
	case 0:
		showAt(4, 4, "�ڰ��Ա����Ҫ�޶�����", YEA);
        	if (!getOkUser("������˭��", uident, 5, 4)) {
			showAt(7, 4, "���޴���", YEA);
			return 0;   
		}
		if (!strcmp(uident, currentuser->userid)) {
			showAt(7, 4, "ţħ���������š��������񾭲�������", YEA);
			return 0;
		}
		move(6, 4);
		if (askyn("��ȷ��Ҫ�������", NA, NA) == NA) {
			showAt(7, 4, "ʲô���㻹û��ã����������������", YEA);
			return 0;
		}
		update_health();
		if (check_health(100, 7, 4, "��û����ô��������ܱ��ˡ�", YEA)) {
			return 0;
		}
		if (myInfo->robExp < 1000 || myInfo->begExp < 1000) {
			showAt(7, 4, "��û���㹻�ľ���������ô������顣", YEA);
			return 0;
		}
		if (myInfo->luck < 80) {
			showAt(7, 4, "��������ǰ�ƣ�����ʱ�̼������Ļ����û���ж�", YEA);
			return 0;
		}
		currtime = time(NULL);
		if (currtime <myInfo->lastActiveTime + 7200) {
			showAt(7, 4, "������˸����ӣ����Ǽ������һ�·����ɡ�", YEA);
			return 0;
		}
		showAt(7, 4, "�㿴���о���������ס������Ѳ�ߡ�", YEA);
		myInfo->lastActiveTime = currtime;
		myInfo->luck -= 10;
		if (random() % 5 != 0) {
			showAt(8, 4, "�㱻��ȡ���˰���ж���", YEA);
			sprintf(title, "���ڰ�����ڲ߻�����ж�");
			sprintf(content, "    ���Ҫ���С�ġ�\n");
			deliverreport(title, content);
			return 0;
		}
		if (askyn("�㻹Ҫ�������а���ж���", NA, NA) == NA) {
			showAt(8, 4, "�㱻��ȡ���˰���ж���", YEA);
			myInfo->luck -= 10;
			myInfo->health -= 50;
			sprintf(title, "���ڰ����Ԥı��ܱ���");
			sprintf(content, "    �ҿ�����" MY_BBS_NAME "����©�˷������ڴ�ҵĽ̻�������а�����ˡ�\n");
			deliverreport(title, content);
			return 0;
		}
		myInfo->Actived += 10;
		if (random() % 2 == 0) {
			myInfo->robExp -= 50;
			myInfo->begExp -= 50;
			myInfo->health = 0;
			myInfo->luck -= 20;
			myInfo->freeTime = time(NULL) + 10800;
			myInfo->credit += myInfo->cash;
			myInfo->cash = 0;
			move(9, 0);
			prints("    �㱻����ץס�ˣ�\n"
				"    ��ĵ�ʶ��������50�㣡\n"
				"    ���������Ϊ0��\n"
				"    �����Ʒ�½�20�㣡\n"
				"    �㱻�����Ѻ3Сʱ��\n"
				"    ��͵�����ɷ�ʴ���ף������������ᰡ��");
			sprintf(title, "���ڰ%s���ʧ��", currentuser->userid);
			sprintf(content, "    �ڰ��Ա%s��ͼ������ˣ��Һñ����켰ʱ���֡�\n"
					"    ���������Ѻ3Сʱ��\n", currentuser->userid);
			deliverreport(title, content);
			saveData(myInfo, sizeof (struct mcUserInfo));
			saveData(mcEnv, sizeof (struct MC_Env));
			pressanykey();
			mcLog("���ʧ��", 0, uident);
			Q_Goodbye();
		}
		sethomefile(buf, uident, "mc.save");
		if (!file_exist(buf))
			initData(1, buf);
		mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			return 0;
		myInfo->robExp += 50;
		myInfo->begExp += 50;
		myInfo->luck -= 20;
		transcash = MIN_XY(myInfo->cash, mcuInfo->cash/5);
		transcredit = MIN_XY(myInfo->credit, mcuInfo->credit/5);
		if (currtime < mcuInfo->insuranceTime) {
			transcash /= 2;
			transcredit /= 2;
		}
		mcuInfo->cash -= transcash;
		mcuInfo->credit -= transcredit;
		myInfo->cash += after_tax(transcash);
		myInfo->credit += after_tax(transcredit);
		myInfo->health = 0;
		move(9, 0);
		prints("    ���ܳɹ���\n"
			"    ��ĵ�ʶ��������50�㣡\n"
			"    �����Ʒ�½�20�㣡\n");
		if (currtime < mcuInfo->insuranceTime)
			prints("    ��Ͷ���ˣ���ʧ����һ�룡\n");
		prints("    ���ó���%d������\n"
			"    ��������ô����£��������ˡ�", transcash + transcredit);
		mcLog("��ܳɹ�������ֽ�", transcash, uident);
		mcLog("��ܳɹ�����ô��", transcredit, uident);
		sprintf(title, "���ڰ�ڰ���ʵʩһ�ΰ���ж�");
		if (currtime < mcuInfo->insuranceTime)
			sprintf(content, "    ĳ���̲��ұ��ڰ��ܡ�\n"
					"    �Һ���Ͷ���ˣ������˲Ʋ���1/10��Ϊ����Ѳ��ػ����ɡ�");
		else
			sprintf(content, "    ĳ���̲��ұ��ڰ��ܡ�\n"
					"    �����˲Ʋ���1/5��Ϊ����Ѳ��ػ����ɡ�\n");
		saveData(mcuInfo, sizeof (struct mcUserInfo));
		deliverreport(title, content);
		sprintf(title, "�㱻�ڰ��ܣ�");
		sprintf(content, "    �㲻�ұ��ڰ��ܣ�����%d�������", transcash + transcredit);
		system_mail_buf(content, strlen(content), uident, title, currentuser->userid);
		pressanykey();
		break;
	case 1:
		showAt(4, 4, "ؤ�������������ѹ�������������Ǵ������Ǻÿ��ģ�", YEA);
		if (!getOkUser("��Ҫ��˭���֣�", uident, 5, 4)) {
			showAt(7, 4, "���޴���", YEA);
			return 0;   
		}                    
	        if (!strcmp(uident, currentuser->userid)) {
			showAt(7, 4, "ţħ���������š��������񾭲�������", YEA);
			return 0;   
		}
		move(6, 4);
		if (askyn("��ȷ��Ҫ����ʩչ���Ǵ���", NA, NA) == NA) {
			showAt(7, 4, "Ŷ���㻹���������֣����ĳ���Ӳ�������ɡ�", YEA);
			return 0;
		}
		update_health();
		if (check_health(100, 7, 4, "��û����ô������ʩչ���Ǵ󷨡�", YEA)) {
			return 0;
		}
		if (myInfo->robExp < 1000 || myInfo->begExp < 1000) {
			showAt(7, 4, "����񹦻�����򣬼��������ɡ�", YEA);
			return 0;
		}
		if (myInfo->luck < 80) {
			showAt(7, 4, "�������Ӽ��߰ߣ���������ӽ�����û�����֡�", YEA);
			return 0;
		}
		currtime = time(NULL);
		if (currtime < myInfo->lastActiveTime + 7200) {
			showAt(7, 4, "�㻹��Ҫ�����˹�����ʩչ���Ǵ󷨡�", YEA);
			return 0;
		}
		showAt(7, 4, "��о�����������Ϣ���ѵ��������˽�������������", YEA);
		myInfo->lastActiveTime = currtime;
		myInfo->luck -= 10;
		if (random() % 5 != 0) {
			showAt(8, 4, "�������ʩչ���Ǵ󷨵���ͷ��", YEA);
			sprintf(title, "��ؤ��������������Ǵ�");
			sprintf(content, "    ���Ҫ���С�ģ���Ҫ���˵���\n");
			deliverreport(title, content);
			return 0;
		}
		if (askyn("�㻹Ҫ�ٳ���һ����", NA, NA) == NA) {
			showAt(8, 4, "�������ʩչ���Ǵ󷨵���ͷ��", YEA);
			myInfo->luck -= 20;
			myInfo->health -= 50;
			sprintf(title, "��ؤ�������ʩչ���Ǵ�");
			sprintf(content, "    �ҿ�����������û�м��������캦����¡�\n");
			deliverreport(title, content);
			return 0;
		}
		myInfo->Actived += 10;
		sethomefile(buf, uident, "mc.save");
		if (!file_exist(buf))
			initData(1, buf);
		mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			return 0;								
		if (random() % 2 == 0) {
			myInfo->robExp -= 50;
			myInfo->begExp -= 50;
			myInfo->health = 0;
			myInfo->luck -= 20;
			myInfo->lastActiveTime = time(NULL) + 7200;
			mcuInfo->robExp += 10;
			mcuInfo->begExp += 10;
			move(9, 0);
			prints("    ���ã��㱻�������������������ˣ�\n"
				"    ��ĵ�ʶ��������50�㣡\n"
				"    ���������Ϊ0��\n"
				"    �����Ʒ�½�20�㣡\n"
				"    ��͵�����ɷ�ʴ���ף������������ᰡ��");
			sprintf(title, "��ؤ�%sʩչ���Ǵ�ʧ��", currentuser->userid);
			sprintf(content, "    ؤ���Ա%s��ͼʹ�����Ǵ���ȡ���˹���������������˷������ˡ�\n"
					"    ��Ҫ����4Сʱ�����ٴ�ʩչ���Ǵ󷨡�\n", currentuser->userid);
			deliverreport(title, content);
			sprintf(title, "�㽫%s���ˣ�", currentuser->userid);
			sprintf(content, "    %s��ͼ�����Ǵ󷨶Ը��㣬������������ˡ�\n"
					"    ��ĵ�ʶ������������10�㣡\n", currentuser->userid);
			system_mail_buf(content, strlen(content), uident, title, currentuser->userid);
			pressanykey();
			mcLog("���Ǵ�ʧ��", -50, uident);
			return 0;
		}
		if (mcuInfo->robExp < 900 || mcuInfo->begExp < 900) {
			myInfo->luck -= 10;
			move(9, 0);
			prints("    �Է���ô���㾹Ȼ���������֣�\n"
				"    �����Ʒ�½�10�㣡");
			sprintf(title, "��ؤ�%sʩչ���Ǵ�δ��", currentuser->userid);
			sprintf(content, "    ؤ���Ա%s���˽������壬ʹ�����Ǵ�"
					"�Ը����ߣ���Ʒ�½�10�㣡\n", currentuser->userid);
			deliverreport(title, content);
			pressanykey();
			mcLog("���Ǵ󷨶Ը�����", -10, uident);
			saveData(mcuInfo, sizeof (struct mcUserInfo));
			return 0;
		}
		transrob = MIN_XY(myInfo->robExp, mcuInfo->robExp/5);
		transbeg = MIN_XY(myInfo->begExp, mcuInfo->begExp/5);						
		myInfo->robExp += transrob;
		mcuInfo->robExp -= transrob;
		myInfo->begExp += transbeg;
		mcuInfo->begExp -= transbeg;
		myInfo->luck -= 20;
		myInfo->health = 0;
		move(9, 0);
		prints("    ��ʩչ���Ǵ󷨳ɹ���\n"
			"    ��ĵ�ʶ����%d�㣬������%d�㣡\n"
			"    �����Ʒ�½�20�㣡\n"
			"    ��������ô����£��������ˡ�", transrob, transbeg);
		mcLog("���Ǵ󷨳ɹ�����õ�ʶ", transrob, uident);
		mcLog("���Ǵ󷨳ɹ��������", transbeg, uident);
		saveData(mcuInfo, sizeof (struct mcUserInfo));
		sprintf(title, "��ؤ�%s���ҳ�Ϊ���Ǵ󷨵��ܺ���", uident);
		sprintf(content, "    %s������ʧ��1/5��\n", uident);
		deliverreport(title, content);
		sprintf(title, "�㱻���Ǵ����У�");
		sprintf(content, "    �㲻�ұ���ʩ�����Ǵ󷨣���ʧ��%d�ĵ�ʶ��%d����",
				transrob, transbeg);
		system_mail_buf(content, strlen(content), uident, title, currentuser->userid);
		pressanykey();
		break;
	}
	return 1;
}

static int
money_pat()
{
	int num, count = 0;
	float r, x, y;
	char uident[IDLEN + 1], buf[256];
	struct userec *lookupuser;
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;

	sprintf(buf, "%s�ڰ�", CENTER_NAME);
	money_show_stat(buf);
	move(4, 4);
	if (check_health(5, 12, 4, "�������������ˣ�", YEA))
		return 0;
	prints("����İ�ש�ʵ���������ȥ����һ��ʹ�졣\n"
	       "    ���ڴ�����һ���ש�� %d %s��", BRICK_PRICE, MONEY_NAME);
	move(6, 4);
	usercomplete("��Ҫ��˭:", uident);
	if (uident[0] == '\0')
		return 0;
	if (!getuser(uident, &lookupuser)) {
		showAt(7, 4, "�����ʹ���ߴ���...", YEA);
		return 0;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(7, 4, "ţħ���������š��������񾭲�������", YEA);
		return 0;
	}
	if (t_search(uident, NA, 1)) {
		showAt(7, 4, "���Ѿ����˷�������û�����֣�ֻ�����ա�", YEA);
		return 0;
	}
	count = userInputValue(7, 4, "��", "��ש", 1, 100);
	if (count < 0)
		return 0;
	num = count * BRICK_PRICE;
	if (myInfo->cash < num) {
		move(9, 4);
		prints("����Ǯ����...��Ҫ %d %s", num, MONEY_NAME);
		pressanykey();
		return 0;
	}
	if (myInfo->luck > -100) {
		myInfo->luck = MAX_XY((myInfo->luck - 1), -100);
		prints("\033[31m    �����Ʒ�����ˣ�\033[m");
	}
	mcLog("����", num, "ԪǮ�İ�ש");
	myInfo->cash -= num;
	mcEnv->prize777 += after_tax(num);	//��ש��Ǯ����777 
	move(10, 8);
	prints("���������͵���͸��٣��㷢��ÿ������7��10��%s��·��Ƨ����",
	       uident);
	move(11, 4);
	prints("���ǵء����������������İ�ש��׼���ж���...");
	move(12, 8);
	prints("��ש�Ǻ�Σ�յ�ร� �㲻�û�������ģ�С�İ���");
	move(13, 4);
	if (askyn("�ϻ���˵���㻹����ô��", YEA, NA) == NA) {
		move(15, 4);
		myInfo->robExp = MAX_XY((myInfo->robExp - 1), 0);
		update_health();
		myInfo->health--;
		myInfo->Actived++;
		prints
		    ("��������ͷ�㺦���ˣ����Բ����ˡ�\n    ��ĵ�ʶ�����ˡ�");
		pressanykey();
		return 0;
	}
#if 0
	x = countexp(currentuser);
	y = countexp(lookupuser);
#endif
	sethomefile(buf, uident, "mc.save");
	if (!file_exist(buf))
		initData(1, buf);
	mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
	if (mcuInfo == (void *) -1)
		return -1;
	x = myInfo->robExp * myInfo->robExp + 1;
	y = mcuInfo->robExp * mcuInfo->robExp;
	r = x / (x + y + 1) * 100;
	num = count * BRICK_PRICE/2 + 1000;	//ҽҩ�� 
	update_health();
	myInfo->health -= 5;
	myInfo->Actived += 2;
	move(16, 4);
	if (r + count < 101) {	//Ŀ�굨ʶ�����Լ�Լ10���ͻ�ط������ٺ� 
		prints("�ܲ��ң���û�����С�����������С�Դ���...");
		move(17, 4);
		prints("����Ѫ��ֹ�����͵�ҽԺ����ʮ���룬�òҰ���");
		move(18, 4);
		if (random() % 2) {
			myInfo->robExp = MAX_XY((myInfo->robExp - 1), 0);
			prints("��ĵ�ʶ�����ˡ�\n    ");
		}
		if (random() % 2) {
			myInfo->robExp = MAX_XY((myInfo->robExp - 1), 0);
			prints("����������ˡ�\n    ");
		}
		mcEnv->prize777 += after_tax(MIN_XY(myInfo->cash, num * 2));
		myInfo->cash = MAX_XY((myInfo->cash - num * 2), 0);
		update_health();
		myInfo->health = 0;
		prints("��󻹽��� %d %s��ҽҩ�ѣ������Ժ󻹸Ҳ���", num * 2,
		       MONEY_NAME);
		pressanykey();
		mcLog("��שʧ�ܣ�����", num*2, "��ҽ�Ʒ�");
		return 0;
	}
	if (myInfo->begExp > mcuInfo->begExp * 20 + 50) {
		myInfo->luck--;
		prints("�Է���ô�����Ȼ�������֣�\n    �����Ʒ�����ˡ�");
		pressanykey();
		return 0;
	}
	if (mcuInfo->guard > 0 && rand() % 3) {
		prints("б����ͻȻ���һֻ���ǹ�����һ���񣬰�שȫ�ҿ��ˡ�\n"
		       "    �ۣ����ǹ������˹����ˣ�");
		if (askyn("��Ҫ��Ҫ�ܣ�", YEA, NA) == YEA) {
			if (random() % 2) {
				myInfo->robExp =
				    MAX_XY((myInfo->robExp - 1), 0);
				prints("��ĵ�ʶ�����ˡ�\n    ");
			}
			if (random() % 4) {
				prints
				    ("\n    ��һ������ɿ�����һ��С���ܵ��ˡ����հ���");
				pressanykey();
				return 0;
			} else
				prints
				    ("\n    ���Ȼ�ܽ���һ������ͬ���ǹ��������ˡ�����");
		}

		if (random() % 3) {
			mcuInfo->guard--;
			myInfo->health -= 5;
			sleep(1);
			prints("\n    �������Ҳ����������ڸɵ��˴��ǹ�������");
			sprintf(buf,
				"���һֻ���ǹ���%s�ɵ��ˣ������ڻ�ʣ%dֻ���ǹ���",
				currentuser->userid, mcuInfo->guard);
			system_mail_buf(buf, strlen(buf), uident,
					"���һֻ���ǹ�׳������",
					currentuser->userid);
			if (!(random() % 4)) {
				myInfo->robExp++;
				prints("\n    ��ĵ�ʶ�����ˣ�");
			}
			prints("\n    �����������������½���");
			mcLog("�ɵ����ǹ�", 1, uident);
		} else {
			sleep(1);
			prints
			    ("\n    �����������������Ǳ����ǹ�������ҧ��һ�ڡ�");
			if (!(random() % 3)) {
				if (myInfo->begExp) {
					myInfo->begExp--;
					prints("\n    ����������ˣ�");
				}
				update_health();
				myInfo->health = myInfo->health / 2;
				prints("\n    ����������룡");
				sprintf(buf,
					"%s����Ĵ��ǹ�ҧ�ˣ��ǵý����ǹ������ͷŶ��",
					currentuser->userid);
				system_mail_buf(buf, strlen(buf), uident,
						"��Ĵ��ǹ��ɹ�������",
						currentuser->userid);
			}
		}
		goto UNMAP;
	}
	if ((random() % 5 || (myInfo->begExp > mcuInfo->begExp * 10 + 50)) &&
	    !(mcuInfo->begExp > myInfo->begExp * 10 + 50)) {
		prints("���⻵��������͵Ϯ������%s��С�Դ��ϡ�", uident);
		if (mcuInfo->cash < num) {
			move(17, 4);
			update_health();
			mcuInfo->health = 0;
			mcLog("��ש�ɹ�������ҽ�Ʒ�", mcuInfo->cash, uident);
			mcEnv->prize777 += after_tax(mcuInfo->cash);
			mcuInfo->cash = 0;
			prints("�㶼�ĵ��˼�ûǮ��������...�������°ɣ�");
			sprintf(buf,
				"�㱻%s���˰�ש����ûǮ���ˣ�ֻ��ҧ����ʹ...",
				currentuser->userid);
			if (!(random() % 5)) {
				myInfo->robExp++;
				prints("\n    ��ĵ�ʶ�����ˣ�");
			}
			if (!(random() % 5)) {
				myInfo->begExp++;
				prints("\n    ����������ˣ�");
			}
		} else {
			mcEnv->prize777 += after_tax(num);
			mcuInfo->cash -= num;
			mcuInfo->health -= 10;
			mcLog("��ש�ɹ�������ҽ�Ʒ�", num, uident);
			if (!(random() % 5))
				mcuInfo->begExp = MAX_XY(0, mcuInfo->begExp--);
			move(17, 4);
			prints("������%s����%d%s���ˣ���ҽԺ�����˺ö��죡",
			       uident, num, MONEY_NAME);
			sprintf(buf, "�㱻%s���˰�ש������%d%s���ˣ�������...",
				currentuser->userid, num, MONEY_NAME);
			if (random() % 2) {
				myInfo->robExp++;
				prints("\n    ��ĵ�ʶ�����ˣ�");
			}
			if (random() % 2) {
				myInfo->begExp++;
				prints("\n    ����������ˣ�");
			}
		}
		system_mail_buf(buf, strlen(buf), uident, "�㱻���˰�ש",
				currentuser->userid);
	} else {
		if (random() % 2) {
			mcuInfo->begExp++;
			sprintf(buf,
				"%s�ð�ש������գ�����������ˣ�ŶҮ��\n",
				currentuser->userid);
		} else {
			mcuInfo->robExp++;
			sprintf(buf,
				"%s�ð�ש������գ���ĵ�ʶ�����ˣ�ŶҮ��\n",
				currentuser->userid);
		}
		mcLog("��ש���", count, uident);
		system_mail_buf(buf, strlen(buf), uident, "������שϮ��",
				currentuser->userid);
		prints("��ѽѽ��û���С�����");

	}

      UNMAP:saveData(mcuInfo, sizeof (struct mcUserInfo));
	pressanykey();
	return 0;
}

static void
RobShop()
{
	int num;
	char buf[256], title[STRLEN];
	time_t currtime;

	if (check_health(90, 12, 4, "��ô�������û�г��������޷���ɡ�", YEA))
		return;
	if (myInfo->robExp < 100) {
		showAt(12, 4, "����ԥ�˰��죬����û�Ҷ��֡�����", YEA);
		return;
	}
	if (myInfo->begExp < 50) {
		showAt(12, 4,
		       "����붯�֣������վ�����ͱ����ϵĵ��Ӱ��ˡ�����",
		       YEA);
		return;
	}
	if (myInfo->luck < 60) {
		showAt(12, 4, "���ܾ��ñ�����˫�۾��ڶ����㿴��", YEA);
		return;
	}

	nomoney_show_stat("����");
	move(4, 4);
	currtime = time(NULL);
	if (currtime < 3600 + myInfo->lastActiveTime) {
		showAt(12, 4,
		       "�ղų��˸����ӣ��о����ڸ���Ѳ�ӣ��Ȳ�Ҫ����Ϊ�á�",
		       YEA);
		return;
	}

	if (askyn("��װ���پ�Ҫ��������װ����Ҫ20����ȷ��Ҫ����", NA, NA) ==
	    NA) {
		myInfo->robExp--;
		prints("\n    ����������ˡ�\n    ��ĵ�ʶ���ͣ�");
		return;
	}
	if (myInfo->cash < 200000) {
		showAt(12, 4, "һ�ֽ�Ǯһ�ֽ�������û���㹻���ֽ�", YEA);
		return;
	}
	update_health();
	myInfo->health -= 10;
	prints("\n    ������%s�Ƴ��ǹ������%s��С��ף��ۣ�˧���ˣ�",
	       CENTER_NAME, CENTER_NAME);
	pressanykey();

	nomoney_show_stat("�ĳ�����");
	move(4, 4);
	if (askyn("���ĳ���Σ�յģ������Ҫ������", NA, NA) == NA) {
		myInfo->robExp -= 2;
		prints
		    ("\n    ����������ˣ���͵͵��װ���ӽ��˸���������Ͱ��\n    ��ĵ�ʶ���ͣ�");
		pressanykey();
		mcLog("�������ٶĳ�", 0, "");
		return;
	}

	myInfo->lastActiveTime = currtime;
	update_health();
	myInfo->health -= 50;
	myInfo->luck -= 10;
	myInfo->Actived += 10;

	move(6, 4);
	prints
	    ("��һ���������ǹ�����ŶĿʹ󺰣���ι�������㣬û�����������𣿣���");
	pressanykey();

	sleep(1);

	move(8, 4);
	if (random() % 2) {
		myInfo->robExp -= 10;
		prints
		    ("��Χ���˹�����Ц���񾭲������ð�ˮǹ���𰡣���\n    �������ڵ�ƭ�ˣ���ת����ܡ�\n    �����������\n    ��ĵ�ʶ����10�㡣");
		sprintf(buf,
			"    һ������ˮǹ����׵��񾭲��ڶĳ���������ݡ�\n"
			"    ϣ����֪�������򾯷��ṩ�й���Ϣ��\n");
		deliverreport("�����š��񾭲��ĳ�����", buf);
		pressreturn();
		mcLog("���ٶĳ������񾭲�", 0, "");
		return;
	}

	update_health();
	if (random() % 3) {
		num = MIN_XY(2000000, mcEnv->prize777) / 2;
		myInfo->cash += num - 200000;
		mcEnv->prize777 -= num - after_tax(200000);
		myInfo->begExp += 30;
		myInfo->robExp += 30;
		prints
		    ("\n    ������Χ����Ŀ�ɿڴ�֮�ʣ����ó�������777̨�ϵ�Ǯ����װ����\n    Ȼ��������һת���߳����š�\n    ��ĵ�ʶ���ӣ�\n    ��������ӣ�");
		sprintf(title, "�����š���������װ���ٶĳ�");
		sprintf(buf,
			"    ��վ�ո��յ�����Ϣ����Щʱ��һ��������װ���ٶĳ������ݡ�\n"
			"    Ŀǰ������ʽ���������¡�ϣ�����������ṩ���������\n");
		mcLog("���ٶĳ��ɹ������", num, "");
	} else {
		myInfo->health = 0;
		myInfo->robExp -= 20;
		myInfo->luck -= 20;
		prints
		    ("\n    ˢˢˢ����Χ����һ��ȫ����װ�ľ��죬��ֻ�����־��ܣ�\n    ԭ��������͵õ����߱���cmft\n"
		     "    �������ȫʧ��\n    �����Ʒ���ͣ�\n    ��ĵ�ʶ����20��!\n");
		sprintf(title, "�����š������������ %s���־���",
			currentuser->userid);
		sprintf(buf,
			"    �����������ȵõ����߱������%s�ĳ�����%s��װ���ٵ�ʱ�򣬳ɹ������ܻ�\n"
			"    ����Ѷ�����з���%sΪ���񲡻��ߣ����������������ϣ�ȡ����ҽ��\n",
			CENTER_NAME, currentuser->userid, currentuser->userid);
		mcLog("���ٶĳ�ʧ��", 0, "");
	}
	deliverreport(title, buf);
	pressanykey();

	return;

}
static int
money_robber()
{
	int ch, quit = 0, tempMoney;
	char uident[IDLEN + 1], title[70], content[256];

	while (!quit) {
		money_show_stat("�ڰ��ܲ�");
		move(4, 4);
		prints
		    ("����ǰ��%s�ڰ��޶���������һʱ���������ϴ����һ��ʱ�������伣��"
		     "\n    ����������𽥻�Ծ�����������ֶ�Ҳ�������λ���������",
		     CENTER_NAME);
		move(7, 4);
		prints
		    ("һ���������߹���С��˵����Ҫ��שô�����˺��۵ġ��ĺ��˻��ܳ���ʶ������");
		move(t_lines - 1, 0);
#if 0
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1]��ש [2]�������� [3]�ڰ� [4] �������� [Q]�뿪\033[m");
#else
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1]��ש [2]�ڰ� [3] �������� [4] �������� [Q]�뿪\033[m");
#endif
		ch = igetkey();
		switch (ch) {
		case '1':
			money_pat();
			break;
#if 0
		case '2':
			money_show_stat("�ڰﱣ�����շѴ�");
			move(6, 4);
			prints
			    ("��ν�������֣����˱����ѣ����ܱ�һ��ʱ��ƽ�������ںڰ��ж�����֮�ڡ�");
			showAt(12, 4, "\033[1;32m���ڲ߻��С�\033[m", YEA);
			break;
#endif
		case '2':
			if (!seek_in_file(ROBUNION, currentuser->userid)) {
				move(12, 4);
				prints("�㲻�Ǻڰ�ģ���������������");
				pressanykey();
				break;
			}
			if (seek_in_file(ROBUNION, currentuser->userid)
				&& seek_in_file(BEGGAR, currentuser->userid)) {
				move(12, 4);
				prints
				    ("�����ˣ��ꡫ����ڰ��ؤ��ˮ���ݣ����̤��ֻ�������ǲ�Ҫ��¶Ϊ�á�");
				pressanykey();
				break;
			}
			while (!quit) {
				money_show_stat("�ڰ�");
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m ѡ�� \033[1;46m [1]���� [2]���� [3]�ٶĳ� [4]��Ʊ [5]�˳��ڰ� [Q]�뿪\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					money_show_stat("������");
					forceGetMoney(0);
					break;
				case '2':
					money_show_stat("���컯��");
					forceGetMoney(2);
					break;
				case '3':
					money_show_stat("�ĳ�");
					RobShop();
					break;
				case '4':
					money_show_stat("����");
					RobPeople(0);
					break;
				case '5':
					money_show_stat("�̻���");
					quitsociety(0);
					quit = 1;
					break;
				case 'q':
				case 'Q':
					quit = 1;
					break;
				}
			}
			quit = 0;
			break;
		case '3':
			whoTakeCharge(4, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			while (!quit) {
				nomoney_show_stat("�ڰ����");
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m ѡ�� \033[1;46m [1]������� [2]�ڳԺ� [3]�鿴�����ʲ� [Q]�뿪\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					nomoney_show_stat("�����");
					showAt(12, 4,
					       "\033[1;32m���ڽ����С�\033[m",
					       YEA);
					break;
				case '2':
					money_show_stat("�ڳԺ�");
					showAt(12, 4,
					       "\033[1;32m���������С�\033[m",
					       YEA);
//forcerobMoney(2); 
					break;
				case '3':
					money_show_stat("С���");
					showAt(12, 4,
					       "\033[1;32m������ڵ�Ǯ�����Ժ�\033[m",
					       YEA);
					break;
				case 'q':
				case 'Q':
					quit = 1;
					break;
				}
			}
			quit = 0;
			break;
		case '4':
			money_show_stat("�����ѽ��ѵ�");
			move(12, 4);
			if (askyn("\033[1;32m����ڰﵽ����٣���Ҫ��Ҫ���������㣿\033[m", YEA, NA)==YEA){
				tempMoney = userInputValue(13, 4, "Ҫ����","��", 5, 100) * 10000;
				if (tempMoney < 0) break;
				if (myInfo->cash < tempMoney){
					showAt(15, 4, "\033[1;37m�㵨��Ϸˣ�ڰ�ǲ��ǻ�Ĳ��ͷ��ˣ�\033[m", YEA);
					break;
				}
				update_health();
				if (check_health(1, 15, 4, "������������ˣ�", YEA))
					break;
				move(15, 4);
				prints("\033[1;37m�㽻��%d%s������\n"
				       "    ��ĵ�ʶ������%d�㣡\033[m\n", 
				       tempMoney, MONEY_NAME, tempMoney / 50000);
				mcLog("���ɱ�����", tempMoney, "Ԫ");
				myInfo->health--;
				myInfo->Actived += tempMoney / 25000;
				myInfo->cash -= tempMoney;
				myInfo->robExp += tempMoney / 50000;
				mcEnv->Treasury += tempMoney;
				if (tempMoney == 100000 &&
					(!seek_in_file(ROBUNION, currentuser->userid))) {
					if(askyn("    ��Ҫ����ڰ���", YEA, NA) == NA)
						break;
					if(seek_in_file(BEGGAR, currentuser->userid)) {
						showAt(17, 4, "���Ѿ���ؤ���Ա�ˣ�\n", NA);
						if(askyn("    ��Ҫ�˳�ؤ����", YEA, NA) == NA)
							break;
						del_from_file(BEGGAR, currentuser->userid);
						mcLog("�˳�ؤ��", 0, "");
						showAt(19, 4, "���Ѿ��˳�ؤ���ˡ�", YEA);
					}
					if(seek_in_file(POLICE, currentuser->userid)) {
						showAt(17, 4, "���Ѿ��Ǿ����ˣ�    \n", NA);
						if(askyn("    ��Ҫ�˳����������", YEA, NA) == NA)
							break;
						del_from_file(POLICE, currentuser->userid);
						myInfo->luck = MAX_XY(-100, myInfo->luck - 50);
						mcLog("�˳�����", 0, "");
						showAt(19, 4, "���Ѿ������Ǿ����ˡ�", YEA);
					}
					addtofile(ROBUNION, currentuser->userid);
					showAt(20, 4, "���Ѿ������˺ڰ", NA);
					sprintf(title, "���ڰ%s����ڰ�", currentuser->userid);
					sprintf(content, "    ���С�ģ������ܻ����ĳ����߰�������̣�\n");
					deliverreport(title, content);
					mcLog("����ڰ�", 0, "");
				}
				pressanykey();
			}else{
				showAt(14, 4, "\033[1;33m�����˴�ٿɱ�˵��û������㣡\033[m", YEA);
			}
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
		limit_cpu();
	}
	return 0;
}

static void
stealbank()
{
	int num;
	struct userec *lookupuser;
	char uident[IDLEN + 1], buf[256], title[STRLEN];
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;
	time_t currtime;

	if (check_health(90, 12, 4, "��ô�������û�г��������޷���ɡ�", YEA))
		return;
	if (myInfo->robExp < 50) {
		showAt(12, 4, "���±��¡���������ֶ���̫�����ˡ�����", YEA);
		return;
	}
	if (myInfo->begExp < 100) {
		showAt(12, 4, "ƾ�����ڵ������������ɣ���ȥ�����ɡ�����",
		       YEA);
		return;
	}
	if (myInfo->luck < 60) {
		showAt(12, 4, "��������������ؼ����鱨������", YEA);
		return;
	}

	currtime = time(NULL);
	if (currtime < 3600 + myInfo->lastActiveTime) {
		showAt(12, 4, "����ϵͳ�ոջ������㻹Ҫ����ʱ����Ϥһ�¡�",
		       YEA);
		return;
	}
	move(4, 4);
        if (askyn("��������ϵͳ��Ҫ������ԣ��������Ҫ20����ȷ��Ҫ����"
			, NA, NA) == NA) {
		myInfo->begExp--;
		prints("\n    ����������ˡ�\n    ��������ͣ�");
		return;
	}
	myInfo->health -= 10;
	
	if (myInfo->cash < 200000) {
		showAt(12, 4, "һ�ֽ�Ǯһ�ֽ�������û���㹻���ֽ�", YEA);
		return;
	}
		 
	myInfo->lastActiveTime = currtime;
	update_health();
	myInfo->health /= 2;
	myInfo->luck -= 10;
	myInfo->Actived += 10;
	move(5, 4);
	prints
	    ("�����ڵ���ǰ�棬���ݵĵ���һ���̣�Ȼ��������˼��¼��̡�\n    ��Ļ����ʾ����ʼ����%s����ϵͳ��",
	     CENTER_NAME);
	pressanykey();
	move(7, 4);
	prints("�������ӡ��������Ժ�");
	sleep(1);

	move(8, 4);
	if (random() % 2) {
		myInfo->begExp -= 10;
		prints
		    ("��ལ����еĳ��������������������֣���Ͻ��Ͽ������ӡ����գ�\n    �����������\n    ���������10�㡣");
		sprintf(buf,
			"    ����������ոռ��ӵ���һ����������ϵͳ����Ϊ������ͼ��������������ʱ��\n"
			"    �����������ѡ�ϣ����֪�������򾯷��ṩ�й���Ϣ��\n");
		deliverreport("�����š�������ʿ��������ϵͳʧ��", buf);
		pressreturn();
		mcLog("��������ʧ��", 0, "");
		return;
	}
	prints
	    ("��ϲ�㣡�Ѿ��ɹ����ӵ�������ת��ϵͳ��\n    ����һ�λ���ӱ��˵��ʻ�ת�ʵ����ʻ���\n");
	usercomplete("    ��Ҫ��˭����ת�ʹ�������Enterȡ����", uident);
	if (uident[0] == '\0') {
		myInfo->robExp -= 10;
		prints
		    ("\n    �㵨�ӷ����ˡ�������\n    �����������\n    ��ĵ�ʶ����10�㡣");
		return;
	}
	if (!getuser(uident, &lookupuser)) {
		myInfo->begExp -= 10;
		showAt(12, 4,
		       "\n    ����һ������ˣ������ȿ��������ڿ�������\n    �����������\n    ���������10�㡣",
		       YEA);
		return;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(7, 4, "��Ŷ����ɽҽԺ��ӭ��������", YEA);
		myInfo->robExp -= 10;
		myInfo->begExp -= 10;
		myInfo->luck -= 10;
		return;
	}
	sethomefile(buf, uident, "mc.save");
	if (!file_exist(buf))
		initData(1, buf);
	mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
	if (mcuInfo == (void *) -1)
		return;
	num =
	    MIN_XY(MIN_XY(MAX_MONEY_NUM / 2, mcuInfo->credit / 5),
		   myInfo->credit);
	sprintf(buf, "Ҫ��%s���ʻ�ת����", uident);
	num = userInputValue(12, 4, buf, MONEY_NAME, 0, num);
	if (num < 0) {
		myInfo->robExp -= 20;
		prints("\n    �㵨�ӷ����ˡ�������\n    �����������\n    ��ĵ�ʶ����20�㡣");
		return;
	}
	if ((num > (myInfo->credit + myInfo->cash) * 5)
	    || (myInfo->begExp * 5 + 50 < mcuInfo->begExp))
		if (random() % 4) {
			sleep(1);
			goto FAILED;
		}
	sleep(1);
	update_health();
	if (random() % 3) {
		if (currtime < mcuInfo->insuranceTime)
			num /= 2;
		mcuInfo->credit -= num;
		myInfo->credit += after_tax(num);
		myInfo->cash -= 200000;
		mcEnv->prize777 += after_tax(200000);
		myInfo->begExp += 30;
		myInfo->robExp += 30;
		sprintf(buf, "�ǵþ���ȥ��������ʻ�������Ŷ��");
		system_mail_buf(buf, strlen(buf), uident, "��������",
				"deliver");
		prints
		    ("\n    ��ϲ�㣡ת�ʳɹ���\n    ��ĵ�ʶ���ӣ�\n"
		       "    ��������ӣ�\n    ��������ľ���\n");
		if (currtime < mcuInfo->insuranceTime)
			prints("    ������Ͷ���ˣ���ֻ�õ���һ���Ǯ��");
		sprintf(title, "��ҥ�ԡ�����������������ת��ϵͳ");
		sprintf(buf,
			"    �ݷ���������ҥ�ԣ����˳ɹ�����������ϵͳ��\n"
			"    �����еĳ�����������޼��ӵ�������Ϊ��\n"
			"    Ŀǰ�������ڵ������Ϣ����ʵ�ȡ�\n"
			"    ���λ��ע�Լ����ʻ�����ʱ�ṩ�����Ϣ��\n");
		mcLog("�������гɹ������", num, uident);
	} else {
	      FAILED:myInfo->health = 0;
		myInfo->begExp -= 20;
		myInfo->luck = MAX_XY(-100, myInfo->luck - 20);
		num = MIN_XY(MAX_MONEY_NUM / 2, myInfo->credit / 5);
		if (currtime < myInfo->insuranceTime)
			num /= 2;
		mcuInfo->credit += after_tax(num / 2);
		mcEnv->prize777 += num / 2;
		myInfo->credit -= num;
		prints
		    ("    ������������ӵ�����Ŀ����ж����㱻ץס�ˣ�\n    ����������㣡\n"
		     "    �����Ʒ����20�㣡\n    ���������20��!\n");
		if (currtime < myInfo->insuranceTime)
			prints("    �Һ���Ͷ���ˣ��㱻����%d%s!", num, MONEY_NAME);
		else
			prints("    �㱻����%d%s!", num, MONEY_NAME);
		sprintf(title, "�����š�%s��������ϵͳ��ץ��",
			currentuser->userid);
		sprintf(buf,
			"    ����������ոռ��ӵ���%s��������ϵͳ����Ϊ�����ɹ�����������\n"
			"    ����������̬�ȽϺã������β����˵�ԭ�򡣷���%d%s!\n"
			"    ����һ����ܺ�����Ϊѹ���ѣ�һ��ע��777��\n"
			"    �������Ϊ�䣡\n",
			currentuser->userid, num, MONEY_NAME);
		mcLog("�������б�ץ����ʧ", num, uident);
	}
	deliverreport(title, buf);
	saveData(mcuInfo, sizeof (struct mcUserInfo));
	pressanykey();

	return;

}

static int
money_beggar()
{
	char ch, quit = 0, uident[IDLEN + 1], title[70], content[256];
	void *buffer = NULL;
	int tempMoney;
	struct mcUserInfo *mcuInfo;

	while (!quit) {
		money_show_stat("ؤ���ܶ�");
		showAt(4, 4,
		       "ؤ���Թ����µ�һ������ƶ�����Խ��Խ������ؤ����Ҳ�����ˡ�\n"
		       "    Ϊ�����ƣ��ⲻ����͵���������ɹ�ƭ֮�£���ȻҲ�нٸ���ƶ֮�١�\n\n"
		       "    һ����ؤ�߹����ʵ�����Ҫ������Ϣô��ؤ�����ϵ���������֪��������������",
		       NA);
		move(t_lines - 1, 0);
#if 0
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1]��̽ [2]��ë�� [3]ؤ�� [4]�������� [Q]�뿪\033[m");
#else
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1]��̽ [2]ؤ�� [3]�������� [4]�ȼ����� [Q]�뿪\033[m");
#endif
		ch = igetkey();

		switch (ch) {
		case '1':
			update_health();
			if (check_health
			    (2, 12, 4, "Ъ��Ъ�ᣬ��־����Ͳ�Ҫ���ˣ�", YEA))
				break;
			if (!getOkUser("��˭�ļҵף�", uident, 8, 4))
				break;
			if (myInfo->cash < 1000) {
				showAt(9, 4, "������ֻ������ô��Ǯ��", YEA);
				break;
			}
			myInfo->cash -= 1000;
			mcEnv->Treasury += 1000;
			update_health();
			myInfo->health -= 2;
			myInfo->Actived++;
			sethomefile(genbuf, uident, "mc.save");
			if (!file_exist(genbuf))
				initData(1, genbuf);
			mcuInfo =
			    loadData(genbuf, buffer,
				     sizeof (struct mcUserInfo));
			if (mcuInfo == (void *) -1)
				break;
			move(10, 4);
			prints
			    ("\033[1;31m%s\033[m ��Լ�� \033[1;31m%d\033[m ��%s���ֽ�"
			     "���Լ� \033[1;31m%d\033[m ��%s�Ĵ�", uident,
			     mcuInfo->cash / 100000 * 10, MONEY_NAME,
			     mcuInfo->credit / 100000 * 10, MONEY_NAME);
			move(11, 4);
			prints
			    ("\033[1;31m%s\033[m �ĵ�ʶ \033[1;31m%d\033[m  �� "
			     "\033[1;31m%d\033[m  ��Ʒ \033[1;31m%d\033[m  ���� "
			     "\033[1;31m%d\033[m  ���ǹ� \033[1;31m%d\033[m ��\n",
			     uident, mcuInfo->robExp, mcuInfo->begExp,
			     mcuInfo->luck, MAX_XY(100, mcuInfo->health),
			     mcuInfo->guard);
			mcLog("��̽", 0, uident);
			if (myInfo->cash < 10000) {
				saveData(mcuInfo, sizeof (struct mcUserInfo));
				pressanykey();
				break;
			}
			move(13, 4);
			if (askyn("��Ҫ��Ӷ˽����̽����������ϸ���������", NA, NA) == YEA) {
				myInfo->cash -= 10000;
				mcEnv->Treasury += 10000;
				move(14, 4);
				prints("\033[1;31m%s\033[m �� \033[1;31m%d\033[m %s���ֽ�"
				       "\033[1;31m%d\033[m %s�Ĵ�\n    "
				       "\033[1;31m%s\033[m ���� \033[1;31m%d\033[m %s����Ϣ��"
				       "\033[1;31m%d\033[m %s�Ĵ��\n    "
				       "�Լ� \033[1;31m%d\033[m %s���ڱ������",
				       uident, mcuInfo->cash, MONEY_NAME,
				       mcuInfo->credit, MONEY_NAME,
				       uident, mcuInfo->interest, MONEY_NAME,
				       mcuInfo->loan, MONEY_NAME,
				       mcuInfo->safemoney, MONEY_NAME);
			}
			saveData(mcuInfo, sizeof (struct mcUserInfo));
			pressanykey();
			break;
#if 0
		case '2':
			money_show_stat("������");
			move(6, 4);
			prints
			    ("�в������ⲻ�����˰������ۺ졣��������ؤ����Ϣ��ͨ��\n"
			     "Ԥ��һ�ʼ�ë���ʷѣ����˰������ʱ�򣬾��ܻ�֪��ë�ţ�\n"
			     "ʹ�ð�Ȼ���ļ������ӡ�");
			showAt(12, 4, "\033[1;32m���ڲ߻��С�\033[m", YEA);
			break;
#endif
		case '2':
			if (!seek_in_file(BEGGAR, currentuser->userid)) {
				move(12, 4);
				prints("���¹���������ô����ؤ��������");
				pressanykey();
				break;
			}
			if (seek_in_file(ROBUNION, currentuser->userid)
				&& seek_in_file(BEGGAR, currentuser->userid)) {
				move(12, 4);
				prints
				    ("�����ˣ��ꡫ����ڰ��ؤ��ˮ���ݣ����̤��ֻ�������ǲ�Ҫ��¶Ϊ�á�");
				pressanykey();
				break;
			}
			while (!quit) {
				money_show_stat("ؤ��");
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m ѡ�� \033[1;46m [1]���� [2]���ֿտ� [3] ������� [4]���Ǵ� [5] �˳�ؤ�� [Q]�뿪\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					money_show_stat("������");
					forceGetMoney(1);
					break;
				case '2':
					money_show_stat("��ҵ��");
					forceGetMoney(3);
					break;
				case '3':
					money_show_stat("�ڿ͵۹�");
					stealbank();
					break;
				case '4':
					money_show_stat("�չ�С��");
					RobPeople(1);
					break;
				case '5':
					money_show_stat("������");
					quitsociety(1);
					quit = 1;
					break;
				case 'q':
				case 'Q':
					quit = 1;
					break;
				}
			}
			quit = 0;
			break;
		case '3':
			whoTakeCharge(5, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			while (!quit) {
				nomoney_show_stat("ؤ�����");
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m ѡ�� \033[1;46m [1]��ƶ�� [2]�򹷰��� [3]�鿴�����ʲ� [Q]�뿪\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					nomoney_show_stat("������");
					showAt(12, 4,
					       "\033[1;32m���ڽ����С�\033[m",
					       YEA);
					break;
				case '2':
					money_show_stat("�����޹�");
					showAt(12, 4,
					       "\033[1;32m��ϰ�С�\033[m", YEA);
//forcerobMoney(2); 
					break;
				case '3':
					money_show_stat("С���");
					showAt(12, 4,
					       "\033[1;32m������ڵ�Ǯ�����Ժ�\033[m",
					       YEA);
					break;
				case 'q':
				case 'Q':
					quit = 1;
					break;
				}
			}
			quit = 0;
			break;
		case '4':
			money_show_stat("����");
			move(12, 4);
			if(askyn("\033[1;33m���´󺵣��������ա���Ҫ��Ҫ�ȼ�һ�����ˣ�\033[m", YEA, NA)==YEA){
				tempMoney = userInputValue(13, 4, "Ҫ����","��", 5, 100) * 10000;
				if (tempMoney < 0) break;
				if (myInfo->cash < tempMoney){
					showAt(15, 4, "\033[1;37mûǮ����ʲô���ˡ�\033[m", YEA);
					break;
				}
				update_health();
				if (check_health(1, 15, 4, "������������ˣ�", YEA))
					break;
				move(15, 4);
				prints("\033[1;37m������˷���%d%s����ʳ\n"
				       "    �����������%d�㣡\033[m\n", 
				       tempMoney, MONEY_NAME, tempMoney / 50000);
				mcLog("�ȼ���������", tempMoney, "Ԫ����ʳ");
				myInfo->health--;
				myInfo->Actived += tempMoney / 25000;
				myInfo->cash -= tempMoney;
				myInfo->begExp += tempMoney / 50000;
				mcEnv->Treasury += tempMoney;
				if (tempMoney == 100000 && 
					(!seek_in_file(BEGGAR, currentuser->userid))) {
					if(askyn("    ��Ҫ����ؤ����", YEA, NA) == NA)
						break;
					if(seek_in_file(ROBUNION, currentuser->userid)) {
						showAt(17, 4, "���Ѿ��Ǻڰ��Ա�ˣ�\n", NA);
						if(askyn("    ��Ҫ�˳��ڰ���", YEA, NA) == NA)
							break;
						del_from_file(ROBUNION, currentuser->userid);
						mcLog("�˳��ڰ�", 0, "");
						showAt(19, 4, "���Ѿ��˳��˺ڰ", YEA);
					}
					if(seek_in_file(POLICE, currentuser->userid)) {
						showAt(17, 4, "���Ѿ��Ǿ�Ա�ˣ�    \n", NA);
						if(askyn("    ��Ҫ�˳����������", YEA, NA) == NA)
							break;
						del_from_file(POLICE, currentuser->userid);
						myInfo->luck = MAX_XY(-100, myInfo->luck - 50);
						mcLog("�˳�����", 0, "");
						showAt(19, 4, "���Ѿ������Ǿ����ˡ�", YEA);
					}
					addtofile(BEGGAR, currentuser->userid);
					showAt(20, 4, "���Ѿ�������ؤ�", NA);
					sprintf(title, "��ؤ�%s����ؤ��", currentuser->userid);
					sprintf(content, "    ���С�ģ������ܻ������л���ʩչ���Ǵ󷨣�\n");
					deliverreport(title, content);
					mcLog("����ؤ��", 0, "");
				}
				pressanykey();
			}else{
				showAt(14, 4, "\033[1;32m����̨��Ҫ��ô��Ǯ��ʲô�ã�\033[m", YEA);
			}
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
		limit_cpu();
	}
	return 0;
}

//---------------------------------  ����  ---------------------------------// 

static int
userSelectStock(int n, int x, int y)
{
	int stockid;
	char buf[8];

	sprintf(genbuf, "�������Ʊ�Ĵ���[0-%d  \033[1;33mENTER\033[m����]:",
		n - 1);
	while (1) {
		move(x, y);
		getdata(x, y, genbuf, buf, 3, DOECHO, YEA);
		if (buf[0] == '\0')
			return -1;
		stockid = atoi(buf);
		if (stockid >= 0 && stockid < n)
			return stockid;
	}
}

static int
stockAdmin(void *stockMem, int n)
{				//������� 
	int ch, quit = 0, stockid;
	struct BoardStock *bs;

	while (!quit) {
		nomoney_show_stat("֤���");
		move(4, 4);
		prints("�����ǹ�����еĻ�����������������Թ�������");
		showAt(6, 0, "        1. ��Ʊ����        2. ����/�رչ���\n"
		             "        3. ��Ʊ����        4. ��ͣ/�ָ���Ʊ Q. �˳�\n\n"
		             "    ��ѡ���������: ", NA);
		ch = igetkey();
		update_health();
		if (check_health
		    (1, 12, 4, "��Ҫ̫���࣬�ȱ�����Ъһ��ɣ�", YEA))
			continue;
		switch (ch) {
		case '1':
			myInfo->health--;
			myInfo->Actived++;
			newStock(stockMem, n);
			break;
		case '2':
			if (!mcEnv->stockOpen)
				sprintf(genbuf, "%s", "ȷ��Ҫ����������");
			else
				sprintf(genbuf, "%s", "ȷ��Ҫ�رչ�����");
			move(10, 4);
			if (askyn(genbuf, NA, NA) == NA)
				break;
			mcEnv->stockOpen = !mcEnv->stockOpen;
			mcLog(mcEnv->stockOpen?"�رչ���":"��������", 0, "");
			update_health();
			myInfo->health--;
			myInfo->Actived++;
			showAt(11, 4, "������ɡ�", YEA);
			break;
		case '3':
			if ((stockid = userSelectStock(n, 10, 4)) == -1)
				break;
			bs = stockMem + stockid * sizeof (struct BoardStock);
			sprintf(genbuf,
				"\033[5;31m���棺\033[0mȷ��Ҫ����Ʊ %s ����ô",
				bs->boardname);
			move(11, 4);
			if (askyn(genbuf, NA, NA) == YEA) {
				bs->timeStamp = 0;	//��Ʊ������־ 
				bs->status = 3;
				deleteList(stockid);
				showAt(12, 4, "������ɡ�", YEA);
			}
			mcLog("��Ʊ����", 0, bs->boardname);
			update_health();
			myInfo->health--;
			myInfo->Actived++;
			break;
		case '4':
			if ((stockid = userSelectStock(n, 10, 4)) == -1)
				break;
			bs = stockMem + stockid * sizeof (struct BoardStock);
			sprintf(genbuf, "ȷ��Ҫ\033[1;32m%s\033[0m��Ʊ %s ��",
				(bs->status == 3) ? "�ָ�" : "��ͣ",
				bs->boardname);
			move(11, 4);
			if (askyn(genbuf, NA, NA) == YEA) {
				mcLog((bs->status == 3) ? "�ָ�����":"��ͣ����",
						0, bs->boardname);
				bs->status = (bs->status == 3) ? 0 : 3;
				showAt(12, 4, "������ɡ�", YEA);
			}
			update_health();
			myInfo->health--;
			myInfo->Actived++;
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
	}
	return 0;
}

static int
trendInfo(void *stockMem, int n)
{				//��ʾ�������� 
	int i, yeste, today, change, stockid;
	char buf[STRLEN];
	struct BoardStock *bs;
	time_t currTime = time(NULL);
	struct tm *thist = localtime(&currTime);

	today = thist->tm_wday;
	yeste = (today <= 1) ? 6 : (today - 1);	//��������, ����һ������������ 
	change = mcEnv->stockPoint[today] - mcEnv->stockPoint[yeste];
	nomoney_show_stat("��ʱ���ӹ�����");
	sprintf(buf, "��ǰָ��: %d        %s: %d��", mcEnv->stockPoint[today],
		change > 0 ? "��" : "��", abs(change));
	showAt(4, 4, buf, NA);
	showAt(6, 0,
	       "                  ��һ\t  �ܶ�\t  ����\t  ����\t  ����\t  ����\n\n"
	       "    \033[1;36m��������\033[0m    ", NA);
	showAt(11, 0, "    \033[1;36m  �ɽ���\033[0m    ", NA);
	move(8, 16);
	for (i = 1; i < 7; i++)
		prints("%6d\t", mcEnv->stockPoint[i]);
	move(11, 16);
	for (i = 1; i < 7; i++)
		prints("%6d\t", mcEnv->tradeNum[i] / 100);
	showAt(14, 0, "    \033[1;33mһ������\033[0m    ", NA);
	while (1) {
		if ((stockid = userSelectStock(n, 20, 4)) == -1)
			break;
		bs = stockMem + stockid * sizeof (struct BoardStock);
		move(14, 16);
		for (i = 1; i < 7; i++) {
			sprintf(genbuf, "%6.2f\t", bs->weekPrice[i]);
			prints("%s", genbuf);
		}
		pressanykey();
	}
	return 0;
}

static int
tryPutStock(void *stockMem, int n, struct BoardStock *new_bs)
{				//��ͼ��һ֧�¹�Ʊ�����λ���ɹ������±�ֵ 
//����Ʊ�Ѵ��ڻ�û�п�λ, ����-1 
	int i, slot = -1;
	struct BoardStock *bs;

	for (i = 0; i < n; i++) {
		bs = stockMem + i * sizeof (struct BoardStock);
		if (!strcmp(bs->boardname, new_bs->boardname) && bs->timeStamp)
			return -1;
		if (bs->timeStamp == 0 && slot == -1)	//�ŵ���һ����λ 
			slot = i;
	}
	if (slot >= 0) {
		bs = stockMem + slot * sizeof (struct BoardStock);
		memcpy(bs, new_bs, sizeof (struct BoardStock));
	}
	return slot;
}

static int
newStock(void *stockMem, int n)
{				//��Ʊ���� 
	int i, money, num;
	float price;
	char boardname[24], buf[256], title[STRLEN];
	struct boardmem *bp;
	struct BoardStock bs;

	make_blist_full();
	move(10, 4);
	namecomplete("�����뽫Ҫ���е�������: ", boardname);
	FreeNameList();
	if (boardname[0] == '\0') {
		showAt(11, 4, "���������������...", YEA);
		return 0;
	}
	bp = getbcache(boardname);
	if (bp == NULL) {
		showAt(11, 4, "���������������...", YEA);
		return 0;
	}
	while (1) {
		getdata(11, 4, "��������ֵ[��λ:ǧ��, [1-10]]:", genbuf, 4,
			DOECHO, YEA);
		money = atoi(genbuf);
		if (money >= 1 && money <= 10)
			break;
	}
	price = bp->score / 1000.0;
	money *= 10000000;
	num = money / price;
	sprintf(buf, "    �������������� %d %s�Ĺ�Ʊ��\n"
		"    ������ %d ��, ÿ�ɼ۸� %.2f %s��\n", money, MONEY_NAME, num,
		price, MONEY_NAME);
	showAt(12, 0, buf, NA);
	move(15, 4);
	if (askyn("ȷ��������", NA, NA) == NA)
		return 0;
//��ʼ�� 
	bzero(&bs, sizeof (struct BoardStock));
	strcpy(bs.boardname, boardname);
	bs.totalStockNum = num;
	bs.remainNum = num;
	for (i = 0; i < 7; i++)
		bs.weekPrice[i] = price;
	for (i = 0; i < 4; i++)
		bs.todayPrice[i] = price;
	bs.high = price;
	bs.low = price;
	bs.timeStamp = time(NULL);
	if (tryPutStock(stockMem, n, &bs) == -1)
		append_record(DIR_STOCK "stock", &bs,
			      sizeof (struct BoardStock));
	sprintf(title, "�����С�%s���������", boardname);
	deliverreport(title, buf);
	showAt(16, 4, "������ɡ�", YEA);
	mcLog("��Ʊ���У���ֵ", money, boardname);
	mcLog("��Ʊ���У�ÿ��", price, boardname);
	return 1;
}

static void
deleteList(short stockid)
{				//ɾ��һ֧��Ʊ�ı��۶��� 
	char listpath[256];
	sprintf(listpath, "%s%d.sell", DIR_STOCK, stockid);
	unlink(listpath);
	sprintf(listpath, "%s%d.buy", DIR_STOCK, stockid);
	unlink(listpath);
}

static void
stockNews(struct BoardStock *bs)
{
	int n, rate;
	char newsbuf[STRLEN], titlebuf[STRLEN];

	n = random() % 1000;
	if (n < 20) {
		bs->status = 3;
		sprintf(newsbuf,
			"��Ʊ%s�������ݹ��۱�֤�������ֹͣ���ܽ��ס�\n",
			bs->boardname);
	} else if (n < 30) {
		rate = random() % 6 + 5;
		bs->boardscore *= (1 + rate / 100.0);
		sprintf(newsbuf, "��Ʊ%s������������Ϣ�̼��¹ɼ���������%d%%��\n",
			bs->boardname, rate);
	} else if (n < 50) {
		rate = random() % 6 + 5;
		bs->boardscore *= (1 - rate / 100.0);
		sprintf(newsbuf, "��Ʊ%s�ݴ�ӯ�����ۣ��з���ָ�ɼ۽��´�%d%%��\n",
			bs->boardname, rate);
	}
	if (n < 50) {
		sprintf(titlebuf, "�����С�%s����������Ϣ", CENTER_NAME);
		deliverreport(titlebuf, newsbuf);
		mcLog(newsbuf, 0, "");
	}
}

static void
UpdateBSStatus(struct BoardStock *bs, short today, int newday, int stockid)
{				//����һ֧��Ʊ��״̬ 

	int yeste = (today <= 1) ? 6 : (today - 1);
	float delta = bs->todayPrice[3] / bs->weekPrice[yeste];
	struct boardmem *bp;
	char buf[80];

	if (bs->status == 3)
		return;
	if (delta >= 1.1)
		bs->status = 1;	//��ͣ 
	else if (delta <= 0.9)
		bs->status = 2;	//��ͣ 
	else
		bs->status = 0;	//�ָ����� 
	if (newday) {
		deleteList(stockid);
		bs->tradeNum = 0;
		bs->sellerNum = 0;
		bs->buyerNum = 0;
		bs->weekPrice[yeste] = bs->todayPrice[3];
		memset(bs->todayPrice, 0, sizeof (float) * 4);
		bs->todayPrice[3] = bs->weekPrice[yeste];
		bp = getbcache(bs->boardname);
		if (bp != NULL) {
			bs->boardscore =
			    (bp->score + bs->boardscore * 29) / 30.0;
			stockNews(bs);
		} else {
			sprintf(buf, "û���ҵ�%s�����ݣ��޷����¹�Ʊ", bs->boardname);
			deliverreport(buf, "");
		}
	}
}

static int
updateStockEnv(void *stockMem, int n, int flag)
{				//���¹���ͳ����Ϣ 

	int i, hour, newday = 0, totalTradeNum = 0;
	float megaStockNum = 0, megaTotalValue = 0, avgPrice;
	struct BoardStock *bs;
	struct tm thist, lastt;
	time_t currTime = time(NULL);

	if (currTime < mcEnv->lastUpdateStock + 60)	//����Ƶ������ 
		return 0;
	localtime_r(&currTime, &thist);
	hour = thist.tm_hour;
	mcEnv->stockTime = (hour >= 9 && hour < 22) && !(hour >= 13
							 && hour < 15)
	    && thist.tm_wday;
	if (!mcEnv->stockTime)	//����״̬�²����� 
		return 0;
	localtime_r(&mcEnv->lastUpdateStock, &lastt);
	mcEnv->lastUpdateStock = currTime;
	if (thist.tm_wday != lastt.tm_wday || flag)	//�Զ�ɾ�����ս��׵� 
		newday = 1;
	for (i = 0; i < n; i++) {
		bs = stockMem + i * sizeof (struct BoardStock);
		if (!bs->timeStamp)	//��Ч��Ʊ 
			continue;
		megaStockNum += bs->totalStockNum / 1000000.0;
		megaTotalValue += megaStockNum * bs->todayPrice[3];
		UpdateBSStatus(bs, thist.tm_wday, newday, i);	//���¹�Ʊ״̬ 
		totalTradeNum += bs->tradeNum;
	}
// ���¹���ָ�� 
	avgPrice = megaTotalValue / megaStockNum;
//	mcEnv->stockPoint[thist.tm_wday] = avgPrice * n;
	mcEnv->stockPoint[thist.tm_wday] = bbsinfo.utmpshm->mc.ave_score;
// ���³ɽ��� 
	mcEnv->tradeNum[thist.tm_wday] = totalTradeNum;
	return 0;
}

static void
showTradeInfo(void *stockMem, int n)
{				//��ʾһ֧��Ʊ�ĵ�ǰ������Ϣ 
	int i, j, m, stockid, col, line;
	int buyerNum = 0, sellerNum = 0;
	size_t filesize;
	char listpath[2][256];
	void *buffer = NULL, *listMem;
	struct BoardStock *bs;
	struct TradeRecord *tr;

	money_show_stat("������Ϣ����");
	showAt(4, 4, "������Բ�ѯ��Ʊ�ĵ�ǰ������Ϣ��", NA);
	if ((stockid = userSelectStock(n, 6, 4)) == -1)
		return;
	bs = stockMem + stockid * sizeof (struct BoardStock);
	sprintf(genbuf, "ȷ��Ҫ��ѯ��Ʊ %s �Ľ�����Ϣô", bs->boardname);
	move(7, 4);
	if (askyn(genbuf, NA, NA) == NA)
		return;
	showAt(9, 0, "    ��ѯ�������[ǰ10��] ��\n"
	       "    [���  ���뱨��    �ཻ����]  ========  [���  ��������    �ཻ����]",
	       NA);
	sprintf(listpath[0], "%s%d.buy", DIR_STOCK, stockid);
	sprintf(listpath[1], "%s%d.sell", DIR_STOCK, stockid);
	for (i = 0; i < 2; i++) {
		col = (i == 0) ? 5 : 45;
		m = get_num_records(listpath[i], sizeof (struct TradeRecord));
		if (m <= 0) {
			showAt(11, col, "û�н��ױ��ۡ�", NA);
			continue;
		}
		filesize = sizeof (struct TradeRecord) * m;
		listMem = loadData(listpath[i], buffer, filesize);
		if (listMem == (void *) -1)
			continue;
		for (j = 0, line = 11; j < m && j < 10; j++) {	//����ʾǰ10 
			tr = listMem + j * sizeof (struct TradeRecord);
			if (tr->num <= 0)
				continue;
			sprintf(genbuf, "%4d  %8.2f    %8d", line - 11,
				tr->price, tr->num);
			showAt(line++, col, genbuf, NA);
			if (i == 0)
				buyerNum++;
			else
				sellerNum++;
		}
		saveData(listMem, filesize);
	}
	bs->buyerNum = buyerNum;	//˳��ͳ����������, ����׼ȷ 
	bs->sellerNum = sellerNum;	//��Ϊֻscan��ǰ10����¼ 
	sleep(1);
	pressanykey();
}

static void
showStockDetail(struct BoardStock *bs)
{				//��ʾһ֧��Ʊ����ϸ��Ϣ 
	char strTime[16];
	struct tm *timeStamp = localtime(&(bs->timeStamp));
	char *status[] = { "\033[1;37m����\033[0m", "\033[1;31m��ͣ\033[0m",
		"\033[1;32m��ͣ\033[0m", "\033[1;34m��ͣ\033[0m"
	};

	sprintf(strTime, "%4d-%2d-%2d", timeStamp->tm_year + 1900,
		timeStamp->tm_mon + 1, timeStamp->tm_mday);
	money_show_stat("���״���");
	move(4, 4);
	prints("�����ǹ�Ʊ %s ����ϸ��Ϣ:", bs->boardname);
	move(6, 0);
	prints("      ��������: %10s\t  ������: %10d\n"
	       "      ϵͳ�ع�: %10d\t  ɢ����: %10d\n"
	       "      ��ǰ����: %10d\t��ǰ���: %10d\n"
	       "        ������: %10d\t    ״̬:   %s\n",
	       strTime, bs->totalStockNum, bs->remainNum,
	       bs->holderNum, bs->sellerNum, bs->buyerNum, bs->tradeNum,
	       status[bs->status]);
	sprintf(genbuf,
		"       ���տ���: %7.2f\t���: %7.2f\t���: %7.2f\tƽ��: %7.2f\n"
		"       ��ʷ���: %7.2f\t���: %7.2f\n",
		bs->todayPrice[0], bs->todayPrice[1], bs->todayPrice[2],
		bs->todayPrice[3], bs->high, bs->low);
	move(11, 0);
	prints("%s", genbuf);
}

static int
getMyStock(struct mcUserInfo *mcuInfo, int stockid)
{
//��������stockid����һ֧��Ʊ 
//���û�иù�Ʊ, ��ô�ҵ�һ����λ, ��ʼ�� 
//����ֵ�ǹ�Ʊ��λ���±�, ����Ѿ�����, ��ô���� -1 
	int i, slot = -1;

	for (i = 0; i < STOCK_NUM; i++) {
		if (mcuInfo->stock[i].stockid == stockid)
			return i;
		if (mcuInfo->stock[i].num <= 0 && slot == -1)
			slot = i;
	}
	if (slot >= 0) {
		bzero(&(mcuInfo->stock[slot]), sizeof (struct myStockUnit));
		mcuInfo->stock[slot].stockid = stockid;
	}
	return slot;
}

static void
newRecord(struct BoardStock *bs, int tradeNum, float tradePrice, int tradeType)
{				//����һ֧��Ʊ����ʷ��¼ 
	time_t currTime = time(NULL);
	struct tm *thist = localtime(&currTime);

	if (tradeNum == 0)
		return;
	bs->high = MAX_XY(tradePrice, bs->high);
	bs->low = MIN_XY(tradePrice, bs->low);
	if (bs->todayPrice[0] == 0)	//ÿ�տ��̼� 
		bs->todayPrice[0] = tradePrice;
	bs->todayPrice[1] = MAX_XY(tradePrice, bs->todayPrice[1]);	//ÿ����߼� 
	bs->todayPrice[2] =
	    MIN_XY(tradePrice, (bs->todayPrice[2] == 0) ? 999 : bs->todayPrice[2]);
	bs->todayPrice[3] =
	    (tradeNum * tradePrice +
	     bs->todayPrice[3] * bs->tradeNum) / (bs->tradeNum + tradeNum);
	bs->tradeNum += tradeNum;
	mcEnv->tradeNum[thist->tm_wday] += tradeNum;
}

static int
addToWaitingList(struct TradeRecord *mytr, int stockid)
{				//��һ�������׵�������� 
	int i, n, slot = -1;
	char filepath[256];
	void *listMem, *buffer = NULL;
	struct TradeRecord *tr;

	if (mytr->tradeType == 1)
		sprintf(filepath, "%s%d.sell", DIR_STOCK, stockid);
	else
		sprintf(filepath, "%s%d.buy", DIR_STOCK, stockid);
	n = get_num_records(filepath, sizeof (struct TradeRecord));
	if (n > 128)		//filesize <= 4k 
		return 0;
	if (n <= 0) {		//ֱ��append_record 
		append_record(filepath, mytr, sizeof (struct TradeRecord));
		return 1;
	}
	listMem = loadData(filepath, buffer, sizeof (struct TradeRecord) * n);
	if (listMem == (void *) -1)
		return 0;
	for (i = 0; i < n; i++) {
		tr = listMem + i * sizeof (struct TradeRecord);
		if (!strncmp(mytr->userid, tr->userid, IDLEN)) {
			slot = i;
			break;
		}
		if (slot == -1 && tr->num <= 0)	//�ҵ���һ����Ч�Ľ������� 
			slot = i;
	}
	if (slot >= 0) {	//�滻 
		tr = listMem + slot * sizeof (struct TradeRecord);
		memcpy(tr, mytr, sizeof (struct TradeRecord));
		saveData(listMem, sizeof (struct TradeRecord) * n);
	} else {
		saveData(listMem, sizeof (struct TradeRecord) * n);
		append_record(filepath, mytr, sizeof (struct TradeRecord));
	}
	return 1;
}

static int
tradeArithmetic(struct BoardStock *bs, struct TradeRecord *trbuy,
		struct TradeRecord *trsell, struct mcUserInfo *buyer,
		struct mcUserInfo *seller)
{				//��Ʊ���׵ĺ��ĺ��� 
	int idx, slot, iCanBuyNum, tradeNum, tradeMoney;

	if (buyer->cash < trsell->price || trbuy->price < trsell->price)
		return 0;	//ûǮ��򿪼۲��� 
	idx = getMyStock(seller, trsell->stockid);
	if (idx == -1)		//�����Ѿ�û����֧��Ʊ�� 
		return 0;
	tradeNum = MIN_XY(trbuy->num, trsell->num);
	iCanBuyNum = buyer->cash / trsell->price;
	tradeNum = MIN_XY(tradeNum, iCanBuyNum);
	tradeNum = MIN_XY(tradeNum, seller->stock[idx].num);
	if (tradeNum <= 0)	//����ȷ������˫�����Խ��׵����� 
		return 0;
	tradeMoney = tradeNum * trsell->price;
	if ((slot = getMyStock(buyer, trbuy->stockid)) == -1)
		return 0;
	buyer->cash -= tradeMoney;	//�򷽿�Ǯ 
	seller->stock[idx].num -= tradeNum;	//�������й���Ҫ�� 
	if (seller->stock[idx].num <= 0)	//��������,�ֹ���-- 
		bs->holderNum--;
	if (buyer->stock[slot].num <= 0)	//������,�ֹ���++ 
		bs->holderNum++;
	buyer->stock[slot].num += tradeNum;	//������ 
	buyer->stock[slot].stockid = trbuy->stockid;
	seller->cash += 0.999 * tradeMoney;	//������Ǯ 
	trsell->num -= tradeNum;	//������������������ 
	trbuy->num -= tradeNum;	//�򷽹�������ҲҪ�� 
	return tradeNum;
}

static int
tradeWithSys(struct BoardStock *bs, struct TradeRecord *mytr, float sysPrice)
{				//�û���ϵͳ���� 
	int sysTradeNum;
	struct mcUserInfo mcuInfo;
	struct TradeRecord systr;

	if (mytr->num <= 0)	//���û���� 
		return 0;
	bzero(&systr, sizeof (struct TradeRecord));
	if (mytr->tradeType == 0) {	//�û����� 
		systr.price = MAX_XY(mytr->price, sysPrice);
		systr.num = bs->remainNum;
	} else {
		systr.price = MIN_XY(sysPrice, mytr->price);
		systr.num = mytr->num;	//ϵͳ���������й������� 
	}
//���湹��ϵͳ�Ľ��׵��ͽ����� 
	systr.stockid = mytr->stockid;
	systr.tradeType = 1 - mytr->tradeType;
	strcpy(systr.userid, "_SYS_");

	bzero(&mcuInfo, sizeof (struct mcUserInfo));
	mcuInfo.cash = 1000000000;	//ϵͳ�������㹻��Ǯ 
	mcuInfo.stock[0].stockid = mytr->stockid;
	mcuInfo.stock[0].num = bs->remainNum;	//! 
	if (mytr->tradeType == 0)	//�û����� 
		sysTradeNum =
		    tradeArithmetic(bs, mytr, &systr, myInfo, &mcuInfo);
	else
		sysTradeNum =
		    tradeArithmetic(bs, &systr, mytr, &mcuInfo, myInfo);
	if (sysTradeNum > 0) {
		bs->remainNum += mytr->tradeType ? sysTradeNum : (-sysTradeNum);
		newRecord(bs, sysTradeNum, systr.price, mytr->tradeType);
	}
	return sysTradeNum * systr.price;
}

static void
sendTradeMail(struct TradeRecord *utr, int tradeNum, float tradePrice)
{				//�ż�֪ͨ���׶Է� 
	char content[STRLEN], title[STRLEN];
	char *actionDesc[] = { "����", "����" };
	char *moneyDesc[] = { "֧��", "��ȡ" };
	int type = utr->tradeType;

	sprintf(title, "��%s��Ʊ %d �Ľ������", actionDesc[type],
		utr->stockid);
	sprintf(content, "�˴ν�����Ϊ %d ��, �ɽ��� %.2f, %s%s %d", tradeNum,
		tradePrice, moneyDesc[type], MONEY_NAME,
		(int) (tradeNum * tradePrice));
	system_mail_buf(content, strlen(content), utr->userid, title,
			currentuser->userid);
}

static int
tradeWithUser(struct BoardStock *bs, struct TradeRecord *mytr)
{				//�û���Ľ��� 
	int i, n, tradeNum, tradeMoney = 0;
	float tradePrice;
	char mcu_path[256], list_path[256];
	void *buffer_mcu = NULL, *buffer_list = NULL, *tradeList;
	struct mcUserInfo *mcuInfo;
	struct TradeRecord *utr;

	if (mytr->tradeType == 0)	//currentuser���� 
		sprintf(list_path, "%s%d.sell", DIR_STOCK, mytr->stockid);
	else
		sprintf(list_path, "%s%d.buy", DIR_STOCK, mytr->stockid);
	n = get_num_records(list_path, sizeof (struct TradeRecord));
	if (n <= 0)
		return 0;
	tradeList =
	    loadData(list_path, buffer_list, sizeof (struct TradeRecord) * n);
	if (tradeList == (void *) -1)
		return 0;
	for (i = 0; i < n && mytr->num > 0; i++) {	//���۶��� 
		utr = tradeList + i * sizeof (struct TradeRecord);
		if (utr->userid[0] == '\0' || utr->num <= 0)
			continue;
		sethomefile(mcu_path, utr->userid, "mc.save");
		mcuInfo =
		    loadData(mcu_path, buffer_mcu, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			continue;
		if (mytr->tradeType == 0)	//currentuser���� 
			tradeNum =
			    tradeArithmetic(bs, mytr, utr, myInfo, mcuInfo);
		else		//currentuser����, �൱�ڶԷ����� 
			tradeNum =
			    tradeArithmetic(bs, utr, mytr, mcuInfo, myInfo);
		if (tradeNum > 0) {	//���¼�¼��֪ͨ���׷� 
			tradePrice = mytr->tradeType ? mytr->price : utr->price;
			newRecord(bs, tradeNum, tradePrice, mytr->tradeType);
			tradeMoney += tradeNum * tradePrice;
			sendTradeMail(utr, tradeNum, tradePrice);
		}
		saveData(mcuInfo, sizeof (struct mcUserInfo));
	}
	saveData(tradeList, sizeof (struct TradeRecord));
	return tradeMoney;
}

static int
tradeInteractive(struct BoardStock *bs, int stockid, int today, int type)
{				//���׵Ľ������� 
	int idx, inList = 0;
	int iHaveNum, inputNum, userTradeNum, sysTradeNum;
	int userTradeMoney, sysTradeMoney, totalMoney;
	int yeste = (today <= 1) ? 6 : (today - 1);
	float sup = bs->weekPrice[yeste] * 1.1;
	float inf = bs->weekPrice[yeste] * 0.9;
	float inputPrice, sysPrice, guidePrice;
	char buf[256];
	char *actionDesc[] = { "����", "����", NULL };
	struct TradeRecord mytr;

	idx = getMyStock(myInfo, stockid);
	if (idx == -1 && type == 0) {
		showAt(14, 4, "���Ѿ�����10֧������Ʊ��, �����ٶ�����", YEA);
		return 0;
	}
	if (myInfo->stock[idx].num <= 0 && type == 1) {
		showAt(14, 4, "��û�г���֧��Ʊ", YEA);
		return 0;
	}
	if (bs->status == 3) {
		showAt(14, 4, "��֧��Ʊ��ֹͣ����", YEA);
		return 0;
	}
	iHaveNum = myInfo->stock[idx].num;
	move(14, 4);
	sprintf(genbuf, "Ŀǰ������ %d ����֧��Ʊ, �ع��� %.4f��.",
		iHaveNum, iHaveNum / (bs->totalStockNum / 100.0));
	prints("%s", genbuf);
	sprintf(genbuf, "������Ҫ%s�Ĺ���[100]: ", actionDesc[type]);
	while (1) {
		inputNum = 100;
		getdata(15, 4, genbuf, buf, 8, DOECHO, YEA);
		if (buf[0] == '\0' || (inputNum = atoi(buf)) >= 100)
			break;
	}
	guidePrice = bs->boardscore / 1000.0;
	if (type == 0)		//currentuser����, ϵͳҪȡ�߼��� 
		sysPrice = MAX_XY(guidePrice, bs->todayPrice[3]);
	else			//currentuser����, ϵͳ�ͼ����� 
		sysPrice = MIN_XY(0.95 * guidePrice, bs->todayPrice[3]);
	sprintf(genbuf, "������%s����[%.2f]: ", actionDesc[type], sysPrice);
	while (1) {
		inputPrice = sysPrice;
		getdata(16, 4, genbuf, buf, 7, DOECHO, YEA);
		if (buf[0] != '\0')
			inputPrice = atof(buf);
		if (inputPrice < inf - 0.01 || inputPrice > sup + 0.01) {
			showAt(17, 4, "�Բ���, ��ı��۳�����ͣ(��ͣ)��λ!",
			       YEA);
		} else
			break;
	}
	sprintf(buf, "ȷ���� %.2f �ı���%s %d �ɵ� %s ��Ʊ��?", inputPrice,
		actionDesc[type], inputNum, bs->boardname);
	move(17, 4);
	if (askyn(buf, NA, NA) == NA)
		return 0;
	if (type == 0 && myInfo->cash < inputNum * inputPrice) {
		showAt(18, 4, "����ֽ𲻹��˴ν���...", YEA);
		return 0;
	}
	if (type == 1 && myInfo->stock[idx].num < inputNum) {
		showAt(18, 4, "��û����ô���Ʊ...", YEA);
		return 0;
	}
//�����ҵĽ��׵� 
	bzero(&mytr, sizeof (struct TradeRecord));
	mytr.num = inputNum;
	mytr.tradeType = type;
	mytr.price = inputPrice;
	mytr.stockid = stockid;
	strcpy(mytr.userid, currentuser->userid);
//�û��佻�� 
	userTradeMoney = tradeWithUser(bs, &mytr);
	userTradeNum = abs(inputNum - mytr.num);
	if (userTradeNum > 0) {
		sprintf(buf, "�û�%s�� %d ��, �ɽ���ÿ�� %.2f %s",
			actionDesc[1 - type], userTradeNum,
			1.0 * userTradeMoney / userTradeNum, MONEY_NAME);
		showAt(18, 4, buf, YEA);
	}
//�������ʣ����δ���, ��ô��ϵͳ���� 
	sysTradeMoney = tradeWithSys(bs, &mytr, sysPrice);
	sysTradeNum = abs(inputNum - mytr.num - userTradeNum);
	if (sysTradeNum > 0) {
		sprintf(buf, "ϵͳ%s�� %d ��, �ɽ���ÿ�� %.2f %s",
			actionDesc[1 - type], sysTradeNum,
			1.0 * sysTradeMoney / sysTradeNum, MONEY_NAME);
		showAt(19, 4, buf, YEA);
	}
//������н϶�ʣ����δ����, ��ô���뽻�׶��� 
	if (mytr.num >= 100)
		inList = addToWaitingList(&mytr, stockid);
	move(20, 4);
	if (mytr.num < inputNum) {	//����н����� 
		totalMoney = userTradeMoney + sysTradeMoney;
		prints("���׳ɹ�! ��%s %d��, ���׽�� %d %s(��������)",
		       actionDesc[type], abs(inputNum - mytr.num), totalMoney,
		       MONEY_NAME);
	}
	if (mytr.num > 0) {	//����δ��� 
		myInfo->stock[idx].status = type + 1;
		move(21, 4);
		prints("ʣ�� %d ��δ����%s", mytr.num,
		       inList ? ", �Ѿ����뱨�۶���" : ".");
	}
	pressanykey();
	sleep(1);
	return 0;
}

static int
stockTrade(void *stockMem, int n, int type)
{				//���״������� 
	int i, j, yeste, today, pages, offset, stockid;
	float delta;
	char buf[256];
	struct BoardStock *bs;
	struct tm *thist;
	time_t currTime = time(NULL);
	char *status[] = { "\033[1;37m����\033[0m", "\033[1;31m��ͣ\033[0m",
		"\033[1;32m��ͣ\033[0m", "\033[1;34m��ͣ\033[0m"
	};

	thist = localtime(&currTime);
	today = thist->tm_wday;
	yeste = (today <= 1) ? 6 : (today - 1);
	move(4, 4);
	prints("Ŀǰ�� %d ֧��Ʊ, ��������ָ�� %d ��, ��ǰָ�� %d ��  ״̬: %s",
	       n, mcEnv->stockPoint[yeste], mcEnv->stockPoint[today],
	       mcEnv->stockTime ? "������" : "\033[1;31m������\033[0m");
	sprintf(buf, "\033[1;36m����  %20s  %8s  %8s  %8s \t%s\033[0m",
		"����", "��������", "���ճɽ�", "�Ƿ�", "״̬");
	showAt(6, 4, buf, NA);
	move(7, 4);
	prints
	    ("----------------------------------------------------------------------------------");
	pages = n / 10 + 1;
	for (i = 0;; i++) {	//i���ڿ���ҳ�� 
		for (j = 0; j < 10; j++) {	//ÿ����ʾ���10֧��Ʊ 
			offset = i * 10 + j;
			move(8 + j, 4);
			if (offset >= n || offset < 0) {
				clrtoeol();
				continue;
			}
			bs = stockMem + offset * sizeof (struct BoardStock);
			delta = bs->todayPrice[3] / bs->weekPrice[yeste] - 1;
			sprintf(buf, "[%2d]  %20s  %8.2f  %8.2f  %8.2f%%\t%s",
				offset, bs->boardname, bs->weekPrice[yeste],
				bs->todayPrice[3], delta * 100,
				status[bs->status]);
			prints("%s", buf);
			offset++;
		}
		getdata(19, 4, "[B]ǰҳ [C]��ҳ [T]���� [Q]�˳�: [C]", buf, 2,
			DOECHO, YEA);
		if (toupper(buf[0]) == 'Q')
			return 0;
		if (toupper(buf[0]) == 'T' && mcEnv->stockTime)
			break;
		if (toupper(buf[0]) == 'B')
			i = (i == 0) ? (i - 1) : (i - 2);
		else
			i = (i == pages - 1) ? (i - 1) : i;
	}
	if ((stockid = userSelectStock(n, 19, 4)) == -1)
		return 0;
	bs = stockMem + stockid * sizeof (struct BoardStock);
	showStockDetail(bs);
	tradeInteractive(bs, stockid, today, type);
	return 1;
}

static void
myStockInfo()
{				//��ʾ���˳��й�Ʊ 
	int i, count = 0;
	char *status[] = { "����", "��������", "��������", NULL };

	nomoney_show_stat("���и��˷�������");

	showAt(4, 0, "    ����������˳��й�Ʊ���: \n"
	       "    [����]\t\t��  ��  ��\t\t״  ̬", NA);
	for (i = 0; i < STOCK_NUM; i++) {
		if (myInfo->stock[i].num <= 0)
			continue;
		move(6 + count++, 0);
		prints("    [%4d]\t\t%10d\t\t%-8s", myInfo->stock[i].stockid,
		       myInfo->stock[i].num, status[myInfo->stock[i].status]);
	}
	pressreturn();
}

static int
money_stock()
{
	int n, ch, quit = 0;
	size_t filesize;
	void *buffer = NULL, *stockMem;
	char buf[256];

	clear();
	if (!mcEnv->stockOpen && !USERPERM(currentuser, PERM_SYSOP)) {
		showAt(6, 16, "\033[1;31m������ͣ����  ���Ժ�����\033[0m", YEA);
		return 0;
	}
	if (!file_exist(DIR_STOCK "stock"))
		initData(2, DIR_STOCK "stock");
	n = get_num_records(DIR_STOCK "stock", sizeof (struct BoardStock));
	if (n <= 0)
		return 0;
	filesize = sizeof (struct BoardStock) * n;
//���ع�����Ϣ 
	stockMem = loadData(DIR_STOCK "stock", buffer, filesize);
	if (stockMem == (void *) -1)
		return -1;
	while (!quit) {
		limit_cpu();
		sprintf(buf, "%s����", CENTER_NAME);
		money_show_stat(buf);
		showAt(4, 4, "\033[1;33m��Ʊ�з��գ��ǵ���Ԥ�ϡ�\n"
		       "    ������������Ʋ����Ը�!\033[0m\n\n", NA);
		move(t_lines - 1, 0);
		prints("\033[1;44m ѡ�� \033[1;46m [1]���� [2]���� [3]��Ϣ "
		       "[4]���� [5]���� [9]֤��� [Q]�뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
		case '2':
			money_show_stat("���״���");
			stockTrade(stockMem, n, ch - '1');
			break;
		case '3':
			showTradeInfo(stockMem, n);
			break;
		case '4':
			trendInfo(stockMem, n);
			break;
		case '5':
			myStockInfo();
			break;
		case '9':
			if (USERPERM(currentuser, PERM_SYSOP))
				stockAdmin(stockMem, n);
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
		updateStockEnv(stockMem, n, 0);
		if (!mcEnv->stockOpen)	//�ߵ������������ 
			break;
	}
	saveData(stockMem, filesize);
	return 0;
}

// -------------------------   �̵�   --------------------------  // 
static int
buy_card(char *cardname, char *filepath)
{
	int i;
	char uident[IDLEN + 1], note[3][STRLEN], tmpname[STRLEN];

	clear();
	ansimore2(filepath, 0, 0, 25);
	move(t_lines - 2, 4);
	sprintf(genbuf, "��ȷ��Ҫ��ؿ�%sô", cardname);
	if (askyn(genbuf, YEA, NA) == NA)
		return 0;
	if (myInfo->cash < 1000) {
		showAt(t_lines - 2, 60, 
			"���Ǯ��������", YEA);
		return 0;
	}
	myInfo->cash -= 1000;
	mcEnv->Treasury += 1000;
	clear();
	if (!getOkUser("Ҫ���⿨Ƭ�͸�˭? ", uident, 0, 0))
		return 0;
	move(2, 0);
	prints("�л�Ҫ�ڿ�Ƭ��˵��[����д3���]");
	bzero(note, sizeof (note));
	for (i = 0; i < 3; i++) {
		getdata(3 + i, 0, ": ", note[i], STRLEN - 1, DOECHO, NA);
		if (note[i][0] == '\0')
			break;
	}
	sprintf(tmpname, "bbstmpfs/tmp/card.%s.%d", currentuser->userid,
		uinfo.pid);
	copyfile(filepath, tmpname);
	if (i > 0) {
		int j;
		FILE *fp = fopen(tmpname, "a");
		fprintf(fp, "\n������ %s �ĸ���:\n", currentuser->userid);
		for (j = 0; j < i; j++)
			fprintf(fp, "%s", note[j]);
		fclose(fp);
	}
	if (mail_file
	    (tmpname, uident, "�����Ҵ��̵���������ĺؿ���ϲ����",
	     currentuser->userid) >= 0) {
		showAt(8, 0, "��ĺؿ��Ѿ�����ȥ��", YEA);
		mcLog("���ͺؿ��ɹ�", 0, uident);
	} else {
		showAt(8, 0, "�ؿ�����ʧ�ܣ��Է��ż�����", YEA);
		mcLog("���ͺؿ�ʧ��", 0, uident);
	}
	unlink(tmpname);
	return 0;
}

static int
shop_card_show()
{
	DIR *dp;
	struct dirent *dirp;
	char dirNameBuffer[10][32], buf[1024];
	char dirpath[256], filepath[256];
	int numDir, numFile, dirIndex, cardIndex;

	clear();
	snprintf(buf, sizeof(buf),
	"    ����ؿ����Ǳ������������ֺؿ���������ԭ��δ�ܱ������ߡ�\n"
	"    ��ؿ������߶�����Ʒ���ڱ���������飬���뱾վϵͳά����ϵ\n"
	"    ��վ����ʱ����������Ը����������\n\n"
	"                    \033[1;32m%s�ؿ���\033[0m\n", CENTER_NAME);
	showAt(6, 0, buf, YEA);
	sprintf(buf, "%s�ؿ���", CENTER_NAME);
	nomoney_show_stat(buf);
	showAt(4, 4, "��������������͵ĺؿ�: ", NA);
	if ((dp = opendir(DIR_SHOP)) == NULL)
		return -1;
	for (numDir = 0; (dirp = readdir(dp)) != NULL && numDir < 10;) {
		snprintf(dirpath, 255, "%s%s", DIR_SHOP, dirp->d_name);
		if (!file_isdir(dirpath) || dirp->d_name[0] == '.')
			continue;
		move(6 + numDir, 8);
		snprintf(buf, sizeof(buf), "\033[1;%dm%d. %s\033[m", numDir + 31,
				numDir, dirp->d_name);
		prints(buf);
		strncpy(dirNameBuffer[numDir], dirp->d_name, 31);
		dirNameBuffer[numDir][31] = '\0';
		numDir++;
	}
	closedir(dp);
	while (1) {
		getdata(16, 4, "��ѡ������:", buf, 3, DOECHO, YEA);
		if (buf[0] == '\0')
			return 0;
		dirIndex = atoi(buf);
		if (dirIndex >= 0 && dirIndex < numDir)
			break;
	}
	snprintf(dirpath, sizeof(dirpath), "%s%s", DIR_SHOP, dirNameBuffer[dirIndex]);
	if ((dp = opendir(dirpath)) == NULL)
		return -1;
	for (numFile = 0; (dirp = readdir(dp)) != NULL;) {
		snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, dirp->d_name);
		if (file_isfile(filepath) && dirp->d_name[0] != '.')
			numFile++;
	}
	closedir(dp);
	move(17, 4);
	snprintf(genbuf, sizeof(genbuf),
		"%s ���͵Ŀ�Ƭ���� %d ��. ��ѡ��ҪԤ���Ŀ���[ENTER����]: ",
		dirNameBuffer[dirIndex], numFile);
	while (1) {
		getdata(18, 4, genbuf, buf, 3, DOECHO, YEA);
		if (buf[0] == '\0')
			return 0;
		cardIndex = atoi(buf);
		if (cardIndex >= 1 && cardIndex <= numFile)
			break;
	}
	snprintf(buf, sizeof(buf), "%s%d", dirNameBuffer[dirIndex], cardIndex);
	snprintf(filepath, sizeof(buf), "%s/%d", dirpath, cardIndex);
	buy_card(buf, filepath);
	limit_cpu();
	return 0;
}

static int
shop_sellExp()
{
	int convertMoney, exp, ch, quit = 0;
	float convertRate;
	char buf[256];

	clear();
	sprintf(buf, "%s����", CENTER_NAME);
	money_show_stat(buf);
	convertRate = MIN_XY(bbsinfo.utmpshm->mc.ave_score / 1000.0 + 0.1, 10) * 1000;
	sprintf(genbuf, "    ������ͨ����������ֵ���%s������ı�����\n"
			"    [1]����ֵ��%.1fÿ��\t"
//			"[2]��ʶ��30000ÿ��\t"
//			"[3]����30000ÿ��\n    [4]��Ʒ��6000ÿ��\t\t"
			"[Q]�˳�\t\t��ѡ��",
		MONEY_NAME, convertRate);
	move(4, 0);
	prints("%s", genbuf);
	
	while(!quit) {
		ch = igetkey();
		switch (ch) {
		case '1':
			exp = countexp(currentuser);
			convertMoney = 
				MIN_XY(mcEnv->Treasury, (exp - myInfo->soldExp) * convertRate);
			if (convertMoney <= 0) {
				showAt(7, 4, "���޷���������ֵ��", YEA);
				quit = 1;
				break;
			}
			sprintf(genbuf, "�����ڵľ���ֵ,���������� %d %s, ��Ҫ������",
					convertMoney, MONEY_NAME);
			move(7, 4);
			if (askyn(genbuf, NA, NA) == NA)
				break;
			if (mcEnv->Treasury >= convertMoney){
				myInfo->Actived++;
				mcEnv->Treasury -= convertMoney;
				myInfo->soldExp += convertMoney / convertRate;
				myInfo->cash += convertMoney;
				move(8, 4);
				prints("���׳ɹ������������� %d %s��", convertMoney, MONEY_NAME);
				mcLog("��������", convertMoney, "");
			}else{
				move(8, 4);
				prints("����û����ô��Ǯ����");
			}
			break;
/*		case '2':
			convertMoney = userInputValue(7, 4, "��", "�㵨ʶ", 1, 100);
			if (convertMoney < 0)	break;
			if (convertMoney > myInfo->robExp) {
				showAt(8, 4, "��û����ô�ߵĵ�ʶ��", YEA);
				break;
			}
			if (mcEnv->Treasury < convertMoney * 30000) {
				showAt(8, 4, "����û����ô��Ǯ����", YEA);
				break;
			}
			myInfo->Actived++;
			myInfo->robExp -= convertMoney;
			myInfo->cash += convertMoney * 30000;
			mcEnv->Treasury -= convertMoney * 30000;
			showAt(9, 4, "���׳ɹ���", YEA);
			break;
		case '3':
                        convertMoney = userInputValue(7, 4, "��", "����", 1, 100);
			if (convertMoney < 0)   break;
			if (convertMoney > myInfo->begExp) {
				showAt(8, 4, "��û����ô�ߵ�����", YEA);
				break;
			}
			if (mcEnv->Treasury < convertMoney * 30000) {
				showAt(8, 4, "����û����ô��Ǯ����", YEA);
				break;
			}
			myInfo->Actived++;
			myInfo->begExp -= convertMoney;
			myInfo->cash += convertMoney * 30000;
			mcEnv->Treasury -= convertMoney * 30000;
			showAt(9, 4, "���׳ɹ���", YEA);
			break;
		case '4':
                        convertMoney = userInputValue(7, 4, "��", "����Ʒ", 1, 100);
			if (convertMoney < 0)   break;
			if (convertMoney > myInfo->luck + 100) {
				showAt(8, 4, "��û����ô�õ���Ʒ��", YEA);
				break;
			}
			if (mcEnv->Treasury < convertMoney * 6000) {
				showAt(8, 4, "����û����ô��Ǯ����", YEA);
				break;
			}
			myInfo->Actived++;
			myInfo->luck -= convertMoney;
			myInfo->cash += convertMoney * 6000;
			mcEnv->Treasury -= convertMoney * 6000;
			showAt(9, 4, "���׳ɹ���", YEA);
			break;
*/		case 'q':
		case 'Q':
		default:
			quit = 1;
			break;
		}
	}
	return 1;
}

static void
buydog()
{
	int guard_num;

	nomoney_show_stat("����԰");
	move(4, 4);
	prints
	    ("%s����԰���۴��ִ��ǹ�������Ҫ��������ʿ��ÿֻ10000%s��",
	     CENTER_NAME, MONEY_NAME);
#if 0
	if (myInfo->cash < 100000) {
		showAt(8, 4, "�㻹��ʡʡ��?û�˻���������ġ�", YEA);
		return;
	}
#endif
	guard_num = (myInfo->robExp / 100) + 1;
	guard_num = (guard_num > 8) ? 8 : guard_num;
	if (myInfo->guard >= guard_num) {
		showAt(8, 4, "���Ѿ����㹻�Ĵ��ǹ��˰���������ô��С�ɣ�", YEA);
		return;
	}

	move(6, 4);
	prints("������Ŀǰ����ݵ�λ������%dֻ���ǹ��͹��ˡ�",
	       guard_num - myInfo->guard);
	guard_num =
	    userInputValue(7, 4, "��", "ֻ���ǹ�", 1,
			   guard_num - myInfo->guard);
	if (guard_num == -1) {
		showAt(8, 4, "Ŷ�����������˰��������ǻ�ӭ�´�������", YEA);
		return;
	}

	if (myInfo->cash < guard_num * 10000) {
		showAt(8, 4, "������˼������Ǯ���������߲��͡�", YEA);
		return;
	}
	myInfo->cash -= guard_num * 10000;
	mcEnv->prize777 += after_tax(guard_num * 10000);
	myInfo->guard += guard_num;
	move(8, 4);
	showAt(9, 4, "����ǹ��ɹ�,�������һ��ʱ�䰲��̫ƽ�ˡ�", YEA);
	mcLog("����ǹ�", guard_num, "ֻ");
	return;
}

static int
money_shop()
{
	int ch, i, quit = 0, bonusExp = 0;
	char buf[256], uident[IDLEN + 1];

	while (!quit) {
		sprintf(buf, "%s�ٻ���˾", CENTER_NAME);
		nomoney_show_stat(buf);
		move(6, 4);
		prints("%s�̳���������𣬴�Ҿ��ˣ�", CENTER_NAME);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1]���� [2]�ؿ� [3]���� [4]�̳�����칫�� [Q]�뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			money_show_stat("�̳������̨");
			move(5, 0);
			prints("\033[1;42m��� ��Ʒ    ��  �� ��  ��  ��  ��             \033[m\n"
					 "  1  ���ǹ�  1��/ֻ ���ٱ���ש����\n"
					 "  2  �����  1��/�� �����������1��5��\n"
					 "  3  ƽ���� 10��/�� ƽ�⵨ʶ������\n"
					 "  4  ������ 20��/�� ������ӵ�ʶ������1��10��\n"
					 "  5  �ܲ���  5��/�� ����25�λ����\n"
					 "  6  ʱ��� 10��/�� ��ǰ�´λʱ��10����"
#if 0
			       "�����裨ʹ�ö����Լ������ã����ڼ�������\n"
			       "���˵���ʹ�ö��󣺿�ָ�������ã��ɹ���ɶ��ڼ���������\n"
			       "��Ͳ�� (ʹ�ö����Լ������ã����ڼӵ�ʶ)\n"
			       "͵�Ļ���ʹ�ö��󣺿�ָ�������ã��ɹ���ɼ��䵨ʶ��\n"
			       "���г���ʹ�ö����Լ������ã����ڼ���)\n"
			       "����Ƥ��ʹ�ö��󣺿�ָ�������ã��ɹ���ɶ��ڼ�������"
#endif
			    );

			move(t_lines - 2, 0);
#if 0
			prints
			    ("\033[1;44m ��Ҫ���� \033[1;46m [1]���ǹ� [2]����� [3]������ [4]���˵�\n"
			     "\033[1;44m          \033[1;46m [5]��Ͳ�� [6]͵�Ļ� [7]���г� [8]����Ƥ [Q]�뿪\033[m");
#else
			prints
			    ("\033[1;44m ��Ҫ���� \033[1;46m [1] ���ǹ� [2] ����� [3] ƽ����         \n"
			     "\033[1;44m          \033[1;46m [4] ������ [5] �ܲ��� [6] ʱ��� [Q] �뿪\033[m");
#endif
			ch = igetkey();
			switch (ch) {
			case '1':
				update_health();
				if (check_health(1, 13, 4, "���������ˣ�", YEA))
					break;
				myInfo->health--;
				myInfo->Actived++;
				buydog();
				break;
			case '2':
				bonusExp = userInputValue(13, 4, "Ҫ��","�������", 1, 20);
				if (bonusExp < 0)
					break;
				if (myInfo->cash < bonusExp * 10000) {
					showAt(15, 4, "��û����ô���ֽ�", YEA);
					break;
				}
				myInfo->cash -= bonusExp * 10000;
				mcEnv->Treasury += bonusExp * 10000;
				update_health();
				for (i = 0; i < bonusExp; i++)
					myInfo->health += random() % 5 + 1;
				myInfo->Actived++;
				showAt(15, 4, "\033[1;32m������������ˣ�\033[m", YEA);
				mcLog("��", bonusExp, "�������");
				break;
#if 0
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
#endif
			case '3':
				update_health();
				move(13, 4);
				if (askyn("ƽ����100,000Ԫһ����ȷʵҪ������", NA, NA)==NA)
					break;
				if (check_health(1, 14, 4, "������������ˣ�", YEA))
					break;
				myInfo->health--;
				myInfo->Actived += 10;
				if (myInfo->cash < 100000) {
					showAt(15, 4, "��û����ô���ֽ�", YEA);
					break;
				}
				myInfo->cash -= 100000;
				mcEnv->Treasury += 100000;
				myInfo->robExp = (myInfo->robExp + myInfo->begExp) / 2;
				myInfo->begExp = myInfo->robExp;
				showAt(16, 4, "ת���ɹ���", YEA);
				mcLog("��ƽ��������ʶ����Ϊ", myInfo->robExp, "");
				break;
			case '4':
				update_health();
				move(13, 4);
				if (askyn("������200,000Ԫһ����ȷʵҪ������", NA, NA)==NA)
					break;
				if (check_health(20, 14, 4, "������������ˣ�", YEA))
					break;
				myInfo->health -= 20;
				myInfo->Actived += 20;
				if (myInfo->cash < 200000) {
					showAt(15, 4, "��û�����ֽ�", YEA);
					break;
				}
				myInfo->cash -= 200000;
				mcEnv->Treasury += 200000;
				bonusExp = random()% 10 + 1;
				myInfo->robExp += bonusExp;
				myInfo->begExp += bonusExp;
				move(16, 4);
				prints("\033[1;31m��ϲ��������͵�ʶ��������%d�㣡��", bonusExp);
				mcLog("���������ӣ���ʶ��������", bonusExp, "��");
				pressanykey();
				break;
			case '5':
				update_health();
				move(13, 4);
				if (askyn("�ܲ���50,000Ԫһ����ȷʵҪ������", NA, NA) == NA)
					break;
				if (check_health(10, 14, 4, "������������ˣ�", YEA))
					break;
				myInfo->health -= 10;
				if (myInfo->cash < 50000) {
					showAt(14, 4, "��û�����ֽ�", YEA);
					break;
				}
				myInfo->cash -= 50000;
				mcEnv->Treasury += 50000;
				myInfo->Actived += 25;
				myInfo->luck -= 5;
				showAt(15, 4, "\033[1;32m��Ļ����������25�㣡", NA);
				showAt(16, 4, "�����Ʒ�½�5�㣡��\033[m", YEA);
				mcLog("ʹ���ܲ���", 0, "");
				break;
			case '6':
				update_health();
				move(13, 4);
				if (askyn("ʱ���100,000Ԫһ����ȷʵҪ������", NA, NA) == NA)
					break;
				if (check_health(20, 14, 4, "������������ˣ�", YEA))
					break;
				myInfo->health -= 20;
				if (myInfo->cash < 100000) {
					showAt(14, 4, "��û�����ֽ�", YEA);
					break;
				}
				myInfo->cash -= 100000;
				mcEnv->Treasury += 100000;
				myInfo->lastActiveTime -= 600;
				myInfo->luck -= 5;
				myInfo->Actived += 10;
				showAt(15, 4, "\033[1;33m���´λʱ����ǰ��10���ӣ�", NA);
				showAt(16, 4, "�����Ʒ�½�5�㣡��\033[m", YEA);
				mcLog("ʹ��ʱ�ջ�", 0, "");
				break;
			case 'q':
			case 'Q':
				quit = 1;
				break;
			}
			quit = 0;
			break;
		case '2':
			shop_card_show();
			break;
		case '3':
			money_show_stat("���̹�̨");
			update_health();
			if (check_health(1, 12, 4, "���������ˣ�", YEA))
				break;
			myInfo->health--;
			shop_sellExp();
			break;
		case '4':
			money_show_stat("�̳�����칫��");
			whoTakeCharge(7, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			while (!quit) {
				nomoney_show_stat("�̳�ҵ��");
				update_health();
				if (check_health
				    (1, 12, 4, "Ъ��Ъ�ᣬ���������ˣ�", YEA))
					break;
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m ѡ�� \033[1;46m [1]�ɹ� [2]���� [3]�鿴�̵��ʲ� [Q]�뿪\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					nomoney_show_stat("�ɹ���");
					showAt(12, 4,
					       "\033[1;32m�滮�С�\033[m", YEA);
					break;
				case '2':
					money_show_stat("�г���");
					showAt(12, 4,
					       "\033[1;32m�滮�С�\033[m", YEA);
					break;
				case '3':
					money_show_stat("С���");
					showAt(12, 4,
					       "\033[1;32m������ڵ�Ǯ�����Ժ�\033[m",
					       YEA);
					break;
				case 'q':
					quit = 1;
					break;
				}
			}
			quit = 0;
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
		limit_cpu();
	}
	return 0;
}

//  ---------------------------   ����     --------------------------  // 

static int
cop_accuse()
{
	char uident[IDLEN + 1], buf[256];
	int award, oldaward;
	time_t currtime;
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;
	FILE *fp;

	move(4, 4);
	prints("������������ٻ�͵�ԣ���������κ����������򾯷����档");
	move(5, 4);
	prints("\033[1;32m�������������������þ��棡(�ٱ���10000Ԫ)\033[0m");
	if (myInfo->cash < 10000) {
		showAt(7, 4, "��û�д����ֽ�", YEA);
		return 0;
	}
	if (!getOkUser("�ٱ�˭��", uident, 7, 4))
		return 0;
	if (seek_in_file(POLICE, uident)) {
		showAt(8, 4, "�󵨣������ݾ�����Ա�𣿣�", YEA);
		return 0;
	}
	award = userInputValue(8, 4, "�����", MONEY_NAME "׽����",
			10000, myInfo->cash);
	if (award < 0) {
		showAt(10, 4, "�㲻��ٱ����ˡ�", YEA);
		return 0;
	}
	move(10, 4);
	if (askyn("\033[1;33m���򾯷��ṩ��������Ϣ��ʵ��\033[0m", NA, NA) ==
	    NA)
		return 0;
	myInfo->cash -= award;
	mcEnv->Treasury += award;
	sethomefile(buf, uident, "mc.save");
	if (!file_exist(buf))
		initData(1, buf);
	mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
	if (mcuInfo == (void *) -1)
		return 0;
	currtime = time(NULL);
	if (mcuInfo->robExp == 0 || mcuInfo->begExp == 0
		|| mcuInfo->lastActiveTime + 21600 < currtime) {
		showAt(8, 4, "���������ܰ��ְ����㲻Ҫ�̰�����Ŷ��", YEA);
		goto UNMAP;
	}
	if (mcuInfo->freeTime > 0) {
		showAt(11, 4, "������Ѿ����������ˡ�", YEA);
		goto UNMAP;
	}
	mcLog("�ٱ�", award, uident);
	if (seek_in_file(CRIMINAL, uident)) {
		fp = fopen(CRIMINAL, "r");
		while(fscanf(fp, "%s %d\n", buf, &oldaward)) {
			if(strcmp(buf, uident))
				continue;
			break;
		}
		fclose(fp);
		award += oldaward;
		del_from_file(CRIMINAL, uident);
	}
	sprintf(buf, "%s %d", uident, award-10000);
	addtofile(CRIMINAL, buf);
	showAt(11, 4, "�����ǳ���л���ṩ�����������ǽ����������ư���", YEA);
	return 1;

      UNMAP:saveData(mcuInfo, 1);
	return 0;
}

static int
cop_arrange(int type)
{
	int found;
	char uident[IDLEN + 1], buf[STRLEN], title[STRLEN];
	char *actionDesc[] = { "-", "��ļ", "��ְ", NULL };
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;

	if (!getOkUser("������ID: ", uident, 12, 4))
		return 0;
	found = seek_in_file(POLICE, uident);
	move(13, 4);
	if (type == 1 && found) {
		showAt(13, 4, "��ID�Ѿ��Ǿ�Ա�ˡ�", YEA);
		return 0;
	} else if (type == 2 && !found) {
		showAt(13, 4, "��ID���Ǿ���Ա��", YEA);
		return 0;
	}
	if (type == 1 && 
		(seek_in_file(ROBUNION, currentuser->userid)
			|| seek_in_file(BEGGAR, currentuser->userid))) {
		showAt(13, 4, "��������ϵ���������˹���Ϊ��Ա��", YEA);
		return 0;
	}

	sprintf(buf, "%sȷ��%s��",
		type == 2 ? "����;�Ա����ʧһ�뵨ʶ������" 
		          : "����Ҫ����100000Ԫ��ļ��Ա��",
		actionDesc[type]);
	if (askyn(buf, NA, NA) == NA)
		return 0;
	if (type == 1) {
		if (myInfo->cash < 100000) {
			showAt(14, 4, "��û���㹻���ֽ���ļ��Ա��", YEA);
			return 0;
		}
		addtofile(POLICE, uident);
		myInfo->cash -= 100000;
		mcEnv->Treasury += 100000;
	} else {
		sethomefile(buf, uident, "mc.save");
		if (!file_exist(buf))
			initData(1, buf);
		mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			return 0;
		mcuInfo->robExp /= 2;
		mcuInfo->begExp /= 2;
		mcuInfo->luck = -100;
		del_from_file(POLICE, uident);
		saveData(mcuInfo, sizeof (struct mcUserInfo));
	}
	mcLog(actionDesc[type], 0, uident);
	sprintf(title, "������%s��Ա %s", actionDesc[type], uident);
	sprintf(buf, "    ������%s %s��Ա %s\n", currentuser->userid,
		actionDesc[type], uident);
	deliverreport(title, buf);
	system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
	showAt(14, 4, "�����ɹ���", YEA);
	return 1;
}

static void
cop_Arrest(void)
{
	char uident[IDLEN + 1], buf[256], title[STRLEN];
	time_t currtime;
	int award;
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;
	FILE *fp;

	if (!seek_in_file(POLICE, currentuser->userid)) {
		showAt(12, 4, "����ִ�����������˵���رܣ��������ˡ�", YEA);
		return;
	}
	if (seek_in_file(ROBUNION, currentuser->userid)
		|| seek_in_file(BEGGAR, currentuser->userid)) {
		showAt(12, 4, "���޼���������˰��㣿", YEA);
		return;
	}

	if (myInfo->robExp < 50 || myInfo->begExp < 50) {
		showAt(12, 4, "�¾���ɣ���ȥ��������������Ȼ���������ˡ�",
		       YEA);
		return;
	}
	if (check_health(60, 12, 4, "��û�г����������ִ������", YEA))
		return;

	currtime = time(NULL);
	if (currtime < 3600 + myInfo->lastActiveTime) {
		showAt(12, 4, "��Ҫ�ż����ϼ���û���ж�ָʾ��", YEA);
		return;
	}

	move(4, 4);
	prints("�ոսӵ��ϼ�ָʾ������Ҫ����һ��ץ���ж���");

	if (!getOkUser("\n��ѡ�����Ŀ�꣺", uident, 6, 4)) {
		move(8, 4);
		prints("���޴���");
		pressanykey();
		return;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(8, 4, "ţħ���������š��������񾭲�������", YEA);
		return;
	}
	if (!seek_in_file(CRIMINAL, uident)) {
		showAt(8, 4, "���˲���ͨ���������档��", YEA);
		return;
	}
	myInfo->Actived += 5;
	move(10, 4);
	prints("������ڴ�������ıؾ�֮·���ȴ�Ŀ��ĳ��֡�");

	sethomefile(buf, uident, "mc.save");
	if (!file_exist(buf))
		initData(1, buf);
	mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
	if (mcuInfo == (void *) -1)
		return;
	myInfo->lastActiveTime = currtime;
	sleep(1);
	if (currtime > 3600 + mcuInfo->lastActiveTime ||
			currtime < mcuInfo->lastActiveTime) {
		myInfo->health -= 10;
		del_from_file(CRIMINAL, uident);
		showAt(12, 4, "���������һ�죬Ŀ�껹��û�г��֣�ֻ�÷����ˡ�",
		       YEA);
		mcLog("����û�����ﷸ", 0, uident);
		return;
	}

	if (!(random() % 3)) {
		myInfo->health -= 10;
		showAt(12, 4, "���ۿ���Զ����Ӱһ�Σ�Ŀ��õ������ܵ��ˡ�",
		       YEA);
		mcLog("����û׷���ﷸ", 0, uident);
		return;
	}

	if ((myInfo->robExp + myInfo->begExp >=
	     (mcuInfo->robExp + mcuInfo->begExp)* 5 + 50 || 
	     myInfo->luck - mcuInfo->luck >= 100 || (random() % 3))
	    && !(mcuInfo->robExp + mcuInfo->begExp >= 
		(myInfo->robExp + myInfo->begExp) * 5 + 50)) {
		myInfo->begExp += 30;
		myInfo->robExp += 30;
		myInfo->health -= 20;
		fp = fopen(CRIMINAL, "r");
		while(fscanf(fp, "%s %d\n", buf, &award)) {
			if(strcmp(buf, uident))
				continue;
			break;
		}
		fclose(fp);
		myInfo->cash += mcuInfo->cash + award;
		mcEnv->Treasury -= award;
		mcLog("����ץס�ﷸ��û���ֽ�", mcuInfo->cash, uident);
		mcuInfo->cash = 0;
		mcuInfo->begExp = MAX_XY(0, mcuInfo->begExp - 10);
		mcuInfo->robExp = MAX_XY(0, mcuInfo->robExp - 10);
		mcuInfo->luck = MAX_XY(-100, mcuInfo->luck - 10);
		mcuInfo->freeTime = currtime + 7200;

		myInfo->luck = MIN_XY(100, myInfo->luck + 10);
		del_from_file(CRIMINAL, uident);
		prints("\n    Զ���������˹������㶨��һ����������Ҫץ��%s��"
		       "\n    ���������γ���ǹ��𵽣����������㱻���ˣ���"
		       "\n    %sĿ�ɿڴ���ֻ�����־��ܡ�"
		       "\n    ��ĵ�ʶ�����ˣ�"
		       "\n    ����������ˣ�"
		       "\n    �����Ʒ�����ˣ�"
		       "\n    û���ﷸ�����ֽ���Ϊ������", uident, uident);
		sprintf(title, "�����𡿾���ץ��ͨ����%s", uident);
		sprintf(buf,
			"    ��Ա%s�����غ򣬹��������ڰ�ͨ����%s���ش˹��棬��ʾ���á�\n",
			currentuser->userid, uident);
		deliverreport(title, buf);
		sprintf(buf,
			"�㱻��Ա%s���ץ�񣬱��ؽ���2Сʱ��û�������ֽ𣬵�ʶ���½���",
			currentuser->userid);
		system_mail_buf(buf, strlen(buf), uident, "�㱻����ץ��",
				currentuser->userid);
		saveData(mcuInfo, sizeof (struct mcUserInfo));
		pressreturn();
	} else {
		myInfo->begExp -= 5;
		myInfo->robExp -= 5;
		myInfo->health = 0;
		myInfo->freeTime = currtime + 3600;
		prints("\n    Զ���������˹������㶨��һ����������Ҫץ��%s��"
		       "\n    ���������γ���ǹ��𵽣����������㱻���ˣ���"
		       "\n    ������ʱ�����ϱ������˴�����һ���ƹ���"
		       "\n    ��ĵ�ʶ�����ˣ�"
		       "\n    ����������ˣ�" "\n    ����˹�ȥ��", uident);
		sprintf(title, "�����𡿾�Ա%s����סԺ", currentuser->userid);
		sprintf(buf,
			"��Ա%s��ִ�������ʱ�򣬱��ﷸ����סԺ��Ŀǰ�����"
			"�������ü��ɳ�Ժ��\n", currentuser->userid);
		deliverreport(title, buf);
		sprintf(buf,
			"����%s���ץ�㣬�������ո��˵�����ɵ����Լ���Ȼ"
			"����", currentuser->userid);
		system_mail_buf(buf, strlen(buf), uident, "�����ѿ�",
				currentuser->userid);
		mcLog("���챻�ﷸ����סԺ", 0, uident);
		saveData(myInfo, sizeof (struct mcUserInfo));
		pressreturn();
		Q_Goodbye();
	}
	return;
}

static int
money_cop()
{
	int ch, quit = 0, num;
	char uident[IDLEN + 1], title[78], buf[256];

	while (!quit) {
		sprintf(buf, "%s����", CENTER_NAME);
		nomoney_show_stat(buf);
		move(8, 16);
		prints("������ά���ΰ���");
		move(10, 4);
		prints("���ڷ���������ӣ��̾���Ҳ��ʼ���Լ�ҹ�ļӰࡣ");
		move(t_lines - 1, 0);
		prints("\033[1;44m ѡ�� \033[1;46m [1]���� [2]ͨ����"
		       " [3]�̾��� [4] �������� [5]�𳤰칫�� [Q]�뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			nomoney_show_stat("����Ӵ���");
			cop_accuse();
			break;
		case '2':
			clear();
			move(1, 0);
			prints("%s����ǰͨ���ķ���������:\n"
				"\033[1;32m�ﷸ����     \033[33m����\033[m", CENTER_NAME);
			listfilecontent(CRIMINAL);
			FreeNameList();
			pressanykey();
			break;
		case '3':
			nomoney_show_stat("�̾���");
//showAt(6, 4, "�ӵ��ϼ�����,�ж���ȡ��...", YEA); 
			cop_Arrest();
			break;
		case '4':
			money_show_stat("���������");
			if (myInfo->cash < 10000) {
				showAt(6, 4, "������ֽ�̫���ˣ�cmft", YEA);
				break;
			}
			num =userInputValue(6, 4, "��������", "��" MONEY_NAME, 
				1, myInfo->cash / 10000);
			if (num <= 0)
				break;
			myInfo->cash -= num * 10000;
			mcEnv->Treasury += num * 10000;
			myInfo->luck = MIN_XY(100, myInfo->luck + num);
			showAt(8, 4, "�����յ������ľ�лл��", YEA);
			mcLog("��������", num, "��Ԫ");
			if (num == 10 &&
				(!seek_in_file(POLICE, currentuser->userid))) {
				move(9, 4);
				if(askyn("��Ҫ���뾯�������", NA, NA) == NA)
					break;
				if(seek_in_file(BEGGAR, currentuser->userid)) {
					showAt(10, 4, "���Ѿ���ؤ���Ա�ˣ�\n", NA);
					if(askyn("    ��Ҫ�˳�ؤ����", YEA, NA) == NA)
						break;
					del_from_file(BEGGAR, currentuser->userid);
					mcLog("�˳�ؤ��", 0, "");
					showAt(12, 4, "���Ѿ��˳�ؤ���ˡ�", YEA);
				}
				if(seek_in_file(ROBUNION, currentuser->userid)) {
					showAt(10, 4, "���Ѿ��Ǻڰ��Ա�ˣ�    \n", NA);
					if(askyn("    ��Ҫ�˳��ڰ���", YEA, NA) == NA)
						break;
					del_from_file(ROBUNION, currentuser->userid);
					mcLog("�˳��ڰ�", 0, "");
					showAt(12, 4, "���Ѿ��˳��ڰ��ˡ�", YEA);
				}
				addtofile(POLICE, currentuser->userid);
				showAt(14, 4, "���Ѿ������˾�����飡", YEA);
				sprintf(title, "������%s��Ϊ����", currentuser->userid);
				sprintf(buf, "    ��ͨ������С�ģ������ܻ���ʱ�����㣡\n");
				deliverreport(title, buf);
				mcLog("��Ϊ����", 0, "");
				break;
			}				
			if (num < 100)
				break;
			move(16, 0);
			if (askyn("    ���뱣���ﷸ��", NA, NA) == NA)
				break;
			if (!getOkUser("������ID: ", uident, 17, 4))
	                	break;
		        if (!seek_in_file(CRIMINAL, uident)) {
				prints("    %s ������ͨ�����ϡ�", uident);
				break;
			}
			del_from_file(CRIMINAL, uident);
			showAt(18, 4, "���Ѿ��ɹ��ñ���������", YEA);
			sprintf(title, "������%s������", uident);
			if (!strcmp(currentuser->userid, uident)) {
				sprintf(buf, "    %s ���� %d �� %s �������Լ���\n",
					currentuser->userid, num, MONEY_NAME);
				deliverreport(title, buf);
			} else {
				sprintf(buf, "    %s ���� %d �� %s ������ %s��\n",
					currentuser->userid, num, MONEY_NAME, uident);
				deliverreport(title, buf);
				sprintf(title, "�㱻 %s ����", currentuser->userid);
				sprintf(buf, "    %s ���� %d �� %s ���㱣�ͳ�����",
					currentuser->userid, num, MONEY_NAME);
                		system_mail_buf(buf, strlen(buf), uident, title,
						currentuser->userid);
			}
			mcLog("�����ﷸ������", num, uident);
			break;
		case '5':
			nomoney_show_stat("�𳤰칫��");
			whoTakeCharge(8, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			showAt(6, 0, "    ��ѡ���������:\n"
			       "        1. ��ļ��Ա         2. ��ְ��Ա\n"
			       "        3. ��Ա����         Q. �˳�",
			       NA);
			ch = igetkey();
			switch (ch) {
			case '1':
			case '2':
				cop_arrange(ch - '0');
				break;
			case '3':
				clear();
				move(1, 0);
				prints("Ŀǰ%s����Ա������", CENTER_NAME);
				listfilecontent(POLICE);
				FreeNameList();
				pressanykey();
				break;
			}
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
		limit_cpu();
	}
	return 0;
}

//  --------------------------    ����    ------------------------  // 
static void
special_MoneyGive()
{
	int money;
	char uident[IDLEN + 1], reason[256], buf[256], title[STRLEN];
	struct mcUserInfo *mcuInfo;
	void *buffer = NULL;

	money_show_stat("�ر𲦿�С���");
	if (!getOkUser("�㲦���˭��", uident, 16, 4)) {
		move(17, 4);
		prints("���޴���");
		pressanykey();
		return;
	}
	sethomefile(buf, uident, "mc.save");
	if (!file_exist(buf))
		initData(1, buf);
	money = userInputValue(17, 4, "��", "��", 10, 1000);
	if (money < 0)
		return;
	getdata(19, 4, "���ɣ�", reason, 40, DOECHO, YEA);

        if (reason[0] == '\0' || reason[0] == ' '){
                showAt(20, 4, "û�����ɲ���˽�Բ��", YEA);
                return;
        }
	move(20, 4);
	if (askyn("��ȷ��Ҫ������", YEA, NA) == YEA) {
		mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			return;
		mcuInfo->credit += 10000 * money;
		mcEnv->Treasury -= 10000 * money;
		myInfo->Actived++;
		sprintf(title, "�ܹ�%s�ر𲦿�%d���%s", currentuser->userid,
			money, uident);
		sprintf(buf, "    �ܹ�%s�ر𲦿�%d���%s \n���ɣ�%s\n",
			currentuser->userid, money, uident, reason);
		system_mail_buf(buf, strlen(buf), uident, title,
				currentuser->userid);
		deliverreport(title, buf);

		saveData(mcuInfo, sizeof (struct mcUserInfo));
		showAt(21, 4, "����ɹ�", YEA);
		mcLog("����", money * 10000, uident);
	}
	return;
}

static void
promotion(int type, char *prompt)
{
	int pos;
	char boss[IDLEN], buf[STRLEN];
	char *feaStr[] = { "admin", "bank", "lottery", "gambling",
		"gang", "beggar", "stock", "shop", "police"
	};

	getdata(16, 4,
		"ְλ: [0.�ܹ� 1.���� 2.��Ʊ 3.�ĳ� 4.�ڰ� 5.ؤ�� 6.���� 7.�̳� 8.���� ]",
		buf, 2, DOECHO, YEA);
	pos = atoi(buf);
	if (pos > 8 || pos < 0)
		return;
	whoTakeCharge(pos, boss);
	move(17, 4);
	if (boss[0] != '\0' && type == 0) {
		prints("%s�Ѿ������ְλ��", boss);
		pressanykey();
		return;
	}
	if (boss[0] == '\0' && type == 1) {
		showAt(17, 4, "Ŀǰ�����˸����ְλ��", YEA);
		return;
	}
	if (type == 0 && !getOkUser(prompt, boss, 18, 4))
		return;
	positionChange(pos, boss, feaStr[pos], type);
}

static int
money_admin()
{
	int ch, quit = 0;
	char admin[IDLEN + 1], uident[IDLEN + 1], title[STRLEN], buf[256];
	void *buffer=NULL;
	struct mcUserInfo *mcuInfo;

	if (readstrvalue(MC_BOSS_FILE, "admin", admin, IDLEN + 1) != 0)
		admin[0] = '\0';
	if (strcmp(admin, currentuser->userid)
	    && !(currentuser->userlevel & PERM_SYSOP))
		return 0;
	while (!quit) {
		clear();
		move(1, 0);
		prints
		("���Ĵ��ţ�\033[1;33m%s\033[m      ��ʶ \033[1;33m%d\033[m  �� \033[1;33m%d\033[m  ��Ʒ \033[1;33m%d\033[m  ���� \033[1;33m%d\033[m \n",
		 currentuser->userid, myInfo->robExp, myInfo->begExp, myInfo->luck,myInfo->health);
		prints
		("\033[1;32m��ӭ����%s�������磬��ǰλ����\033[0m \033[1;33m�����������\033[0m", CENTER_NAME);
		move(3, 0);
		prints
		("\033[1m--------------------------------------------------------------------------------\033[m");
					
		sprintf(buf, "    ���︺��%s������������¹���\n"
			     "    �����ܹ���: \033[1;32m%s\033[m\n"
			     "    ���ⴢ����: \033[1;33m%15Ld\033[m %s"
			     "    �û��ܲƲ�: \033[1;32m%15Ld\033[m %s\n",
		             CENTER_NAME, admin, mcEnv->Treasury, MONEY_NAME, 
			     mcEnv->AllUserMoney, MONEY_NAME);
		showAt(5, 0, buf, NA);
		mcLog("�ܹܲ鿴", 0, buf);
		sprintf(buf, "    �����û�������: \033[1;32m%9d\033[m ��    "
			     "    �û��˾��Ƹ�: \033[1;33m%13Ld\033[m %s",
			     mcEnv->userNum, mcEnv->userNum==0?
			     0:(mcEnv->AllUserMoney/mcEnv->userNum), MONEY_NAME);
		showAt(8, 0, buf, NA);
		mcLog("�ܹܲ鿴", 0, buf);
		showAt(10, 0,
		       "        1. ����ְλ                    2. ��ȥְλ\n"
		       "        3. �г�ְλ����                4. �ر�/������������\n"
		       "        5. �ر𲦿�                    6. ���л���\n"
		       "        7. �趨��������                8. ���ٻ���\n"
		       "        9. �����û��ܲƲ�              0. ��ѯ�û�\n"
		       "        Q. �˳�", NA);
		ch = igetkey();
		switch (ch) {
		case '1':
			promotion(0, "����˭��");
			break;
		case '2':
			promotion(1, "��ȥ˭��");
			break;
		case '3':
			clear();
			move(1, 0);
			prints("Ŀǰ%s���������ְλ�����", CENTER_NAME);
			listfilecontent(MC_BOSS_FILE);
			FreeNameList();
			pressanykey();
			break;
		case '4':
			move(16, 4);
			if (mcEnv->closed)
				sprintf(buf, "%s", "ȷ��Ҫ����ô");
			else
				sprintf(buf, "%s", "ȷ��Ҫ�ر�ô");
			if (askyn(buf, NA, NA) == NA)
				break;
			mcEnv->closed = !mcEnv->closed;
			if (!mcEnv->closed)
				mcEnv->openTime = time(NULL);
			showAt(17, 4, "�����ɹ�", YEA);
			mcLog(mcEnv->closed?"�رմ�������":"������������", 0, "");
			break;
		case '5':
			special_MoneyGive();
			break;
		case '6':
			move(17, 4);
			if (askyn("ȷʵҪ����һǧ�������", NA, NA) == NA)
				break;
			mcEnv->Treasury += 10000000;
			move(18, 4);
			prints("���Ѿ�ӡ��һǧ��%s��ע���˹��⡣", MONEY_NAME);
			sprintf(title, "�����⡿%s�ܹ�%s�����ע����10000000%s",
					CENTER_NAME, currentuser->userid, MONEY_NAME);
			sprintf(buf, "    Ϊ���⾭��Σ�����ܹ�%s��׼ӡ��10000000%s\n",
					currentuser->userid, MONEY_NAME);
			deliverreport(title, buf);
			mcLog("����", 10000000, "����");
			break;
		case '7':
			banID();
			break;
		case '8':
			move(17, 4);
			if (askyn("ȷʵҪ����һǧ�������", NA, NA) == NA)
				break;
			mcEnv->Treasury -= 10000000;
			move(18, 4);
			prints("���Ѿ�������һǧ�����%s��", MONEY_NAME);
			sprintf(title, "�����⡿%s�ܹ�%s������10000000����%s",
					CENTER_NAME, currentuser->userid, MONEY_NAME);
			sprintf(buf, "    �ܹ�%s������10000000���Ƶ�%s\n",
					currentuser->userid, MONEY_NAME);
			deliverreport(title, buf);
			mcLog("����", 10000000, "���ƻ���");
			break;
		case '9':
			showAt(17, 4, "�����С���", NA);
			refresh();
			all_user_money();
			showAt(18, 4, "������ɣ�", YEA);
			break;
		case '0':
			clear();
			if (!getOkUser("��ѯ˭�������", uident, 0, 0))
				break;
			myInfo->Actived++;
			sethomefile(buf, uident, "mc.save");
			if (!file_exist(buf))
				initData(1, buf);
			mcuInfo = loadData(buf, buffer,	sizeof (struct mcUserInfo));
			if (mcuInfo == (void *) -1)
				break;
			prints("�汾��%12d\tmutex��%11d\ntemp[0]��%9d\ttemp[1]��%9d\n",
				mcuInfo->version, mcuInfo->mutex, mcuInfo->temp[0], mcuInfo->temp[1]);
			prints("�ֽ�%12d\t��%12d\n���%12d\t��Ϣ��%12d\t�����䣺%10d\n",
				mcuInfo->cash, mcuInfo->credit, mcuInfo->loan, mcuInfo->interest, mcuInfo->safemoney);
			prints("��ʶ��%12d\tpoliceExp��%7d\t����%12d\n��������ֵ��%6d\t�������%8d\n",
				mcuInfo->robExp, mcuInfo->policeExp, mcuInfo->begExp, mcuInfo->soldExp, mcuInfo->Actived);
			prints("������%12d\t��Ʒ��%12d\n���ǹ���%10d\t�������ƣ�%8d\n",
				mcuInfo->health, mcuInfo->luck, mcuInfo->guard, mcuInfo->antiban);
			prints("��ǰʱ�䣺      %s", ctime(&mcuInfo->aliveTime));
			prints("����/��Ժʱ�䣺 %s", ctime(&mcuInfo->freeTime));
			prints("����ʱ�䣺      %s", ctime(&mcuInfo->loanTime));
			prints("����ʱ�䣺      %s", ctime(&mcuInfo->backTime));
			prints("�ϴδ��ʱ�䣺  %s", ctime(&mcuInfo->depositTime));
			prints("�ϴ��칤��ʱ�䣺%s", ctime(&mcuInfo->lastSalary));
			prints("�ϴλʱ�䣺  %s", ctime(&mcuInfo->lastActiveTime));
			prints("�ϴβ�Ǯʱ�䣺  %s", ctime(&mcuInfo->secureTime));
			prints("���յ���ʱ�䣺  %s", ctime(&mcuInfo->insuranceTime));
			mcLog("��ѯ",1,uident);
			pressanykey();
			break;
		case 'Q':
		case 'q':
			quit = 1;
			break;
		}
	}
	return 0;
}

static void
banID(void)
{
	int found;
	char uident[IDLEN + 1], buf[256], title[STRLEN];
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;
	
        if (!getOkUser("������ID: ", uident, 16, 4))
		return;
	found = seek_in_file(BADID, uident);
	if (found) {
		sprintf(buf, "%s���ڻ���������Ҫ�����ͷ���", uident);
		move(17, 4);
		if (askyn(buf, NA, NA) == NA)
			return;
		del_from_file(BADID, uident);
		sprintf(title, "���ܹܡ�����%s�����������", uident);
		sprintf(buf, "    ����%s���иĻ�֮�ģ������ܹ�%s���������½���������硣\n", 
				uident, currentuser->userid);
		deliverreport(title, buf);
		system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
		showAt(18, 4, "�����ɹ���", YEA);
		mcLog("�����ID�����������", 0, uident);
		return;
	}else{
		sprintf(buf, "��ȷ����%s���뻵����������������ָ���������㣡", uident);
		move(17, 4);
		if (askyn(buf, NA, NA) == NA)
			return;
                sethomefile(buf, uident, "mc.save");
		if (!file_exist(buf))
			initData(1, buf);
		mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			return;
		if (mcuInfo->antiban > 0) {
			move(18, 4);
			prints("%s������������%d�飬���β�����ȥ1�顣", uident, mcuInfo->antiban);
			mcuInfo->antiban--;
			mcLog("��ֹ��ID����������硣������������", mcuInfo->antiban, uident);
			saveData(mcuInfo, sizeof (struct mcUserInfo));
			sprintf(title, "���ܹܡ�%sʹ����һ����������", uident);
			sprintf(buf,
				"    ����%s���Ҵ��������������"
				"���ܹ�%sȡ���������������ʸ�\n"
				"    ������ӵ���������ƣ���������̷�����������������һ��\n",
				uident, currentuser->userid);
			deliverreport(title, buf);
			system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
			showAt(19, 4, "�������������ơ�", YEA);
			return;
		}
		mcuZero(uident);
		addtofile(BADID, uident);
		sprintf(title, "���ܹܡ������ܹ�%sȡ��%s�������������ʸ�",
				currentuser->userid, uident);
		sprintf(buf, "    ����%s���Ҵ�������������򣬱�ȡ���������������ʸ�\n"
			     "    ������ָ���ѱ����㡣��������½���������磬�����ܹ���ϵ��\n",
			      uident);
		deliverreport(title, buf);
		system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
		showAt(18, 4, "�����ɹ���", YEA);
		mcLog("��ֹ��ID����������磬����ָ����������", 0, uident);
		return;
	}
}

void mcuZero(char *uident)
{
	char filepath[256];
	struct mcUserInfo *mcuInfo;
	struct MC_Env *mce;
	void *buffer_env = NULL, *buffer_me = NULL;

	sprintf(filepath, "%s", DIR_MC "mc.env");
	mce = loadData(filepath, buffer_env, sizeof (struct MC_Env));
	if (mce == (void *) -1)
		return;
	
	sethomefile(filepath, uident, "mc.save");
	if (!file_exist(filepath))
		return;
	mcuInfo = loadData(filepath, buffer_me, sizeof (struct mcUserInfo));
	if (mcuInfo == (void *) -1)
		return;
	
	mcuInfo->robExp = 0;
	mcuInfo->begExp = 0;
	mcuInfo->luck = MIN_XY(mcuInfo->luck, 0);
	mce->Treasury += mcuInfo->cash +
			 mcuInfo->credit +
			 mcuInfo->interest +
			 mcuInfo->safemoney;
	mcuInfo->cash = 0;
	mcuInfo->credit = 0;
	mcuInfo->interest = 0;
	mcuInfo->safemoney = 0;
	
	if (seek_in_file(ROBUNION, uident))
		del_from_file(ROBUNION, uident);
	if (seek_in_file(BEGGAR, uident))
		del_from_file(BEGGAR, uident);
	if (seek_in_file(POLICE, uident))
		del_from_file(POLICE, uident);
	if (seek_in_file(BADID, uident))
		del_from_file(BADID, uident);
	
	saveData(mcuInfo, sizeof (struct mcUserInfo));
	saveData(mce, sizeof (struct MC_Env));
	
	return;
}

static void antiban()
{
	int num;
	money_show_stat("Ļ����ְ칫��");
	move(6, 4);
	if(askyn("��������1000��һ�飬��Ҫ����", NA, NA) == NA)
		return;
	if(myInfo->cash < 10000000) {
		showAt(7, 4, "��û�����ֽ�", YEA);
		return;
	}
	num = userInputValue(7, 4, "��","����������",1, myInfo->cash / 10000000);
	if (num < 1) {
		showAt(9, 4, "�㲻���ˡ�", YEA);
		return;
	}
	myInfo->cash -= num * 10000000;
	mcEnv->Treasury += num * 10000000;
	myInfo->antiban += num;
	mcLog("����������", num, "��");
	showAt(10, 4, "���׳ɹ���", YEA);
	return;
}
static void
all_user_money()
{
	FILE *fp, *tmp;
        char buf[256];
	void *buffer = NULL;
	float rate = mcEnv->depositRate / 10000.0;
	int cal_Interest; 
	struct mcUserInfo *mcuInfo;

	mcEnv->AllUserMoney = 0;
	mcEnv->userNum = 0;
	fp = fopen(DIR_MC "mc_user", "r");
	flock(fileno(fp), LOCK_EX);
	tmp = fopen(DIR_MC "mc_user.tmp", "w");
	flock(fileno(tmp), LOCK_EX);
	while (fscanf(fp, "%s\n", buf)!=EOF) {
		if (!file_exist(buf))
			continue;
		mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			continue;
		fprintf(tmp, "%s\n", buf);
		if (mcuInfo->cash < 0) {
			mcEnv->Treasury += mcuInfo->cash;
			mcuInfo->cash = 0;
		}
		if (mcuInfo->credit < 0) {
			mcEnv->Treasury += mcuInfo->credit;
			mcuInfo->credit = 0;
		}
		if (mcuInfo->interest > 0) {
			cal_Interest = makeInterest(mcuInfo->credit, mcuInfo->depositTime, rate);
			mcuInfo->interest += cal_Interest;
			mcEnv->Treasury -= cal_Interest;
			mcuInfo->depositTime = time(NULL);
			mcuInfo->credit += mcuInfo->interest;
			mcuInfo->interest = 0;
		}
		mcEnv->AllUserMoney += mcuInfo->cash + 
				mcuInfo->credit + 
				mcuInfo->interest +
				mcuInfo->safemoney;
		mcEnv->userNum++;
		sleep(1);
	}
	flock(fileno(tmp), LOCK_UN);
	fclose(tmp);
	flock(fileno(fp), LOCK_UN);
	fclose(fp);
	rename(DIR_MC "mc_user.tmp", DIR_MC "mc_user");
	unlink(DIR_MC "mc_user.tmp");
	return;
}

static int
money_hall()
{
	char ch, quit = 0;

	while (!quit) {
		nomoney_show_stat("���������������");
		move(6, 4);
		prints
		    ("������%s����������������������ͨ�����ء����������������֡�"
		     "\n    ��˵����������������������������¼�������",
		     CENTER_NAME);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m ѡ�� \033[1;46m [1] �������а� [2] ����״̬ [3] �������� [4] �ܹܰ칫�� [Q] �뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			show_top();
			break;
		case '2':
			show_info();
			break;
		case '3':
			antiban();
			break;
		case '4':
			money_admin();
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
	}
	return 0;
}

static void
show_info()
{
	int pos, sellexpmoney, userSalary;
	float convertRate;
	char boss[STRLEN-1], title[78], buf[256];
	const char feaStr[][20] =
	{ "�ܹ�", "�����г�", "��Ʊ����", "�ĳ��ϰ�", "�ڰ��ϴ�",
		"ؤ�����", "֤��ᾭ��", "�̵꾭��", "����ֳ�"};
	if (myInfo->cash < 0) {
		mcEnv->Treasury += myInfo->cash;
		myInfo->cash = 0;
	}
	if (myInfo->credit < 0) {
		mcEnv->Treasury += myInfo->credit;
		myInfo->credit = 0;
	}
	if (mcEnv->Treasury > 10000000) {
		mcEnv->Treasury -= 5000000;
		sprintf(title, "�����⡿%s�ܹ�������5000000����%s",
				CENTER_NAME, MONEY_NAME);
		sprintf(buf, "    �ܹ�������5000000���Ƶ�%s\n", MONEY_NAME);
		deliverreport(title, buf);
		mcLog("����", 5000000, "���ƻ���");
	}
	if (mcEnv->Treasury < -2500000) {
		mcEnv->Treasury += 5000000;
		sprintf(title, "�����⡿%s�ܹ������ע����5000000%s",
				CENTER_NAME, MONEY_NAME);
		sprintf(buf, "    Ϊ���⾭��Σ�����ܹ���׼ӡ��5000000%s\n",
				MONEY_NAME);
		deliverreport(title, buf);
		mcLog("����", 5000000, "����");
	}
	clear();
	update_health();
	money_show_stat("����״̬");
	move(5, 0);
	prints("    ��ı������У�%11d %s\n", 
			myInfo->safemoney, MONEY_NAME);
	prints("    ��Ĵ��%15d %s\n", 
			myInfo->loan, MONEY_NAME);
	userSalary = makeSalary();
	prints("    ��Ĺ��ʣ�%15d %s\n", 
		userSalary, MONEY_NAME);
	convertRate = MIN_XY(bbsinfo.utmpshm->mc.ave_score / 1000.0 + 0.1, 10) * 1000;
	sellexpmoney = (countexp(currentuser) - myInfo->soldExp) * convertRate;
	prints("    ��������ɵã�%11d %s\n",
		sellexpmoney, MONEY_NAME);
	prints("    777������У�%12d %s\n",
		mcEnv->prize777, MONEY_NAME);
	sprintf(buf, "    �����%15Ld %s\n",
		mcEnv->Treasury, MONEY_NAME);
	prints(buf);
	prints("    �����ڣ�  ");
	if (seek_in_file(ROBUNION, currentuser->userid))
		prints("�ڰ���� ");
	if (seek_in_file(BEGGAR, currentuser->userid))
		prints("ؤ����� ");
	if (seek_in_file(POLICE, currentuser->userid))
		prints("���� ");
	prints("\n");
	prints("    ���ְ��");
	for (pos = 0; pos < 9; pos++) {
		whoTakeCharge(pos, boss);
		if (!strcmp(boss, currentuser->userid))
			prints("%s ", feaStr[pos]);
	}
	prints("\n");
	prints("    �����������ƣ�%11d ��\n", myInfo->antiban);
	prints("    ��Ļ������%11d ��\n", myInfo->Actived);
	prints("    ��ǰϵͳʱ���ǣ�  %s",
		ctime(&myInfo->aliveTime));
	if (userSalary > 0)
		prints("    �´η����ʵ�ʱ�䣺%s",
			ctime(&mcEnv->salaryEnd));
	if (myInfo->loan > 0)
		prints("    ��Ĵ����ʱ�䣺%s",
			ctime(&myInfo->backTime));
	prints("    ���ϴβ�Ǯ��ʱ�䣺%s",
		ctime(&myInfo->secureTime));
	prints("    ��ı��յ���ʱ�䣺%s",
		ctime(&myInfo->insuranceTime));
	prints("    ���ϴ��ж���ʱ�䣺%s",
		ctime(&myInfo->lastActiveTime));
	pressanykey();
	return;
}

static int
game_prize()
{
	int quit = 0;
	char ch;
	while (!quit) {
		money_show_stat("�齱��");
		move (4, 0);
		prints ("���������Ϸ����������а񣬿���������齱��һ���ϰ��¼ֻ�ܳ�һ�ν���");
		move (t_lines - 1, 0);
		prints
		    ("\033[1;37;44m ѡ�� \033[1;46m [1]������ [2]ɨ�� [3]��Ӧʽɨ�� [4]����˹���� [5]���� [Q]�뿪\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			choujiang(MY_BBS_HOME "/etc/worker/worker.rec", "������");
			break;
		case '2':
			choujiang(MY_BBS_HOME "/etc/winmine/mine2.rec", "ɨ��");
			break;
		case '3':
			choujiang(MY_BBS_HOME "/etc/winmine2/mine3.rec",
				  "��Ӧʽɨ��");
			break;
		case '4':
			choujiang(MY_BBS_HOME "/etc/tetris/tetris.rec",
				  "����˹����");
			break;
		case '5':
			choujiang(MY_BBS_HOME "/etc/tt/tt.dat", "������ϰ");
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
	}
	return 0;
}

static void
choujiang(char *recfile, char *gametype)
{
	int n, topS[20], topT[20], chou, jiang, tax;
	char topID[20][20], topFROM[20][20], prize[20][3], buf[256],
	    title[STRLEN];
	FILE *fp;

	fp = fopen(recfile, "r");
	chou = 0;
	if (!strcmp(gametype, "������")){
		for (n = 0; n <= 19; n++){
			fscanf(fp, "%s %d %d %s %s\n",
				topID[n], &topS[n], &topT[n], topFROM[n], prize[n]);
			if (!strcmp(currentuser->userid, topID[n])
				&& !strcmp(prize[n], "δ")){
				chou = n + 1;
			}
		}
	}else{
		for (n = 0; n <= 19; n++) {
			fscanf(fp, "%s %d %s %s\n", topID[n], &topT[n], topFROM[n],
		       		prize[n]);
			if (!strcmp(currentuser->userid, topID[n])
			    && !strcmp(prize[n], "δ")) {
				chou = n + 1;
			}
		}
	}
	fclose(fp);
	
	if (chou == 0) {
		showAt(6, 4, "\033[1;37m��Ŀǰ�޷��齱\033[m", YEA);
		return;
	}
	move(6, 4);
	prints("\033[1;37m���� %s �� %d ��\033[m", gametype, chou);
	move(7, 4);
	if (askyn("Ҫ�齱��", YEA, NA) == YEA) {
		jiang = 0;
		if (chou == 1)
			jiang = random() % 500 + 500;
		if (chou > 1 && chou <= 5)
			jiang = random() % 500 + 100;
		if (chou > 5 && chou <= 10)
			jiang = random() % 190 + 10;
		if (chou > 10 && chou <= 15)
			jiang = random() % 95 + 5;
		if (chou > 15 && chou <= 20)
			jiang = random() % 49 + 1;
		jiang = jiang * 1000;
		
		if (mcEnv->Treasury > jiang + 10000){
			tax = jiang - after_tax(jiang);
			mcEnv->Treasury -= jiang - tax;
			myInfo->cash += jiang - tax;
			strcpy(prize[chou-1], "��");
			myInfo->Actived += 2;
			sprintf(buf, "\033[1;32m�������� %d %s����˰��ʵ�� %d %s\033[m", 
				jiang, MONEY_NAME, jiang - tax, MONEY_NAME);
			showAt(8, 4, buf, YEA);
			sprintf(title, "���齱��%s ������ %d %s", 
				currentuser->userid, jiang, MONEY_NAME);
			sprintf(buf, "    %s �� %s ���˵� %d ����\n�齱�鵽 %d %s��\n"
					"    ��˰��ʵ�� %d %s\n",
				currentuser->userid, gametype, chou, jiang, MONEY_NAME, 
				jiang - tax, MONEY_NAME);
			deliverreport(title, buf);
			mcLog("�齱���", jiang, gametype);
		}else{
			showAt(8, 4, "�����ûǮ�ˣ��Ȼ��ٳ�ɡ�", YEA);
		}
		fp = fopen(recfile, "w");
		if (!strcmp(gametype, "������")){
			for (n = 0; n <= 19; n++)
				fprintf(fp, "%s %d %d %s %s\n", 
					topID[n], topS[n], topT[n], topFROM[n], prize[n]);
		}else{
			for (n = 0; n <= 19; n++){
				fprintf(fp, "%s %d %s %s\n", 
					topID[n], topT[n], topFROM[n], prize[n]);
			}
		}
		fclose(fp);
	} else {
		move(8, 4);
		prints("\033[1;32m������齱��\033[m");
	}
	return;
}

static void
quitsociety(int type)
{
	char title[70], content[256];
	move(6, 4);
	switch(type){
	case 0:
		if(askyn("��ȷʵҪ�˳��ڰ���", NA, NA) == NA)
			return;
		del_from_file(ROBUNION, currentuser->userid);
		showAt(8, 4, "���Ѿ��˳��˺ڰҪ�ú����˰���", YEA);
		sprintf(title, "���ڰ%s�˳��ڰ�", currentuser->userid);
		sprintf(content, "    ���Ѿ�����ϴ���ˡ�\n");
		break;
	case 1:
		if(askyn("��ȷʵҪ�˳�ؤ����", NA, NA) == NA)
			return;
		del_from_file(BEGGAR, currentuser->userid);
		showAt(8, 4, "���Ѿ��˳���ؤ�Ҫ�ú����˰���", YEA);
		sprintf(title, "��ؤ�%s�˳�ؤ��", currentuser->userid);
		sprintf(content, "    ���Ѿ��½������ˡ�\n");
		break;
	}
	deliverreport(title, content);
	return;
}

static void
mcLog(char *action, long long num, char *object)
{
	char buf[256];
	time_t currtime = time(NULL);
	sprintf(buf, "%24.24s %s %s %Ld %s\n", ctime(&currtime),
		currentuser->userid, action, num, object);
	addtofile(LOGFILE, buf);
	return;
}

//������ÿ����Ϸ����MONKEY_UNIT����ʼ�����3��5�е�ͼ����
//����5���ߣ�(0,0)-(0,1)-(0,2)-(0,3)-(0,4)��(0,0)-(1,1)-(2,2)-(1,3)-(0,4)
//           (1,0)-(1,1)-(1,2)-(1,3)-(1,4)
//           (2,0)-(1,1)-(0,2)-(1,3)-(2,4)��(2,0)-(2,1)-(2,2)-(2,3)-(2,4)
//���ǰ3��4��5����һ����ͼ�����ͻά����������DIR_MC "monkey_rate"
//ǰ12������4�ණ��3��4��5�������ʣ�����ӡ��Ϸ������
//��ͬͼ�����ʲ�һ��������Դ����κ�ͼ�����˺ͺ��Ӳ�������Щ��ϡ�
//���ǰ3��4��5�г����ˣ����10�������Ϸ��
//�����Ϸ����������г��ֺ��ӣ�̨�潱���(����Ŀ-1)
//�����Ϸ�г��ֵĺ�����Ŀ���ֻ����8��5��4��
static void
monkey_business(void)
{
	int i,j,MONKEY_UNIT, TempMoney=0, FreeGame=0, total=0;
	int quit=0, multi=0, man=0, monkey=1;
	char ch,
	     pic[12][2] ={"��","��","��","��","10","��",
		          "��","��","ʨ","��","��","��"};
	int pattern[3][5], sum[5]={12345,12345,12345,12345,12345}, winrate[100012];
	FILE *fp;

	MONKEY_UNIT = userInputValue(6, 4, "ÿ����Ϸ����",MONEY_NAME, 200, 10000);
	if(MONKEY_UNIT < 0) {
		showAt(8, 4, "������ң�", YEA);
		return;
	}
	mcLog("����MONKEY��ÿ�λ���", MONKEY_UNIT, "");
	if (myInfo->cash < MONKEY_UNIT) {
		showAt(8, 4, "ûǮ�ͱ���ˡ�", YEA);
		return;
	}

	fp = fopen(DIR_MC "monkey_rate", "r");
	for(i=0;i<100012;i++)
		fscanf(fp,"%d\n",&winrate[i]);
	fclose(fp);									

	while(!quit)
	{
		if (!(random() % MAX_ONLINE/5) && FreeGame <= 0)
			policeCheck();
		
		clear();
		money_show_stat("MONKEY BUSINESS");
		move(5, 0);
		prints("      --== ��Ϸ���� ==--      |\n");
		prints("                              |\n");
		prints("������5�����У�               |\n");
		prints("(1,1)\033[1;31m-\033[0;37m(1,2)\033[1;31m-\033[0;37m(1,3)"
			"\033[1;31m-\033[0;37m(1,4)\033[1;31m-\033[0;37m(1,5) |\033[m\n");
		prints("     \033[1;32m�v\033[0;37m   \033[1;35m�u\033[0;37m     "
			"\033[1;35m�v\033[0;37m   \033[1;32m�u\033[0;37m      |\033[m\n");
		prints("(2,1)\033[1;33m-\033[0;37m(2,2)\033[1;33m-\033[0;37m(2,3)"
			"\033[1;33m-\033[0;37m(2,4)\033[1;33m-\033[0;37m(2,5) |\033[m\n");
		prints("     \033[1;35m�u\033[0;37m   \033[1;32m�v\033[0;37m     "
			"\033[1;32m�u\033[0;37m   \033[1;35m�v\033[0;37m      |\033[m\n");
		prints("(3,1)\033[1;36m-\033[0;37m(3,2)\033[1;36m-\033[0;37m(3,3)"
			"\033[1;36m-\033[0;37m(3,4)\033[1;36m-\033[0;37m(3,5) |\033[m\n");
		prints("���ǰ3��ǰ4��ǰ5��ͼ����ͬ�� |\n");
		prints("����(��):  \033[1;44;37m 3\033[40m \033[46;37m 4\033[40m "
			"\033[44;37m 5\033[0;40;37m           |\033[m\n");
		prints("\033[1m���ˣѣ�10\033[32m \033[44;32m%2d\033[40m \033[46;32m%2d"
			"\033[40m \033[44;32m%2d\033[0;40;37m           |\033[m\n",
			winrate[0], winrate[1], winrate[2]);
		prints("\033[1m     �� ��\033[33m \033[44;33m%2d\033[40m \033[46;33m%2d"
			"\033[40m \033[44;33m%2d\033[0;40;37m           |\033[m\n",
			winrate[3], winrate[4], winrate[5]);
		prints("\033[1m        ��\033[35m \033[44;35m%2d\033[40m \033[46;35m%2d"
			"\033[40m \033[44;35m%2d\033[0;40;37m "
			"\033[5;1;31m��\033[0;37m���Դ���|\033[m\n",
			winrate[6], winrate[7], winrate[8]);
		prints("\033[1m     ʨ ��\033[31m \033[44;31m%2d\033[40m \033[46;31m%2d"
			"\033[40m \033[44;31m%2d\033[0;40;37m �κ�ͼ����|\033[m\n",
			winrate[9], winrate[10], winrate[11]);
		prints("���ǰ3��ǰ4��ǰ5�г���\033[1;33m��\033[0;37m���� |\033[m\n");
		prints("�ɻ��10�������Ϸ���ᣬ��ʱ��|\n");
		prints("����3�г���\033[1;33m����\033[0;37m����̨������ |\033[m\n");
		prints("���2����3����4��             |\n");
		
		move(5, 34);
		prints("���룺%d\t��һ����Ҫ��%d", myInfo->cash, MONKEY_UNIT);
		if (FreeGame > 0) {
			move(6, 34);
			prints("��Ѵ�����%d\t̨����룺%d", FreeGame, TempMoney);
		}
		
		showAt(7, 34, "���ո����ʼ��Ϸ����\033[1;32mQ\033[m�˳���", NA);

		ch=igetkey();

		if(myInfo->cash < MONKEY_UNIT) {
			showAt(8, 34, "��ûǮ�ˣ�ֻ���˳���Ϸ��", YEA);
			ch = 'q';
		}
		if(myInfo->health == 0)	{
			showAt(9, 34, "��û�����ˡ�ֻ���˳���Ϸ��", YEA);
			ch = 'q';
		}
		switch(ch) {
		case 'q':
		case 'Q':
			mcLog("�˳�MONKEY��Ӯ��",TempMoney,"");
			myInfo->cash += after_tax(TempMoney);
			TempMoney = 0;
			quit = 1;
			break;
		default:
			myInfo->health--;
			myInfo->Actived += 2;
			for (i=0;i<3;i++)
				for (j=0;j<5;j++) {
					if (j==2 && monkey)
						pattern[i][j]=random()%12;
					else
						pattern[i][j]=random()%11;
				}

			for(j=0;j<5;j++) {
				for(i=0;i<3;i++) {
					move(10 + i, 38 + j*3);
					prints("%2s", pic[pattern[i][j]]);
				}
				prints("\n");
				refresh();
				sleep(1);
			}			
			
			sum[0] = 10000 * (pattern[0][0] % 10) +
				  1000 * (pattern[0][1] % 10) +
				   100 * (pattern[0][2] % 10) +
				    10 * (pattern[0][3] % 10) +
				         pattern[0][4] % 10;
			sum[1] = 10000 * (pattern[0][0] % 10) +
				  1000 * (pattern[1][1] % 10) +
				   100 * (pattern[2][2] % 10) +
				    10 * (pattern[1][3] % 10) +
				         pattern[0][4] % 10;
			sum[2] = 10000 * (pattern[1][0] % 10) +
				  1000 * (pattern[1][1] % 10) +
				   100 * (pattern[1][2] % 10) +
				    10 * (pattern[1][3] % 10) +
				         pattern[1][4] % 10;
			sum[3] = 10000 * (pattern[2][0] % 10) +
				  1000 * (pattern[1][1] % 10) +
				   100 * (pattern[0][2] % 10) +
				    10 * (pattern[1][3] % 10) +
				         pattern[2][4] % 10;
			sum[4] = 10000 * (pattern[2][0] % 10) +
				  1000 * (pattern[2][1] % 10) +
				   100 * (pattern[2][2] % 10) +
				    10 * (pattern[2][3] % 10) +
				         pattern[2][4] % 10;
			
			if(pattern[0][0]==10) {
				if(sum[0] != 9999 && 
				   sum[0]/10 != 999 && 
				   sum[0]%1000 != 999)
					sum[0] = 12345;
				if(sum[1] != 9999 &&
				   sum[1]/10 != 999 &&
				   sum[1]%1000 != 999)
					sum[1] = 12345;
			}
                        if(pattern[0][1]==10)
				if(sum[0]%1000 != 999)
					sum[0] = 12345;
			if(pattern[0][2]==10 || pattern[0][2]==11) {
				sum[0] = 12345;
				sum[3] = 12345;
			}
			if(pattern[1][0]==10)
				if(sum[2] != 9999 &&
				   sum[2]/10 != 999 &&
				   sum[2]%1000 != 999)
					sum[2] = 12345;
			if(pattern[1][1]==10) {
				if(sum[1]%1000 != 999)
					sum[1] = 12345;
				if(sum[2]%1000 != 999)
					sum[2] = 12345;
				if(sum[3]%1000 != 999)
					sum[3] = 12345;
			}
			if(pattern[1][2]==10 || pattern[1][2]==11)
				sum[2] = 12345;
			if(pattern[2][0]==10) {
				if(sum[3] != 9999 &&
				   sum[3]/10 != 999 &&
				   sum[3]%1000 != 999)
					sum[3] = 12345;
				if(sum[4] != 9999 &&
				   sum[4]/10 != 999 &&
				   sum[4]%1000 != 999)
					sum[4] = 12345;
			}
			if(pattern[2][1]==10)
				if(sum[4]%1000 != 999)
					sum[4] = 12345;
			if(pattern[2][2]==10 || pattern[2][2]==11) {
				sum[1] = 12345;
				sum[4] = 12345;
			}
			
			if(pattern[0][3]==10) {
				if(pattern[0][0] != 0)
					sum[0] = sum[0] / 100 * 100;
				else if(pattern[0][1] != 0)
					sum[0] = sum[0] / 100 * 100;
				else if(pattern[0][2] != 0)
					sum[0] = sum[0] / 100 * 100;
				else
					sum[0] = 11;
			}
			if(pattern[0][4]==10) {
				if(pattern[0][0] != 0)
					sum[0] = sum[0] / 10 * 10;
				else if(pattern[0][1] != 0)
					sum[0] = sum[0] / 10 * 10;
				else if(pattern[0][2] != 0)
					sum[0] = sum[0] / 10 * 10;
				else if(pattern[0][3] != 0)
					sum[0] = sum[0] / 10 * 10;
				else
					sum[0] = 1;
				if(pattern[0][0] != 0)
					sum[1] = sum[1] / 10 * 10;
				else if(pattern[1][1] != 0)                                                                 
					sum[1] = sum[1] / 10 * 10;
				else if(pattern[2][2] != 0)                                                                 
					sum[1] = sum[1] / 10 * 10;
				else if(pattern[1][3] != 0)
					sum[1] = sum[1] / 10 * 10;
				else
					sum[1] = 1;
			}
			if(pattern[1][3]==10) {
				if(pattern[0][0] != 0)
					sum[1] = sum[1] / 100 * 100;
				else if(pattern[1][1] != 0)
					sum[1] = sum[1] / 100 * 100;
				else if(pattern[2][2] != 0)
					sum[1] = sum[1] / 100 * 100;
				else
					sum[1] = 11;
				if(pattern[1][0] != 0)
					sum[2] = sum[2] / 100 * 100;
				else if(pattern[1][1] != 0)
					sum[2] = sum[2] / 100 * 100;
				else if(pattern[1][2] != 0)
					sum[2] = sum[2] / 100 * 100;
				else
					sum[2] = 11;
				if(pattern[2][0] != 0)
					sum[3] = sum[3] / 100 * 100;
				else if(pattern[1][1] != 0)
					sum[3] = sum[3] / 100 * 100;
				else if(pattern[0][2] != 0)
					sum[3] = sum[3] / 100 * 100;
				else
					sum[3] = 11;
			}
			if(pattern[1][4]==10) {
				if(pattern[1][0] != 0)
					sum[2] = sum[2] / 10 * 10;
				else if(pattern[1][1] != 0)
					sum[2] = sum[2] / 10 * 10;
				else if(pattern[1][2] != 0)
					sum[2] = sum[2] / 10 * 10;
				else if(pattern[1][3] != 0)
					sum[2] = sum[2] / 10 * 10;
				else
					sum[2] = 1;
			}
			if(pattern[2][3]==10) {
				if(pattern[2][0] != 0)
					sum[4] = sum[4] / 100 * 100;
				else if(pattern[2][1] != 0)
					sum[4] = sum[4] / 100 * 100;
				else if(pattern[2][2] != 0)
					sum[4] = sum[4] / 100 * 100;
				else
					sum[4] = 11;
			}
			if(pattern[2][4]==10) {
				if(pattern[2][0] != 0)
					sum[3] = sum[3] / 10 * 10;
				else if(pattern[1][1] != 0)
					sum[3] = sum[3] / 10 * 10;
				else if(pattern[0][2] != 0)
					sum[3] = sum[3] / 10 * 10;
				else if(pattern[1][3] != 0)
					sum[3] = sum[3] / 10 * 10;
				else
					sum[3] = 1;
				if(pattern[2][0] != 0)
					sum[4] = sum[4] / 10 * 10;
				else if(pattern[2][1] != 0)
					sum[4] = sum[4] / 10 * 10;
				else if(pattern[2][2] != 0)
					sum[4] = sum[4] / 10 * 10;
				else if(pattern[2][3] != 0)
					sum[4] = sum[4] / 10 * 10;
				else
					sum[4] = 1;
			}

			total = 0;
			for(i=0;i<5;i++)
				  total += winrate[sum[i]+12];
			total *= MONKEY_UNIT;

			move(15, 34);
			if(total > 0)
				prints("��Ӯ�� %d %s", total,  MONEY_NAME);
			
			if(FreeGame > 0) {
				TempMoney += total;
				mcEnv->prize777 -= total;
				mcLog("��MONKEY.FreeGameӮ��", total, "");
				multi = 0;
				for(i=0;i<3;i++)
					if(pattern[i][2]==11)
						multi = 1;
				if(multi) {
					mcLog("MONKEY.FreeGamę����", TempMoney, "");
					mcEnv->prize777 -= TempMoney * (man - 2);
					TempMoney *= man - 1;
					monkey--;
					move(16, 34);
					prints("�����˺��ӣ���̨��Ľ����%d��",
							man - 1);
					mcLog("���ֺ��ӣ�̨��Ǯ��", man-1, "");
				}
				FreeGame--;
				if(FreeGame == 0) {
					TempMoney = after_tax(TempMoney);
					mcLog("MONKEY.FreeGame��Ӯ��", TempMoney, "");
					mcLog("MONKEY.FreeGame������", 9 - man - man/4 - monkey,"ֻ����");
					myInfo->cash += TempMoney;
					TempMoney = 0;
				}
			} else {
				man=0;
				for(j=0;j<5;j++){
					multi = 0;
					for(i=0;i<3;i++)
						if (pattern[i][j]==10)
							multi = 1;
					man += multi;
					if(multi == 0) {
						if(j < 3)
							man = 0;
						goto ENDMAN;
					}
				}
				ENDMAN:
			
				if(man >=3) {
					FreeGame = 10;
					TempMoney = 0;
					monkey = 9 - man - man/4;
					move(16, 34);
					prints("������%d���ˣ�����10�������Ϸ���ᡣ\n",
							man);
					mcLog("MONKEY�г���", man, "���ˣ�10��FreeGame");
					move(17, 34);
					prints("�����Ϸ��������ֺ��ӣ�"
						"��̨��Ľ��𽫻��%d��", man-1);
				}
				total = after_tax(total - MONKEY_UNIT);
				myInfo->cash += total;
				mcEnv->prize777 -= total;
				mcLog("��MONKEYӮ��", total, "");
			}
			move(18, 36);
			prints("\033[1;31m~~~~~  \033[32m~-_-~  \033[33m-----  "
					"\033[35m_-~-_  \033[36m_____\033[m");
			move(19, 34);
			for(i=0;i<5;i++)
				prints("%7d", sum[i]);
			move(20, 33);
			for(i=0;i<5;i++)
				prints("%7d", winrate[sum[i]+12]);
			pressanykey();
			break;
		}
	}
	return;
}

static void
policeCheck(void)
{
	char passbuf[32];
	int answer = 1;
	clear();
	nomoney_show_stat("����Ѳ��");
	move(5, 8);
	prints("=============== �� �� Ѳ �� ============");
	move(8, 0);
	prints
		("      ����ĳ��ٰ�Ƶ�������Ծ������о��ٽ���Ѳ�ӣ��̲����Ŀ�ꡣ"
		 "\n  ��ѽ�����ã������������˹�����");
	pressanykey();
	move(10, 6);
	if (random() % 3)
		prints
			("���û��ã�����Ŀ��б�ӵĴ�������߹�ȥ�ˡ�");
	else {
		prints
			("�������´�������һ�ۣ�����˹���˵������Ҫ���ȷ�ϣ���");
		getdata(11, 6, "����������: ",
				passbuf, PASSLEN, NOECHO, YEA);
		if (passbuf[0] == '\0' || passbuf[0] == '\n'
				|| !checkpasswd(currentuser->passwd,
					currentuser->salt, passbuf)) {
			getdata(13, 6, "����������ٴ�����(���һ�λ���): ",
					passbuf, PASSLEN, NOECHO, YEA);
			if (passbuf[0] == '\0' || passbuf[0] == '\n'
				|| !checkpasswd(currentuser->passwd,
					currentuser->salt, passbuf)) {
				answer = 0;
			} else if (random()%2)
				if (show_cake())
					answer = 0;
		} else if (random() % 2)
			if (show_cake())
				answer = 0;
		
		if (answer) {
			move(20, 0);
			prints
			("\n      ���ٵ��ͷ�����ţ�û��������ɡ���");
		} else {
			prints
				("\n      ����ŭ�𣺡������֤ʧ�ܣ�������һ�ˣ���"
				 "\n      555���㱻û�������ֽ𣬲��ұ��ϳ��������硣");
			mcLog("��777��������ߣ���ʧ", myInfo->cash, "�ֽ�");
			mcEnv->Treasury += myInfo->cash;
			myInfo->cash = 0;
			myInfo->mutex = 0;
			saveData(myInfo, sizeof (struct mcUserInfo));
			saveData(mcEnv, sizeof (struct MC_Env));
			pressreturn();
			Q_Goodbye();
		}
	}
	pressanykey();
}
/*
int
guess_number()
{
	int quit = 0, ch, num, money, a, b, c, win, count;
	char ans[5] = "";
	int bet[7] = { 0, 100, 50, 10, 5, 2, 1 };
	srandom(time(0));
	while (!quit) {
		clear();
		money_show_stat("��Ϳ��������֮·...");
		move(4, 4);
		prints("\033[1;31m�����Խ�׬Ǯ��~~~\033[m");
		move(5, 4);
		prints("��Сѹ 100 ��Ϳ�ң�����999");
		move(t_lines - 1, 0);
		prints
		("\033[1;44m ѡ�� \033[1;46m [1]��ע [Q]�뿪                                                \033[m");
		ch = igetkey();
		switch (ch) {
			case '1':
				win = 0;
				getdata(8, 4, "��ѹ���ٺ�Ϳ�ң�[999]", genbuf, 4,
						DOECHO, YEA);
				num = atoi(genbuf);
				if (!genbuf[0])
					num = 999;
				if (num < 100) {
					move(9, 4);
					prints("��û��Ǯ������ô��Ǯ���ǲ������");
					pressanykey();
					break;
				}
				sprintf(genbuf,
						"��ѹ���� \033[1;31m%d\033[m ��Ϳ�ң�ȷ��ô��",
						num);
				move(9, 4);
				if (askyn(genbuf, YEA, NA) == YEA) {
					money = load_money(currentuser.userid);
					if (money < num) {
						move(11, 4);
						prints("ȥȥȥ��û��ô��Ǯ��ʲô��");
						pressanykey();
						break;
					}
					if (num > 999)
						num = 999;
					do {
						itoa(random() % 10000, ans);
						for (a = 0; a < 3; a++)
							for (b = a + 1; b < 4; b++)
								if (ans[a] == ans[b])
									ans[0] = 0;
					} while (!ans[0]);
					for (count = 1; count < 7; count++) {
						do {
							move(10, 4);
							prints
								("�������ĸ����ظ�������");
							getdata(11, 4,
									"���[q - �˳�] �� ",
									genbuf, 5, DOECHO, YEA);
							if (genbuf[0] == 'q'
									|| genbuf[0] == 'Q') {
								utmpshm->allprize +=
									num;
								if (utmpshm->allprize >
										1000000000)
									utmpshm->
										allprize =
										1000000000;
								move(12, 4);
								save_money(currentuser.
										userid,
										-num);
								prints("byebye!");
								pressanykey();
								quit = 1;
								return 0;
							}
							c = atoi(genbuf);
							itoa(c, genbuf);
							for (a = 0; a < 3; a++)
								for (b = a + 1; b < 4;
										b++)
									if (genbuf[a] ==
											genbuf[b])
										genbuf
											[0]
											= 0;
							if (!genbuf[0]) {
								move(12, 4);
								prints
									("��������������!!");
								pressanykey();
								move(12, 4);
								prints
									("                ");
							}
						} while (!genbuf[0]);
						move(count + 13, 0);
						prints("  �� %2d �Σ� %s  ->  %dA %dB ",
								count, genbuf, an(genbuf, ans),
								bn(genbuf, ans));
						if (an(genbuf, ans) == 4)
							break;
					}
					move(12, 4);
					if (count > 6) {
						sprintf(genbuf,
								"�������ϣ���ȷ���� %s���´��ټ��Ͱ�!!",
								ans);
						sprintf(genbuf,
								"\033[1;31m����û�µ������� %d Ԫ��\033[m",
								num);
						utmpshm->allprize += num;
						if (utmpshm->allprize > 1000000000)
							utmpshm->allprize = 1000000000;
						save_money(currentuser.userid, -num);
					} else {
						int oldmoney = num;
						num *= bet[count];
						if (num - oldmoney > 0) {
							sprintf(genbuf,
									"��ϲ���ܹ����� %d �Σ���׬���� %d Ԫ",
									count, num);
							save_money(currentuser.userid,
									num);
							win = 1;
						} else if (num - oldmoney == 0)
							sprintf(genbuf,
									"�������ܹ����� %d �Σ�û��ûӮ��",
									count);
						else {
							utmpshm->allprize += oldmoney;
							if (utmpshm->allprize >
									1000000000)
								utmpshm->allprize =
									1000000000;
							sprintf(genbuf,
									"�������ܹ����� %d �Σ���Ǯ %d Ԫ��",
									count,
									oldmoney - money);
							save_money(currentuser.userid,
									-num);
						}
					}
					prints("���: %s", genbuf);
					move(13, 4);
					pressanykey();
				}
				break;
			case 'Q':
			case 'q':
				quit = 1;
				break;
		}
	}
	return 0;
}

int
an(a, b)
char *a, *b;
{
	int i, k = 0;
	for (i = 0; i < 4; i++)
		if (*(a + i) == *(b + i))
			k++;
	return k;
}

int
bn(a, b)
char *a, *b;
{
	int i, j, k = 0;
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			if (*(a + i) == *(b + j))
				k++;
	return (k - an(a, b));
}

void
itoa(i, a)
int i;
char *a;
{
	int j, k, l = 1000;
	//prints("itoa: i=%d ", i);
	for (j = 3; j > 0; j--) {
		k = i - (i % l);
		i -= k;
		k = k / l + 48;
		a[3 - j] = k;
		l /= 10;
	}
	a[3] = i + 48;
	a[4] = 0;
}*/
