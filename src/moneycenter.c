#include <sys/mman.h>		// for mmap
#include "bbs.h"
#include "bbstelnet.h"

#define MONEYCENTER_VERSION         1
#define DIR_MC          MY_BBS_HOME "/etc/moneyCenter/"
#define LOGFILE		DIR_MC "log"			//大富翁系统记录 add by yxk
#define DIR_STOCK MY_BBS_HOME "/etc/moneyCenter/stock/"
#define DIR_MC_TEMP     MY_BBS_HOME "/bbstmpfs/tmp/"
#define MC_BOSS_FILE    DIR_MC "mc_boss"
// 贺卡店每种类型的贺卡建一个子目录, 贺卡名取{1, 2, ... , n} 
#define DIR_SHOP        DIR_MC "/cardshop/"
//各种金额限制 
#define PRIZE_PER       1000000	// 固定奖金
#define MAX_POOL_MONEY  5000000	// 最大奖金
#define MAX_MONEY_NUM 300000000
#define STOCK_NUM            10	// 个人最多持有股票数

#define MONEY_NAME "天天币"	//如："糊涂币"
#define CENTER_NAME MY_BBS_NAME	//如："一塌糊涂"
#define ROBUNION	DIR_MC "robmember"		//黑帮成员名单 add by yxk
#define BEGGAR		DIR_MC "begmember"		//丐帮成员名单 add by yxk
#define POLICE		DIR_MC "policemen"		//警察成员名单
#define CRIMINAL	DIR_MC "criminals_list"		//通缉犯名单 add by yxk
#define BADID		DIR_MC "bannedID"		//禁止进入大富翁 add by yxk
#define MAX_ONLINE 300		//控制随机事件几率
#define BRICK_PRICE 200		//砖头单价
#define PAYDAY 5		//发工资周期
#define LISTNUM 60		//排行镑记录数目
//#define MONKEY_UNIT 250		//玩一次Monkey_business所需要的钱 add by yxk

struct LotteryRecord {		//彩票买注记录 
	char userid[IDLEN + 1];
	int betCount[5];
};

struct myStockUnit {		//个人持有股票记录单元 
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
};				// 32 bytes, 交易单 

struct mcUserInfo {
	char version, mutex, temp[2];
	int cash, credit, loan, interest;
	short robExp, policeExp, begExp, guard;	//robExp 胆识 begExp 身法 guard 狼狗 
	time_t aliveTime, freeTime;
	time_t loanTime, backTime, depositTime;
	time_t lastSalary, lastActiveTime;
	struct myStockUnit stock[STOCK_NUM];
	int soldExp;
	int health, luck;	//helath 体力 luck 人品 
	int Actived;		//行动次数，随机事件出现条件之一 
	int safemoney;
	time_t secureTime;
	time_t insuranceTime;
	int antiban;
	int unused[12];
};				// 256 bytes 个人数据 

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
// 加载全局参数和我的数据 
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
// 检查是否可以进入 
	retv = check_allow_in();
	mcLog("进入大富翁世界", retv, "");
	if (retv == 0)
		goto UNMAP;
	if (retv == -1)
		goto MUTEX;
//myInfo->aliveTime = time(NULL); //自动更新体力转移到从进站开始 main.c 

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
			prints("突然之间，大富翁世界剧烈的旋转起来。。。");
			myInfo->Actived = 0;
			pressanykey();
			randomevent();
		}
		clear();
		nomoney_show_stat("十字路口");
		move(6, 0);
		prints("\033[33;41m " MY_BBS_NAME " \033[m\n\n");
		prints
		    ("\033[1;33m        ▁▁▁ ▔█▔         ▁▁╱          ▁▁╱\n"
		     "\033[1;33m          █  █▔▔█  ▁▁  █ | █   ▁▁  █ | █    \033[31m  ▁▁▁▁▁\033[m\n"
		     "\033[1;33m          █  █ █ █  █ ● █ | █   █ ● █ | █    \033[31m▕大富翁世界▏\033[m\n"
		     "\033[1;33m        —█ ▁╱ ●    ▔▔▁◤ ●◥◣ ▔▔▁◤ ●◥◣  \033[31m  ▔▔▔▔▔\033[m");
		move(t_lines - 2, 0);
		prints
		    ("\033[1;44m 你要去 \033[1;46m [0]中央大厅 [1]国有银行 [2]彩票中心 [3]百变股市 [4]警署\033[m");
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m        \033[1;46m [5]超级赌场 [6]黑帮总部 [7]丐帮主舵 [8]百货公司 [Q]离开\033[m");
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
		if(askyn("您的现金过多，需要缴税。你要交吗？", YEA, NA) == NA) {
			prints("\n    你偷税漏税，被罚款%d偿还所欠税款，并被监禁30分钟！", tax_paid);
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
		prints("您交了%d的税，胆识、身法、人品各增加%d点！", 
				tax_paid, tax_paid / 110000);
		pressanykey();
	}
	if (myInfo->credit > no_tax_money && !(random() % (MAX_ONLINE / 2))) {
		tax_paid = myInfo->credit - no_tax_money -
			   after_tax(myInfo->credit - no_tax_money);
		myInfo->credit -= tax_paid;
		clear();
		move(4, 4);
		if(askyn("您的存款过多，需要缴税。你要交吗？", YEA, NA) == NA) {
			prints("\n    你偷税漏税，被罚款%d偿还所欠税款，并被监禁30分钟！", tax_paid);
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
		prints("您交了%d的税，胆识、身法、人品各增加%d点！", 
				tax_paid, tax_paid / 110000);
		pressanykey();
	}
	return;
}

/*   体力更新 add by koyu */
static void
update_health()
{
	time_t curtime;
	int add_health;

	curtime = time(NULL);
	add_health = (curtime - myInfo->aliveTime) / 20;	//每20秒体力加1 
	if (myInfo->health < 0)
		myInfo->health = 0;
	myInfo->health = MIN_XY((myInfo->health + add_health), 100);
	myInfo->aliveTime += add_health * 20;

//人品[-100, 100]
	if (myInfo->luck > 100)
		myInfo->luck = 100;
	if (myInfo->luck < -100)
		myInfo->luck = -100;
//胆识身法上限30000. add by yxk
	if (myInfo->robExp > 30000 || myInfo->robExp < -10000)
		myInfo->robExp = 30000;
	if (myInfo->begExp > 30000 || myInfo->begExp < -10000)
		myInfo->begExp = 30000;
//金钱上限20亿. add by yxk
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

/*  检查行动需要的体力，体力够返回0，不够显示消息返回1  add by koyu */
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
	prints("              \033[1;32m这是一个砖块横飞的世界！\n\n");

	prints("              \033[1;32m这是一个梦与冒险的世界！\033[m\n\n");

	prints("              \033[1;31m在这里，适者生存！\033[m\n\n");

	prints("              \033[1;31m在这里，强者为王！\033[m\n\n");

	prints
	    ("              \033[1;35m【友情提醒】钱财为身外之物 胜败乃玩家常事\033[m\n\n\n");

	prints
	    ("                    \033[0;1;33m★%s 2005大富翁世界欢迎您★  \033[0m",
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
	
	//初始化
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
	
	//记录文件不存在
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
			deliverreport("系统故障",
					"\033[1;31m排名记录文件已经存在！\033[m\n");
		}
		return;
	}
	
	//记录文件存在
	flock(fileno(fp), LOCK_EX);
	//读记录
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %d\n", topIDM[n], &topMoney[n]);
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %hd\n", topIDR[n], &topRob[n]);
	for (n = 0; n < LISTNUM; n++)
		fscanf(fp, "%s %hd\n", topIDB[n], &topBeg[n]);
	//ID已上榜
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
	//ID未上榜
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
	//排序	
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
	//写入新文件
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
	showAt(10, 14, "\033[1;32m期待您再次光临大富翁世界，"
	       "早日实现您的梦想。\033[m", YEA);
	mcLog("退出大富翁", 0, "");
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
		prints("\033[1;44;37m  " MY_BBS_NAME " BBS    \033[32m--== 富  翁  榜 ==-- "
		       "\033[33m--== 胆  识  榜 ==-- \033[36m--== 身  法  榜 ==--\033[m\r\n");
		prints("\033[1;41;37m   排名           名字       总资产         名字    "
				"胆识         名字    身法 \033[m\r\n");
	
		for (n = 0; n < 20; n++)
			prints("\033[1;37m%6d\033[32m%18s\033[33m%11d\033[32m%15s"
				"\033[33m%6d\033[32m%15s\033[33m%6d\033[m\r\n", 
				m*20+n+1, topIDM[m*20+n], topMoney[m*20+n],
				topIDR[m*20+n], topRob[m*20+n], topIDB[m*20+n], topBeg[m*20+n]);
		prints("\033[1;41;33m                    这是第%2d屏，按任意键查看下一屏"
			"                            \033[m\r\n", m+1);
		pressanykey();
	}
}

static int
getOkUser(char *msg, char *uident, int line, int col)
{				// 将输入的有效id放到uident里, 成功返回1, 否则返回0 

	move(line, col);
	usercomplete(msg, uident);
	if (uident[0] == '\0')
		return 0;
	if (!getuser(uident, NULL)) {
		showAt(line + 1, col, "错误的使用者代号...", YEA);
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

	if (type == 0) {	//初始化全局数据 
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
	} else if (type == 1) {	//初始化个人数据 
		filesize = sizeof (struct mcUserInfo);
		bzero(&ui, filesize);
		ui.version = MONEYCENTER_VERSION;
		ptr = &ui;
		ui.credit = 50000;
		mcEnv->Treasury -= 50000;
		addtofile(DIR_MC "mc_user", filepath);
	} else {		//初始化股市 
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

	snprintf(content, STRLEN - 1, "%s多少%s？[%d--%d]", act, name, inf,
		 sup);
	getdata(line, col, content, buf, 10, DOECHO, YEA);
	num = atoi(buf);
	num = MAX_XY(num, inf);
	num = MIN_XY(num, sup);
	move(line + 1, col);
	snprintf(content, STRLEN - 1, "确定%s %d %s吗？", act, num, name);
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
		int type, bonus;	//type:1 现金 2 胆识  3 身法 4 运气 5 体力 6 存款 etc 
	} rd_event[] = {	//注意排列顺序，人品越好，遇到上面的可能性越大 
		{
		"慷慨赞助" MY_BBS_NAME, "所有参数增加", "", 7, 5}, {
		"配合" MY_BBS_NAME "政府征地", "获得了", "赔偿金", 6, 2}, {
		"遇到大财神", "得到了", "的存款", 6, 5}, {
		"遇到小财神", "得到了", "的奖励", 0, 5}, {
		"感冒去校医院", "被误诊体力减少到", "点", 5, -20}, {
		"横渡英吉利海峡", "胆识增加了", "点", 2, 15}, {
		"遇到有人在地铁放屁", "体力减少到", "", 5, -40}, {
		"得到易筋经", "身法提高了", "点", 3, 15}, {
		"喝了脑白痴", "体力减少到", "", 5, -100}, {
		"早锻炼遇到FR jj", "胆识增加", "点", 2, 10}, {
		"新买自行车", "花费", "的现金", 1, -50000}, {
		"早锻炼遇到FR jj", "学习舞蹈后身法提高了", "点", 3, 10}, {
		"看贴不回贴", "人品降低", "点", 4, -5}, {
		"对意中人勇敢表白", "胆识增加", "点", 2, 10}, {
		"对意中人勇敢表白", "为此采购道具花了", "的现金", 1, -100000},{
		"得到电机车", "身法提高了", "点", 3, 10}, {
		"非典期间倒卖醋", "人品减少", "点", 4, -10}, {
		"见义勇为", "胆识增加", "点", 2, 5}, {
		"偷拍mm洗澡", "人品降低", "点", 4, -10}, {
		"不好好学习", "逃课打篮球身法增加", "点", 3, 5}, {
		"贪小便宜买了假货", "损失", "的现金", 1, -200000}, {
		"感冒去校医院", "胆识增加", "点", 2, 5}, {
		"买了新车Passat", "花了", "", 1, -200000}, {
		"新买自行车", "身法提高了", "点", 3, 5}, {
		"一心多用劈腿", "人品减少", "点", 4, -20}, {
		"孝敬父母", "人品提升", "点", 4, 20}, {
		"开车撞上电线杆", "身法降低了", "点", 3, -5}, {
		"非典期间倒卖醋", "赚了", "不义之财", 1, 200000}, {
		"违章被警察拦住", "胆识降低了", "点", 2, -5}, {
		"协助警察抓住劫匪", "获得", "奖励", 1, 200000}, {
		"遇到疯狗", "被咬伤后身法降低了", "点", 3, -5}, {
		"抵制日货", "人品提升", "点", 4, 10}, {
		"跟泼妇吵架被抓破脸", "胆识降低了", "点", 2, -5}, {
		"英雄救美", "人品增加", "点", 4, 10}, {
		"在公交车上被性骚扰", "胆识减少了", "点", 2, -10}, {
		"拣到钱包", "翻出来", "偷偷放进了自己口袋。", 1, 100000}, {
		"被小流氓打伤", "身法降低", "点", 3, -10}, {
		"拾金不昧", "人品提升", "点", 4, 5}, {
		"晚上回家路上被抢劫", "身法降低", "点", 3, -10}, {
		"卖废品", "赚了", "", 1, 50000}, {
		"被黑帮绑架", "胆识减少了", "点", 2, -10}, {
		"吃了千年人参", "体力增加到", "", 5, 100}, {
		"遇到痴女对他表白", "胆识减少了", "点", 2, -15}, {
		"服下大力丸", "体力增加到", "", 5, 40}, {
		"在游乐场不慎摔伤", "身法降低", "点", 3, -15}, {
		"坚持锻炼身体", "体力增加到", "点", 5, 20}, {
		"遇到小衰神", "丢失了", "的现金", 0, -5}, {
		"遇到大衰神", "丢了一张", "的存折", 6, -5}, {
		"慷慨赞助" MY_BBS_NAME, "花费了", "的存款", 6, -2}, {
		"企图不利于" MY_BBS_NAME, "全部指数下降", "", 7, -5}
	};

	money_show_stat("神秘世界");

	mcLog("进入神秘世界", 0, "");
	move(4, 4);
	prints("不知道过了多久，你慢慢醒了过来，发现自己到了一个神秘的地方。\n"
	       "    你正奇怪，突然听到一个声音对你说到：”欢迎来到神秘世界！“\n"
	       "    你回答正确一个问题后，就会遇到一次随机事件，祝你好运！");
	pressreturn();
	money_show_stat("神秘世界");
	if (check_health
	    (40, 12, 4,
	     "啊哦，您现在的体力不够承受这么刺激的事情，还是算了吧。。。", YEA))
		return;

	if (show_cake()) {
		prints("你别是个机器人吧...\n");
		pressreturn();
		return;
	}

	clear();
	update_health();
	myInfo->health -= 10;
	money_show_stat("神秘世界");

	rat = EVENT_NUM * myInfo->luck * 3 / 800;
	getrandomint(&num);
	num = num % (EVENT_NUM - abs(rat));
	if (rat < 0)
		num -= rat;

	num = abs(num) % EVENT_NUM;	//上面几行在算什么？限制在数组界内吧
	
	switch (rd_event[num].type) {
	case 0:		//金钱百分比 
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
		prints(total > 0 ? "\033[1;31m你%s，%s%d%s%s。\033[m" :
		       "\033[1;34m你%s，%s%d%s%s。\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), MONEY_NAME,
		       rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 100000)
			break;
		sprintf(title, "【事件】%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s，%s%d%s%s。\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			MONEY_NAME, rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 1:		//金钱数量 
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
                prints(total > 0 ? "\033[1;31m你%s，%s%d%s%s。\033[m" :
		       "\033[1;34m你%s，%s%d%s%s。\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), MONEY_NAME,
		       rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 100000)
			break;
		sprintf(title, "【事件】%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s，%s%d%s%s。\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			MONEY_NAME, rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 2:		//胆识 
		total = rd_event[num].bonus;
		move(6, 4);
		myInfo->robExp = MAX_XY(0, (myInfo->robExp + total));
                prints(total > 0 ? "\033[1;31m你%s，%s%d%s。\033[m" :
		       "\033[1;34m你%s，%s%d%s。\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 10)
			break;
		sprintf(title, "【事件】%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s，%s%d%s。\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 3:		//身法 
		total = rd_event[num].bonus;
		move(6, 4);
		myInfo->begExp = MAX_XY(0, (myInfo->begExp + total));
                prints(total > 0 ? "\033[1;31m你%s，%s%d%s。\033[m" :
		       "\033[1;34m你%s，%s%d%s。\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 10)
			break;
		sprintf(title, "【事件】%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s，%s%d%s。\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 4:		//人品 
		total = rd_event[num].bonus;
		move(6, 4);
		myInfo->luck = MAX_XY(-100, (myInfo->luck + total));
                prints(total > 0 ? "\033[1;31m你%s，%s%d%s。\033[m" :
		       "\033[1;34m你%s，%s%d%s。\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), rd_event[num].desc3);
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		//sprintf(title, "【事件】%s%s", currentuser->userid,
		//	rd_event[num].desc1);
		//sprintf(buf, "%s%s，%s%d%s。", currentuser->userid,
		//	rd_event[num].desc1, rd_event[num].desc2, abs(total),
		//	rd_event[num].desc3);
		break;
	case 5:		//体力 
		update_health();
		total = MIN_XY(100, rd_event[num].bonus + myInfo->health);
		move(6, 4);
		myInfo->health = MAX_XY(total, 0);
                prints(rd_event[num].bonus > 0 ? "\033[1;31m你%s，%s%d%s。\033[m" :
		       "\033[1;34m你%s，%s%d%s。\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, myInfo->health, rd_event[num].desc3);
		mcLog(rd_event[num].desc2, myInfo->health, rd_event[num].desc3);
		//sprintf(title, "【事件】%s%s", currentuser->userid,
		//	rd_event[num].desc1);
		//sprintf(buf, "%s%s，%s%d%s。", currentuser->userid,
		//	rd_event[num].desc1, rd_event[num].desc2,
		//	myInfo->health, rd_event[num].desc3);
		break;
	case 6:		//存款百分比 
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
                prints(total > 0 ? "\033[1;31m你%s，%s%d%s%s。\033[m" :
		       "\033[1;34m你%s，%s%d%s%s。\033[m", rd_event[num].desc1,
		       rd_event[num].desc2, abs(total), MONEY_NAME,
		       rd_event[num].desc3);
		if (rd_event[num].bonus <= 3) {
			myInfo->health = 0;
			move(7, 4);
			prints("经历了这么大的事，你忙得虚脱了。");
		}
		mcLog(rd_event[num].desc2, total, rd_event[num].desc3);
		if (total < 100000)
			break;
		sprintf(title, "【事件】%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s，%s%d%s%s。\n", currentuser->userid,
			rd_event[num].desc1, rd_event[num].desc2, abs(total),
			MONEY_NAME, rd_event[num].desc3);
		deliverreport(title, buf);
		break;
	case 7:		//全部
		total = MIN_XY(myInfo->cash / rd_event[num].bonus, 2000000);
		total = MIN_XY(total, mcEnv->Treasury / 2);
		myInfo->cash += total;
		mcEnv->Treasury -= total;
		mcLog(rd_event[num].desc2, total, "现金");
		total = MIN_XY(myInfo->credit / rd_event[num].bonus, 2000000);
		total = MIN_XY(total, mcEnv->Treasury / 2);
		myInfo->credit += total;
		mcEnv->Treasury -= total;
		mcLog(rd_event[num].desc2, total, "存款");
		total = MIN_XY(myInfo->robExp / rd_event[num].bonus, 30);
		myInfo->robExp += total;
		mcLog(rd_event[num].desc2, total, "胆识");
		total = MIN_XY(myInfo->begExp / rd_event[num].bonus, 30);
		myInfo->begExp += total;
		mcLog(rd_event[num].desc2, total, "身法");
		total = abs(myInfo->luck) / rd_event[num].bonus;
		myInfo->luck += total;
		mcLog(rd_event[num].desc2, total, "人品");
		myInfo->health = 0;
		move(6, 4);
                prints(total > 0 ? "\033[1;31m你%s，%s。\033[m" :
		       "\033[1;34m你%s，%s。\033[m", rd_event[num].desc1,
		       rd_event[num].desc2);
		prints("\n    经历那么大的事，你忙的虚脱了。");
		sprintf(title, "【事件】%s%s", currentuser->userid,
			rd_event[num].desc1);
		sprintf(buf, "    %s%s，%s。\n", currentuser->userid,
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
{				// 计算利息 
	int calHour, Interest;
	time_t currTime = time(NULL);

	if (lastTime > 0 && currTime > lastTime) {
		calHour = (currTime - lastTime) / 3600;
		calHour = MIN_XY(calHour, 87600);
		Interest = basicMoney * rate * calHour / 24;
		if(Interest > 0)
			mcLog("获得", Interest, "利息");
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
		prints("\033[1;5;36m[您有信件]\033[m");
	}
	if (time(NULL) < myInfo->freeTime) {
		move(12, 4);
		prints("    啊，你上次做的事犯了！你被警察抓获了！");
		myInfo->mutex = 0;
		saveData(myInfo, sizeof (struct mcUserInfo));
		saveData(mcEnv, sizeof (struct MC_Env));
		pressreturn();
		Q_Goodbye();
		mcLog("被警察抓获", 0, "");
	}

	move(1, 0);
	update_health();
	prints
	    ("您的代号：\033[1;33m%s\033[m      胆识 \033[1;33m%d\033[m  身法 \033[1;33m%d\033[m  人品 \033[1;33m%d\033[m  体力 \033[1;33m%d\033[m \n",
	     currentuser->userid, myInfo->robExp, myInfo->begExp, myInfo->luck,
	     myInfo->health);
	prints("您身上带着 \033[1;31m%d\033[m %s，", myInfo->cash, MONEY_NAME);
	prints("存款 \033[1;31m%d\033[m %s。当前位置 \033[1;33m%s\033[m",
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
		prints("\033[1;5;36m[您有信件]\033[m");
	}
	if (time(NULL) < myInfo->freeTime) {
		move(12, 4);
		prints("    啊，你上次做的事犯了！你被警察抓获了！");
		myInfo->mutex = 0;
		saveData(myInfo, sizeof (struct mcUserInfo));
		saveData(mcEnv, sizeof (struct MC_Env));
		pressreturn();
		mcLog("被警察抓获", 0, "");
		Q_Goodbye();
	}
	update_health();
	if (myInfo->luck > 100)
		myInfo->luck = 100;
	if (myInfo->luck < -100)
		myInfo->luck = -100;
	move(1, 0);
	prints
	    ("您的代号：\033[1;33m%s\033[m      胆识 \033[1;33m%d\033[m  "
	     "身法 \033[1;33m%d\033[m  人品 \033[1;33m%d\033[m  体力 \033[1;33m%d\033[m \n",
	     currentuser->userid, myInfo->robExp, myInfo->begExp, myInfo->luck,
	     myInfo->health);
	prints
	    ("\033[1;32m欢迎光临%s大富翁世界，当前位置是\033[0m \033[1;33m%s\033[0m",
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
	if (mcEnv->closed) {	/* 大富翁世界关闭 */
		showAt(10, 10, "大富翁世界关闭中...请稍后再来", YEA);
		whoTakeCharge(0, uident);
		if (!USERPERM(currentuser, PERM_SYSOP) && 
			strcmp(currentuser->userid, uident))
			return 0;
		move(12, 10);
		if(askyn("你想要进入大富翁世界进行管理操作吗？", NA, NA) == YEA)
			money_admin();
		return 0;
	}
	if (mcEnv->openTime > currentuser->lastlogin) {
		showAt(10, 4, "由于修改了代码，你需要退出所有窗口再重新登录才允许进入大富翁世界", YEA);
		return 0;
	}
		
	if (myInfo->mutex++ && count_uindex_telnet(usernum) > 1) {	// 避免多窗口, 同时处理掉线 
		showAt(10, 10, "你已经在大富翁世界里啦!", YEA);
		return -1;
	}
/* 犯罪被监禁 */
	clrtoeol();
	if (currTime < myInfo->freeTime) {
		day = (myInfo->freeTime - currTime) / 86400;
		hour = (myInfo->freeTime - currTime) % 86400 / 3600;
		minute = (myInfo->freeTime - currTime) % 3600 / 60 + 1;
		if (seek_in_file(POLICE, currentuser->userid))
			prints("你执行任务受伤还需要修养%d天%d小时%d分钟。",
			       day, hour, minute);
		else {
			prints("你被%s警署监禁了。还有%d天%d小时%d分钟的监禁。",
			       CENTER_NAME, day, hour, minute);
			move(12, 0);
			if (askyn("    你要贿赂狱警吗？", NA, NA) == NA) {
				whoTakeCharge(0, uident);
				if (!USERPERM(currentuser, PERM_SYSOP) &&
						strcmp(currentuser->userid, uident))
					return 0;
				move(14, 10);
				if(askyn("你想要进入大富翁世界进行管理操作吗？", NA, NA) == YEA)
					money_admin();									
				return 0;
			}
			num = userInputValue(14, 4, "贿赂他","万" MONEY_NAME,
					1, myInfo->credit / 10000);
			if (num < 1) {
				showAt(16, 4, "你不干了……", YEA);
				return 0;
			}
			if (num * 10000 > myInfo->credit) {
				showAt(16, 4, "你没有那么多存款！", NA);
				return 0;
			}
			myInfo->credit -= num * 10000;
			mcEnv->Treasury += num * 10000;
			myInfo->freeTime -= num * 60;
			mcLog("贿赂", num, "万 给狱警");
			if (currTime > myInfo->freeTime) {
				myInfo->freeTime = 0;
				showAt(16, 4, "狱警收了你的贿赂，偷偷把你放了。", YEA);
				return 1;
			}
			if (currTime < myInfo->freeTime) {
				move(16, 4);
				prints("狱警收了你的贿赂，你的监禁时间缩短了%d分钟", num);
			}
		}
		pressanykey();
		return 0;
	} else if (currTime > myInfo->freeTime && myInfo->freeTime > 0) {
		myInfo->freeTime = 0;
		if (seek_in_file(POLICE, currentuser->userid))
			showAt(10, 10, "恭喜你伤愈出院！", YEA);
		else
			showAt(10, 10, "监禁期满，恭喜你重新获得自由！", YEA);
	}
	clrtoeol();
/* 欠款不还 */
	if (currTime > myInfo->backTime && myInfo->backTime > 0) {
		if (askyn("你欠银行的贷款到期了，赶紧还吧？", YEA, NA) == NA)
			return 0;
		money_bank();
		return 0;
	}
// 坏人名单
	if(seek_in_file(BADID, currentuser->userid)) {
		showAt(10, 10, "你扰乱大富翁世界金融秩序，被取消进入大富翁世界的资格。\n"
			       "          请与大富翁总管联系。", YEA);
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
		sprintf(buf, "    请于%d天内到%s银行领取，过期视为放弃。\n",
				PAYDAY, CENTER_NAME);
		deliverreport("【银行】本站公务员领取工资", buf);
		mcLog("发放工资", 0, "");
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
	    { "大富翁世界总管", "银行行长", "博彩公司经理", "赌场经理",
		"黑帮帮主", "丐帮帮主", "证监会主席",
		"商场经理", "警署署长"
	};
	char ps[][STRLEN] =
	    { "谨望其能廉洁奉公，不以权谋私利，为本站金融事业的发展鞠躬尽瘁。",
		"大富翁世界对其一直以来的工作表示感谢，祝以后顺利！"
	};
	if (type == 0) {
		strcpy(head, "任命");
		strcpy(in, "为");
		strcpy(end, "");
	} else {
		strcpy(head, "免去");
		strcpy(in, "的");
		strcpy(end, "职务");
	}
	move(20, 4);
	snprintf(title, STRLEN - 1, "【总管】%s%s%s%s%s", head, boss, in,
		 posDesc[pos], end);
	sprintf(genbuf, "确定要 %s 吗", (title + 8));	//截去 [公告] 
	if (askyn(genbuf, YEA, NA) == NA)
		return 0;
	sprintf(genbuf, "%s %s", posStr, boss);
	if (type == 0) {
		addtofile(MC_BOSS_FILE, genbuf);
		sprintf(letter, "    %s\n", ps[0]);
	} else {
		getdata(21, 4, "原因：", buf, 40, DOECHO, YEA);
		sprintf(letter, "    原因：%s\n\n%s\n", buf, ps[1]);
		del_from_file(MC_BOSS_FILE, posStr);
	}
	deliverreport(title, letter);
	system_mail_buf(letter, strlen(letter), boss, title,
			currentuser->userid);
	sprintf(genbuf, "任命 %s 为 %s", boss, posDesc[pos]);
	mcLog(genbuf, 0, "");
	showAt(22, 4, "手续完成", YEA);
	return 1;
}

// ------------------------   银行   ----------------------- // 
static int
setBankRate(int rateType)
{
	char buf[STRLEN], strRate[10];
	char rateDesc[][10] = { "存款利", "贷款利", "转帐费" };
	unsigned char rate;

	sprintf(buf, "设定新的%s率[10-250]: ", rateDesc[rateType]);
	getdata(12, 4, buf, strRate, 4, DOECHO, YEA);
	rate = atoi(strRate);
	if (rate < 10 || rate > 250) {
		showAt(13, 4, "超出浮动范围!", YEA);
		return 1;
	}
	move(13, 4);
	sprintf(buf, "新的%s率是%.2f％，确定吗", rateDesc[rateType],
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
	sprintf(genbuf, "    新的%s率为 %.2f％ 。\n", rateDesc[rateType],
		rate / 100.0);
	sprintf(buf, "【银行】%s银行调整%s率", CENTER_NAME, rateDesc[rateType]);
	deliverreport(buf, genbuf);
	sprintf(buf, "调整%s率为", rateDesc[rateType]);
	mcLog(buf, (int)rate, "/ 10000");
	showAt(14, 4, "设置完毕", YEA);
	return 0;
}

static int
bank_saving()
{
	char ch, quit = 0, buf[STRLEN], getInterest;
	float rate = mcEnv->depositRate / 10000.0;
	int num, total_num, cal_Interest;

	money_show_stat("银行储蓄窗口");
	sprintf(buf, "存款利率（日）为 %.2f％", rate * 100);
	showAt(4, 4, buf, NA);
	move(t_lines - 1, 0);
	prints("\033[1;44m 选单 \033[1;46m [1]存款 [2]取款 ");
	if(seek_in_file(ROBUNION, currentuser->userid))
		prints("[3] 黑帮保险箱 ");
	if(seek_in_file(BEGGAR, currentuser->userid))
		prints("[3] 丐帮小金库 ");
	if(seek_in_file(POLICE, currentuser->userid))
		prints("[3] 警察私房钱 ");
	prints("[Q]离开\033[m");
	ch = igetkey();
	switch (ch) {
	case '1':
		if (check_health(1, 12, 4, "您的体力不够了！", YEA))
			break;
		num =
		    userInputValue(6, 4, "存", MONEY_NAME, 1000, MAX_MONEY_NUM);
		if (num == -1)
			break;
		if (myInfo->cash < num) {
			showAt(8, 4, "您没有这么多钱可以存。", YEA);
			break;
		}
		myInfo->cash -= num;
/* 加上原先存款的利息 */
		cal_Interest = makeInterest(myInfo->credit, myInfo->depositTime, rate);
		myInfo->interest += cal_Interest;
		mcEnv->Treasury -= cal_Interest;
/* 新的存款开始时间 */
		myInfo->depositTime = time(NULL);
		myInfo->credit += num;
		update_health();
		myInfo->health--;
		myInfo->Actived++;
		move(8, 4);
		prints("交易成功，您现在存有 %d %s，利息共计 %d %s。",
		       myInfo->credit, MONEY_NAME, myInfo->interest,
		       MONEY_NAME);
		pressanykey();
		mcLog("存款", num, "");
		break;
	case '2':
		if (check_health(1, 12, 4, "您的体力不够了！", YEA))
			break;
		num =
		    userInputValue(6, 4, "取", MONEY_NAME, 1000, MAX_MONEY_NUM);
		if (num == -1)
			break;
		if (num > myInfo->credit) {
			showAt(8, 4, "您没有那么多存款。", YEA);
			break;
		}
		
		cal_Interest = makeInterest(num, myInfo->depositTime, rate);
		myInfo->interest += cal_Interest;
		mcEnv->Treasury -= cal_Interest;
		move(8, 4);
		sprintf(genbuf, "是否取出 %d %s的存款利息", 
				myInfo->interest, MONEY_NAME);
		if (askyn(genbuf, NA, NA) == YEA) {
/* 存款加利息 */
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
		prints("交易成功，您现在存有 %d %s，存款利息共计 %d %s。",
		       myInfo->credit, MONEY_NAME, myInfo->interest,
		       MONEY_NAME);
		pressanykey();
		mcLog("取款", total_num, "");
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

	money_show_stat("银行贷款窗口");
	sprintf(buf, "贷款利率（日）为 %.2f％", rate * 100);
	showAt(4, 4, buf, NA);
	move(5, 4);
	hour = (myInfo->backTime - currTime) / 3600 + 1;
	total_num =
	    myInfo->loan + makeInterest(myInfo->loan, myInfo->loanTime, rate);
	if (myInfo->loan > 0) {
		prints("您贷款 %d %s，当前本息共计 %d %s，距到期 %d 小时。",
		       myInfo->loan, MONEY_NAME, total_num, MONEY_NAME, hour);
	} else
		prints("您目前没有贷款。");
	move(t_lines - 1, 0);
	prints("\033[1;44m 选单 \033[1;46m [1]贷款 [2]还贷 [Q]离开\033[m");
	ch = igetkey();
	switch (ch) {
	case '1':
		if (check_health(1, 12, 4, "您的体力不够了！", YEA))
			break;
		maxLoanMoney = MIN_XY(countexp(currentuser) * 500, mcEnv->Treasury);
		move(6, 4);
		if (maxLoanMoney < 1000) {
			showAt(8, 4, "对不起，您还没有贷款的资格。", YEA);
			break;
		}
		prints("按照银行的规定，您目前最多可以申请贷款 %d %s。",
		       maxLoanMoney, MONEY_NAME);
		num =
		    userInputValue(7, 4, "贷", MONEY_NAME, 1000, maxLoanMoney);
		if (num == -1)
			break;
		if (myInfo->loan > 0) {
			showAt(8, 4, "请先还清贷款。", YEA);
			break;
		}
		if (num > maxLoanMoney) {
			showAt(8, 4, "对不起，您要求贷款的金额超过银行规定。",
			       YEA);
			break;
		}
		while (1) {
			getdata(8, 4, "您要贷款多少天？[3-30]: ", buf,
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
		showAt(9, 4, "您的贷款手续已经完成。请到期还款。", YEA);
		mcLog("贷款", num, "");
		break;
	case '2':
		if (check_health(1, 12, 4, "您的体力不够了！", YEA))
			break;
		if (myInfo->loan == 0) {
			showAt(6, 4, "您记错了吧？没有找到您的贷款记录啊。",
			       YEA);
			break;
		}
		if (time(NULL) < myInfo->loanTime + 86400*3) {
			move(6, 4);
			if (askyn("您要提前偿还贷款吗？(会收取1%手续费)", NA, NA) == NA)
				break;
			total_num *= 1.01;
			move(5, 4);
			prints("您贷款 %d %s，当前本息共计 %d %s，距到期 %d 小时。",
				myInfo->loan, MONEY_NAME, total_num, MONEY_NAME, hour);
		} else {
			move(6, 4);
			if (askyn("您要现在偿还贷款吗？", NA, NA) == NA)
				break;
		}
		if (myInfo->cash < total_num) {
			showAt(7, 4, "对不起，您的钱不够偿还贷款。", YEA);
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
		showAt(7, 4, "您的贷款已经还清。银行乐见并铭记您的诚信。", YEA);
		mcLog("还贷", total_num, "");
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
	money_show_stat("银行商业贷款洽谈室");
	showAt(6, 4,
	       "\033[1;33m商业贷款是为了缓解暂时性资金周转不灵而设置的。\n    其相对于普通贷款的特点是金额大，利息高，欠贷惩罚重。\n\n    商业贷款的客户是赌场，商店等经营性业务承包人，黑帮丐帮等社团负责人。\n\n\033[1;32m    本行正在详细筹划中。。。\033[m",
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

	money_show_stat("银行转账窗口");
	if (check_health(1, 12, 4, "您的体力不够了！", YEA))
		return;
	move(4, 4);
	sprintf(genbuf,
		"最小转账金额 1000 %s。手续费 %.2f％ ",
		MONEY_NAME, transferRate * 100);
	prints("%s", genbuf);
	if (!getOkUser("转账给谁？", uident, 5, 4)) {
		showAt(12, 4, "查无此人", YEA);
		return;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(12, 4, "嗨，哥们～玩啥呢？”", YEA);
		return;
	}
	num = userInputValue(6, 4, "转账", MONEY_NAME, 1000, MAX_MONEY_NUM);
	if (num == -1)
		return;
	total_num = num + num * transferRate;
	prints("\n    请选择：从[1]现金 [2]存款 转帐？");
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
			prints("您的现金不够，加手续费此次交易共需 %d %s",
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
			prints("您的存款不够，加手续费此次交易共需 %d %s",
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
	sprintf(title, "您的朋友 %s 给您送钱来了", currentuser->userid);
	sprintf(buf, "%s 通过%s银行给您转帐了 %d %s，请查收。",
		currentuser->userid, CENTER_NAME, num, MONEY_NAME);
	system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
	if (num >= 1000000) {
		sprintf(title, "【银行】%s大笔资金转移", currentuser->userid);
		sprintf(buf, "    %s 通过%s银行转帐了 %d %s 给%s。\n",
			currentuser->userid, CENTER_NAME, num, MONEY_NAME,
			uident);
		deliverreport(title, buf);
	}
	sprintf(buf, "从%s转帐%d%s成功，我们已经通知了您的朋友。",
		(ch == '1') ? "现金" : "存款", num, MONEY_NAME);
	showAt(12, 4, buf, YEA);
	mcLog("转帐", num, uident);
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
		money_show_stat("黑帮保险箱");
		prints("    存放在保险箱里面的钱不会被偷被抢\n");
		break;
	case 1:
		money_show_stat("丐帮小金库");
		prints("    藏在小金库里面的钱别人偷不到\n");
		break;
	case 2:
		money_show_stat("警察私房钱");
		prints("    警察也需要私房钱改善生活\n");
		break;
	}
	prints("    每人每天只能存50万，并且收取10％的手续费\n"
	       "    你的保险箱里有 %10d 万 %s\n"
	       "    你最多能存     %10d 万 %s",
	       myInfo->safemoney/10000, MONEY_NAME, max_money/10000, MONEY_NAME);
	move(t_lines-1, 0);
	prints("\033[1;44m 选项 \033[1;46m [1] 存钱 [2] 取钱 [Q] 离开\033[m");
	ch = igetkey();
	switch (ch) {
	case '1':
		update_health();
		if (check_health(1, 10, 4, "你没有足够的力气了！", YEA))
			break;
		localtime_r(&myInfo->secureTime, &Tsecure);
		currTime = time(NULL);
		localtime_r(&currTime, &Tnow);
		if (Tsecure.tm_yday == Tnow.tm_yday &&
		    Tsecure.tm_year == Tnow.tm_year) {
			showAt(10, 4, "你今天已经存过50万了！", YEA);
			break;
		}
		if (myInfo->safemoney + 500000 > max_money) {
			showAt(10, 4, "按照你现在的地位只能藏这么些钱了。", YEA);
			break;
		}
		move(10, 4);
		if (askyn("你确实要存50万吗？", NA, NA) == NA)
			break;
		if (myInfo->cash < 550000) {
			showAt(12, 4, "你没有带够现金！", YEA);
			break;
		}
		myInfo->cash -= 550000;
		myInfo->safemoney += 500000;
		mcEnv->Treasury += 50000;
		myInfo->Actived++;
		myInfo->health--;
		myInfo->secureTime = time(NULL);
		showAt(12, 4, "交易成功！\n", YEA);
		mcLog("存入", 50, "万 安全钱");
		break;
	case '2':
		update_health();
		if (check_health(1, 10, 4, "你没有足够的力气了！", YEA))
			break;
		inputNum = userInputValue(10, 4, "你要取", "万" MONEY_NAME "？",
				50, myInfo->safemoney / 500000 * 50);
		if (inputNum < 0) {
			showAt(12, 4, "你不打算取钱了。", YEA);
			break;
		}
		if (inputNum * 10000 > myInfo->safemoney) {
			showAt(12, 4, "你没有这么多钱！", YEA);
			break;
		}
		if (inputNum % 50 != 0) {
			showAt(12, 4, "只能取50万的整倍数。", YEA);
			break;
		}
		myInfo->cash += inputNum * 10000;
		myInfo->safemoney -= inputNum * 10000;
		myInfo->Actived++;
		myInfo->health--;
		showAt(14, 4, "交易成功！\n", YEA);
		mcLog("取出", inputNum, "万 安全钱");
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
		mcLog("被收了", income - after, "的税");
	return after;
}

static int
money_bank()
{
	int ch, quit = 0,insurance;
	char uident[IDLEN + 1], buf[256];

	while (!quit) {
		sprintf(buf, "%s银行", CENTER_NAME);
		money_show_stat(buf);
		move(8, 16);
		prints("%s银行欢迎您的光临！", CENTER_NAME);
		move(t_lines - 1, 0);
		prints("\033[1;44m 选单 \033[1;46m [1]转账 [2]储蓄"
		       " [3]贷款 [4]工资 [5]保险 [6]行长办公室 [Q]离开\033[m");
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
			money_show_stat("保险公司");
			move(6, 4);
			if (askyn("你要买保险吗？", NA, NA) == NA)
				break;
			if (myInfo->cash < 10000) {
				showAt(7, 4, "你没有带够现金。", YEA);
				break;
			}
			insurance = userInputValue(7, 4, "你要投保", "分钟", 1, myInfo->cash / 10000);
			if (insurance < 1) {
				showAt(9, 4, "你不打算投保了。", YEA);
				break;
			}
			myInfo->cash -= insurance * 10000;
			mcEnv->Treasury += insurance * 10000;
			myInfo->insuranceTime = time(NULL) + insurance * 60;
			move(10, 4);
			prints("你投保了%d小时%d分钟", insurance / 60, insurance % 60);
			mcLog("投保", insurance, "分钟");
			pressanykey();
			break;
		case '6':
			money_show_stat("行长办公室");
			move(6, 4);
			whoTakeCharge(1, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			prints("请选择操作代号:");
			move(7, 0);
			sprintf(genbuf, "      1. 调整存款利率。目前利率: %.2f％\n"
					"      2. 调整贷款利率。目前利率: %.2f％\n"
					"      3. 调整转帐费率。目前利率: %.2f％\n"
					"      Q. 退出", mcEnv->depositRate / 100.0, 
					mcEnv->loanRate / 100.0, mcEnv->transferRate / 100.0);
			prints(genbuf);
			ch = igetkey();
			switch (ch) {
			case '1':
			case '2':
			case '3':
				update_health();
				if (check_health
				    (1, 12, 4, "您工作太辛苦了，休息一下吧！",
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
	money_show_stat("银行工资代办窗口");
	if (check_health(1, 12, 4, "您的体力不够了！", YEA))
		return;;
	salary = makeSalary();
	if (salary == 0) {
		showAt(10, 10, "您不是本站公务员，没有工资。",
				YEA);
		return;
	}
	if (mcEnv->salaryStart == 0
			|| time(NULL) > mcEnv->salaryEnd) {
		showAt(10, 10, "对不起，银行还没有收到工资划款。", YEA);
		return;
	}
	if (myInfo->lastSalary >= mcEnv->salaryStart) {
		sprintf(genbuf, "您已经领过工资啦。还是勤奋工作吧！\n"
				"          下次发工资日期：%16s",
				ctime(&mcEnv->salaryEnd));
		showAt(10, 10, genbuf, YEA);
		return;
	}
	//if (mcEnv->Treasury < salary){
	//      showAt(10, 10, "对不起，现在银行没有现金，请您稍后再来。", YEA);
	//      return;
	//      }

	move(6, 4);
	sprintf(genbuf, "您本月的工资 %d %s已经划到银行。现在领取吗？",
			salary, MONEY_NAME);
	if (askyn(genbuf, NA, NA) == NA)
		return;
	myInfo->lastSalary = mcEnv->salaryEnd;
	myInfo->cash += salary;
	mcEnv->Treasury -= salary;
	update_health();
	myInfo->health--;
	myInfo->Actived++;
	showAt(8, 4, "这里是您的工资。感谢您所付出的工作!",
			YEA);
	mcLog("领取", salary, "的工资");
	return;
}

// ------------------------  彩票  -------------------------- // 
//part one: 36_7 
static int
valid367Bet(char *buf)
{
	int i, j, temp[7], slot = 0;

	if (strlen(buf) != 20)	//  长度必须为20= 2 * 7 + 6 
		return 0;
	for (i = 0; i < 20; i += 3) {	//  基本格式必须正确 
		if (i + 2 != 20)
			if (buf[i + 2] != '-')	// 分隔符不正确 
				return 0;
		if (!isdigit(buf[i]) || !isdigit(buf[i + 1]))	// 不是数字 
			return 0;
		temp[slot] = (buf[i] - '0') * 10 + (buf[i + 1] - '0');
		if (temp[slot] > 36)
			return 0;
		slot++;
	}
	for (i = 0; i < 7; i++) {	// 数字无重复 
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
		do {		//  数字不能相同 
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
	sprintf(genbuf, "    数字组合是：  %s  。您中奖了吗？\n", prizeSeq);
	deliverreport("【彩票】本期36选7彩票摇奖结果", genbuf);
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
			if (meetSeperator == 1)	//如果连续遇到-，肯定不正确 
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
				if (buf[i] == first)	//重合 
					return 0;
				second = buf[i];
			} else if (count == 3) {
				if (buf[i] == first || buf[i] == second)	//重合 
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
{				//计算复式注的数量 
	int i, countNum = 0, total = 1;
	int len = strlen(complexBet);

	for (i = 0; i < len; i++) {
		if (complexBet[i] == '-') {
			total *= countNum;
			countNum = 0;
		} else
			countNum++;
	}
	total *= countNum;	// 再乘上最后一个单元 
	return total;
}

static int
makeSoccerPrize(char *bet, char *prizeSeq)
{
	int i, diff = 0;
	int n1 = strlen(bet);
	int n2 = strlen(prizeSeq);

	if (n1 != n2)
		return 10;	// 不中奖 
	for (i = 0; i < n1; i++) {
		if (bet[i] != prizeSeq[i])
			diff++;
	}
	return diff;
}

static void
parseSoccerBet(char *prizeSeq, char *complexBet, struct LotteryRecord *LR)
{				// 解析一个复式买注,保存奖励情况至LR 
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
	if (simple) {		//简单标准形式 
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
		for (i = 0; i < len; i++) {	//寻找第一个复式单元 
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

		for (i = 0; i < count; i++) {	//对每一个要拆分的单元的元素 
			int slot = 0;
			char temp[STRLEN];

//得到前面的部分 
			if (firstDivStart != 0) {
				for (j = 0; j < firstDivStart; j++, slot++)
					temp[slot] = complexBet[j];
			}
			temp[slot] = complexBet[firstDivStart + i];
			slot++;
//得到后面的部分 
			for (j = firstDivEnd + 1; j < len; j++, slot++) {
				temp[slot] = complexBet[j];
			}
			temp[slot] = '\0';
//对每一个拆分，进行递归调用 
			parseSoccerBet(prizeSeq, temp, LR);
		}

	}
}

//part three: misc 
static int
createLottery(int prizeMode)
{
	char buf[STRLEN];
	char lotteryDesc[][16] = { "-", "36选7彩票", "足彩" };
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
	if (check_health(1, 12, 4, "您的体力不够了！", YEA))
		return 1;
	move(12, 4);
	if (currTime < *endTime) {
		prints("%s销售正在火热进行。", lotteryDesc[prizeMode]);
		pressanykey();
		return 1;
	}
	prints("新建%s", lotteryDesc[prizeMode]);
	while (1) {
		getdata(14, 4, "彩票销售天数[1-7]: ", buf, 2, DOECHO, YEA);
		day = atoi(buf);
		if (day >= 1 && day <= 7)
			break;
	}
	*startTime = currTime;
	*endTime = currTime + day * 86400;
	update_health();
	myInfo->health--;
	myInfo->Actived++;
	sprintf(genbuf, "    本期彩票将于 %d 天后开奖。欢迎大家踊跃购买！\n", day);
	sprintf(buf, "【彩票】新一期%s开始销售", lotteryDesc[prizeMode]);
	deliverreport(buf, genbuf);
	showAt(15, 4, "建立成功！请到时开奖。", YEA);
	mcLog("新建", 0, lotteryDesc[prizeMode]);
	return 0;
}

static void
savePrizeList(int prizeMode, struct LotteryRecord LR,
	      struct LotteryRecord *totalCount)
{				//将中奖情况按userid保存至临时文件 

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
		if (!strcmp(LR_curr->userid, LR.userid)) {	// 如果userid已经存在 
			for (i = 0; i < 5; i++)
				LR_curr->betCount[i] += LR.betCount[i];
			miss = 0;
			break;
		}
	}
	if (miss)		//userid记录不存在, add 
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
// 全局统计累加 
	for (i = 0; i < 5; i++)
		totalCount->betCount[i] += LR.betCount[i];
	return;
}

static int
sendPrizeMail(struct LotteryRecord *LR, struct LotteryRecord *totalCount)
{
	int i, totalMoney, perPrize, myPrizeMoney;
	char title[STRLEN];
	char *prizeName[] = { "NULL", "36选7", "足彩", NULL };
	char *prizeClass[] = { "特等", "一等", "二等", "三等", "安慰", NULL };
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
	for (i = 0; i < 5; i++) {	// 对每个中奖的userid,检查各种奖励 
		if (LR->betCount[i] > 0 && totalCount->betCount[i] > 0) {
			perPrize =
			    prizeRate[i] * totalMoney / totalCount->betCount[i];
			myPrizeMoney = perPrize * LR->betCount[i];
			mcuInfo->cash += myPrizeMoney;
			mcEnv->Treasury -= myPrizeMoney;
			sprintf(genbuf,
				"您一共中了 %d 注,得到了 %d %s的奖金。恭喜！",
				LR->betCount[i], myPrizeMoney, MONEY_NAME);
			sprintf(title, "恭喜您获得%s%s奖！",
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
	char *prizeName[] = { "NULL", "36选7", "足彩", NULL };
	char *prizeClass[] = { "特等", "一等", "二等", "三等", "安慰", NULL };
	float prizeRate[] = { 0.60, 0.20, 0.10, 0.05, 0.02 };

	if (prizeMode == 1) {
		make367Seq(prizeSeq);	//产生序列 
		totalMoney = mcEnv->prize367 + PRIZE_PER;
		fp = fopen(DIR_MC "36_7_list", "r");
	} else {
		totalMoney = mcEnv->prizeSoccer + PRIZE_PER;
		fp = fopen(DIR_MC "soccer_list", "r");
	}
	totalMoney = MIN_XY(totalMoney, MAX_POOL_MONEY);
	if (fp == NULL)
		return -1;
//   ---------------------计算奖励----------------------- 

	memset(&totalCount, 0, sizeof (struct LotteryRecord));	// 总计初始化 
	while (fgets(line, 255, fp)) {
		userid = strtok(line, " ");
		bet = strtok(NULL, "\n");
		if (!userid || !bet) {
			continue;
		}
		memset(&LR, 0, sizeof (struct LotteryRecord));
		strcpy(LR.userid, userid);
// 解析买注,中奖情况保存在LR中 
		if (prizeMode == 1)
			parse367Bet(prizeSeq, bet, &LR);
		else
			parseSoccerBet(prizeSeq, bet, &LR);
		for (i = 0; i < 5; i++) {	// 检查是否中奖 
			if (LR.betCount[i] > 0) {
				savePrizeList(prizeMode, LR, &totalCount);
				break;
			}
		}
	}
	fclose(fp);
//  ------------------------ 发奖 --------------------- 
	remainMoney = totalMoney;
	if (prizeMode == 1) {
		sprintf(genbuf, "%s", DIR_MC_TEMP "36_7_prizeList");
		strcpy(totalCount.userid, "36_7");
	} else {
		sprintf(genbuf, "%s", DIR_MC_TEMP "soccer_prizeList");
		strcpy(totalCount.userid, "soccer_");

// 遍历前面生成的中奖文件,给每个中奖ID发奖. 每个ID是一个记录 
// 同时总计累加 
		new_apply_record(genbuf, sizeof (struct LotteryRecord),
				 (void *) sendPrizeMail, &totalCount);

//  ---------------------- 版面通知 --------------------- 
		for (i = 0; i < 5; i++) {
			if (totalCount.betCount[i] > 0) {
				sprintf(title, "【彩票】本期%s%s奖情况揭晓",
					prizeName[prizeMode], prizeClass[i]);
				sprintf(buf, "    共计注数: %d\n单注奖金: %d\n",
					totalCount.betCount[i],
					(int) (prizeRate[i] * totalMoney /
					       totalCount.betCount[i]));
				deliverreport(title, buf);
				remainMoney -= totalMoney * prizeRate[i];
			}
		}
	}
// 清扫战场 
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
	if (check_health(1, 12, 4, "您的体力不够了！", YEA))
		return 1;
	if (prizeMode == 1) {
		startTime = mcEnv->start367;
		endTime = mcEnv->end367;
	} else {
		startTime = mcEnv->soccerStart;
		endTime = mcEnv->soccerEnd;
	}
	if (startTime == 0) {
		showAt(t_lines - 5, 4, "没有找到该彩票的记录...", YEA);
		return -1;
	}
	if (time(NULL) < endTime) {
		showAt(t_lines - 5, 4, "还没有到开奖的时间啊!", YEA);
		return -1;
	}
	if (prizeMode == 1)
		buf[0] = '\0';
	else {
		getdata(t_lines - 5, 4,
			"请输入兑奖序列(无需 - )[按\033[1;33mENTER\033[m放弃]: ",
			buf, 55, DOECHO, YEA);
		if (buf[0] == '\0')
			return 0;
	}
	flag = doOpenLottery(prizeMode, buf);
	move(t_lines - 4, 4);
	if (flag == 0)
		prints("开奖成功！");
	else
		prints("发生意外错误...");
	update_health();
	myInfo->health--;
	myInfo->Actived++;
	pressanykey();
	mcLog("开奖", flag, (prizeMode == 1)? "36选7" : "足彩" );
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
	char *desc[] = { "36选7", "足球" };

	money_show_stat("彩票窗口");
	if (check_health(1, 12, 4, "您的体力不够了！", YEA))
		return 0;
	if (type == 0) {	//36选7 
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
		sprintf(buf, "抱歉，新一期的%s彩票还未开始销售。", desc[type]);
		showAt(4, 4, buf, YEA);
		return 0;
	}
	if (time(NULL) >= endTime) {
		showAt(4, 4, "抱歉，本期彩票销售期已经结束。请等待开奖。", YEA);
		return 0;
	}
	if (type == 0)
		showAt(4, 4, "下注方法：从01～36中选择7个数字(不能重复)，数字间用-隔开，例如：\n"
			"              08-13-01-25-34-17-18 01-12-23-34-32-21-10\n"
			"    彩票到期后开奖，猜中3个或3个以上数字才中奖。", NA);
	else
		showAt(4, 0,
		       "    主场胜/平/负分别为为3/1/0。各场比赛用-隔开。\n"
		       "    支持复式买注。例如： 1-310-1-01-3-30", NA);
	move(7, 4);
	prints("当前奖金池：\033[1;31m%d\033[m   固定奖金：\033[1;31m%d\033[m",
	       *poolMoney, PRIZE_PER);
	sprintf(genbuf, "每注 %d %s。确定买注吗", perMoney, MONEY_NAME);
	move(9, 4);
	if (askyn(genbuf, NA, NA) == NA)
		return 0;
	while (1) {
		if (type == 0) {
			if(getdata(10, 4, "请填写买注单(直接按回车将由系统自动生成): ",
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
			getdata(10, 4, "请填写买注单: ", buf, maxBufLen, DOECHO, YEA);
		showAt(11, 4, buf, NA);
		if (askyn("\n    你决定买这一注吗？", YEA, NA) == NA) {
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
			showAt(13, 4, "对不起，您的下注单填写得不对喔。", YEA);
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
			showAt(13, 4, "对不起，您的钱不够。", YEA);
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
		sprintf(letter, "您购买了一注%s彩票。注号是：%s。", desc[type],
			buf);
		system_mail_buf(letter, strlen(letter), currentuser->userid,
				"彩票中心购买凭证", currentuser->userid);
		clrtoeol();
		sprintf(buf, "成功购买 %d 注%s彩票 。祝您中大奖！", num,
			desc[type]);
		sleep(1);
		showAt(13, 4, buf, YEA);
		mcLog("买了", num, desc[type]);
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
		nomoney_show_stat("彩票中心");
		showAt(5, 4, "目前发行两种彩票: 36选7 和 足球彩票。欢迎购买！",
		       NA);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]36选7 [2]足彩 [3]经理室 [Q]离开\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
		case '2':
			buyLottery(ch - '1');
			break;
		case '3':
			nomoney_show_stat("博彩公司经理室");
			whoTakeCharge(2, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			quitRoom = 0;
			while (!quitRoom) {
				nomoney_show_stat("博彩公司经理室");
				move(5, 0);
				prints("    建立彩票:    1.  36选7    %s",
				       file_isfile(DIR_MC "36_7_list")?"已建立\n":"\n");
				prints("                 2.  足球彩票 %s",
				       file_isfile(DIR_MC "soccer_list")?"已建立\n":"\n");
				prints("    开    奖:    3.  36选7    到期时间：%s",
					ctime(&mcEnv->end367));
				prints("                 4.  足球彩票 到期时间：%s",
					ctime(&mcEnv->soccerEnd));
				prints("                 Q.  退出");
				move(10, 4);
				prints("请选择要操作的代号:");
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

// --------------------------    赌场    ----------------------- // 
static int
money_dice()
{
	int i, ch, quit = 0, target, win, num, sum;
	unsigned int t[3];

	while (!quit) {
		money_show_stat("赌场骰宝厅");
		move(4, 0);
		prints("    分大小两门，4-10点是小，11-17点为大。\n"
		       "    若押小开小，可拿一倍彩金，押大的就全归庄家。\n"
		       "    庄家要是摇出全骰（三个骰子点数一样）则通吃大小家。\n"
		       "    \033[1;31m多买多赚，少买少赔，买定离手，愿赌服输!\033[m");
		move(t_lines - 1, 0);
		prints("\033[1;44m 选单 \033[1;46m [1]下注 [2]VIP [Q]离开\033[m");
		win = 0;
		ch = igetkey();
		switch (ch) {
		case '1':
			update_health();
			if (check_health(1, 12, 4, "您的体力不够了！", YEA))
				break;
			num =
			    userInputValue(9, 4, "压", MONEY_NAME, 1000,
					   200000);
			if (num == -1)
				break;
			getdata(11, 4, "压大(L)还是小(S)？[L]", genbuf, 3,
				DOECHO, YEA);
			if (genbuf[0] == 'S' || genbuf[0] == 's')
				target = 1;
			else
				target = 0;
			sprintf(genbuf,
				"买 \033[1;31m%d\033[m %s的 \033[1;31m%s\033[m，确定么？",
				num, MONEY_NAME, target ? "小" : "大");
			move(12, 4);
			if (askyn(genbuf, NA, NA) == NA)
				break;
			move(13, 4);
			if (myInfo->cash < num) {
				showAt(13, 4, "去去去，没那么多钱捣什么乱！",
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
				sprintf(genbuf, "\033[1;32m庄家通杀！\033[m");
				mcLog("玩骰子遇到通杀，输了", num, "");
			} else if (sum <= 10) {
				sprintf(genbuf, "%d 点，\033[1;32m小\033[m",
					sum);
				if (target == 1)
					win = 1;
			} else {
				sprintf(genbuf, "%d 点，\033[1;32m大\033[m",
					sum);
				if (target == 0)
					win = 1;
			}
			sleep(1);
			prints("开了开了~~  %d %d %d  %s", t[0], t[1], t[2],
			       genbuf);
			move(14, 4);
			if (win) {
				myInfo->cash += 2 * num;
				mcEnv->prize777 -= num;
				prints("恭喜您，再来一把吧！");
				mcLog("玩骰子赢了", num, "");
			} else {
				mcEnv->prize777 += after_tax(num);
				prints("没有关系，先输后赢...");
				mcLog("玩骰子输了", num, "");
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
		money_show_stat("赌场777");
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
		if (check_health(1, 12, 4, "您的体力不够了！", YEA))
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
		prints("777 1:80 (有机会赢得当前累计基金的一半，最多不超过100万)");
		move(9, 4);
		prints
		    ("目前累积奖金数: %d  想赢大奖么？压200就有机会喔。",
		     mcEnv->prize777);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m 选单 \033[1;46m [1] 压50 [2] 压200 [3] 注入奖金 [Q]离开\033[m");
		ch = igetkey();
		if (ch == 'q' || ch == 'Q')
			break;
		if (ch == '1') {
			if (mcEnv->prize777 < 10000) {
				showAt(11, 4,
				       "目前777奖金殆尽，请等待注入奖金。",
				       YEA);
				return 0;
			}
			bid = 50;
		} else if (ch == '2') {
			if (mcEnv->prize777 < 10000) {
				showAt(11, 4,
				       "目前777奖金殆尽，请等待注入奖金。",
				       YEA);
				return 0;
			}
			bid = 200;
		} else if (ch == '3') {
			if (myInfo->cash < 10000) {
				showAt(11, 4, "你带的现金太少了，cmft", YEA);
				continue;
			}
			num =
			    userInputValue(11, 4, "注入", MONEY_NAME, 10000,
					   myInfo->cash);
			if (num <= 0)
				continue;
			myInfo->cash -= num;
			mcEnv->prize777 += after_tax(num);
			myInfo->luck = MIN_XY(100, myInfo->luck + num / 10000);
			showAt(15, 4, "注入奖金成功！", YEA);
			mcLog("在777买人品，花了", num, "现金");
			continue;
		} else
			continue;
		if (myInfo->cash < bid) {
			showAt(11, 4, "没钱就别玩了...", YEA);
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
			       "输了，赌注流入累积基金，造福他人等于造福自己。",
			       YEA);
			continue;
		}
		mcEnv->prize777 -= bid * (winrate - 1);
		myInfo->cash += bid * winrate;
		move(12, 4);
		prints("您赢了 %d %s", bid * (winrate - 1), MONEY_NAME);
		if (winrate == 81 && bid == 200) {
			num = MIN_XY(mcEnv->prize777 / 2, 1000000);
//			num = MAX_XY(num, random() % (myInfo->luck + 101) * 5000 );
			tax = num - after_tax(num);
			myInfo->cash += num - tax;
			mcEnv->prize777 -= num;
			move(12, 4);
			prints("\033[1;5;33m恭喜您获得大奖！别忘了要缴税啊！\033[0m");
			sprintf(title, "【赌场】%s 赢得777大奖！",
				currentuser->userid);
			sprintf(buf, "    赌场传来消息：%s 赢得777大奖 %d %s!\n"
					"    扣税后实得 %d %s\n",
				currentuser->userid, num, MONEY_NAME, 
				num - tax, MONEY_NAME);
			deliverreport(title, buf);
			mcLog("赢得", num, "777大奖");
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
		       "门卫斜眼瞅你一眼，伸手把你拦住：“一副穷酸相也想进大户密室？大你个头大！！”",
		       YEA);
		return 0;
	}
	move(7, 4);
	sprintf(buf, "入场费\033[1;32m10万\033[m%s，你确定要进去吗？",
		MONEY_NAME);
	if (askyn(buf, NA, NA) == NA) {
		prints("\n拜拜，小气鬼～");
		return 0;
	}
	myInfo->cash -= 100000;
	mcEnv->prize777 += after_tax(100000);
	clear();
	money_show_stat("赌场大户密室");
	move(4, 0);
	prints
	    ("    这里是大户密室，这里的赌注比外面的更要大，所以更加的刺激：）\n"
	     "    这地方一般有钱有权的才能进的来，赶紧押吧！\n"
	     "    哦，对了，赢了的话要抽取10％的小费的。");
	pressanykey();

	while (!quit) {
		clear();
		money_show_stat("赌场大户密室");
		move(4, 0);
		prints("    骰宝。简介：分大小两门，4-10点是小，11-17点为大。\n"
		       "    若押小开小，可拿一倍彩金，押大的就全归庄家。\n"
		       "    庄家要是摇出全骰（三个骰子点数一样）则通吃大小家。\n"
		       "    \033[1;31m多买多赚，少买少赔，买定离手，愿赌服输!\033[m");
		move(t_lines - 1, 0);
		prints("\033[1;44m 选单 \033[1;46m [1]下注 [Q]离开\033[m");
		win = 0;
		ch = igetkey();
		switch (ch) {
		case '1':
			update_health();
			if (check_health(3, 12, 4, "您的体力不够了！", YEA))
				break;
			sprintf(buf, "\033[1;32m万\033[m%s", MONEY_NAME);
			num = userInputValue(9, 4, "压", buf, 10, 1000);
			if (num == -1)
				break;
			getdata(11, 4, "压大(L)还是小(S)？[L]", genbuf, 3,
				DOECHO, YEA);
			if (genbuf[0] == 'S' || genbuf[0] == 's')
				target = 1;
			else
				target = 0;
			sprintf(genbuf,
				"买 \033[1;31m%d\033[m \033[1;32m万\033[m%s的 \033[1;31m%s\033[m，确定么？",
				num, MONEY_NAME, target ? "小" : "大");
			move(12, 4);
			if (askyn(genbuf, NA, NA) == NA)
				break;
			move(13, 4);
			if (myInfo->cash < (num * 10000)) {
				showAt(13, 4, "去去去，没那么多钱捣什么乱！",
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
				sprintf(genbuf, "\033[1;32m庄家通杀！\033[m");
				mcLog("在赌场大户室被通杀，输了", num, "万");
			} else if (sum <= 10) {
				sprintf(genbuf, "%d 点，\033[1;32m小\033[m",
					sum);
				if (target == 1)
					win = 1;
			} else {
				sprintf(genbuf, "%d 点，\033[1;32m大\033[m",
					sum);
				if (target == 0)
					win = 1;
			}
			sleep(1);
			prints("开了开了~~  %d %d %d  %s", t[0], t[1], t[2],
			       genbuf);
			move(14, 4);
			if (win) {
				myInfo->cash += 19000 * num;
				mcEnv->prize777 -= 10000 * num;
				mcEnv->Treasury += 1000 * num;
				prints("恭喜您，抽取小费%d%s~~ 再来一把吧！",
				       1000 * num, MONEY_NAME);
				mcLog("在赌场大户室赢了", num, "万");
			} else{
				mcEnv->prize777 += after_tax(10000 * num);
				prints("没有关系，先输后赢，至少不用给小费了...");
				mcLog("在赌场大户室输了", num, "万");
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
		money_show_stat("赌场大厅");
		move(6, 4);
		prints("%s赌场最近生意红火，大家尽兴啊！", CENTER_NAME);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]骰宝 [2]777 [3]MONKEY [4]游戏抽奖 [5]猜数字 [6]经理办公室[Q]离开\033[m");
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
			nomoney_show_stat("赌场经理办公室");
			showAt(12, 4, "\033[1;32m正在建设中。\033[m", YEA);
			break;
		case 'q':
		case 'Q':
			quit = 1;
			break;
		}
	}
	return 0;
}

//  ----------------------------    社团    --------------------------  // 
static int
forceGetMoney(int type)
{
	int money, cost_health;
	time_t currtime;
	char uident[IDLEN + 1], buf[256], place[STRLEN];
	char *actionDesc[] = { "勒索", "乞讨", "抢劫", "妙手空空", NULL };
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;

	if ((type == 2 && myInfo->robExp < 50)
	    || (type == 3 && myInfo->begExp < 50)) {
		move(10, 8);
		prints("你还没有足够的%s来做%s这么大的事情。",
		       type % 2 ? "身法" : "胆识", actionDesc[type]);
		pressanykey();
		return 0;
	}
	move(4, 4);
	switch (type) {
	case 0:
		strcpy(place, "富人区");
		break;
	case 1:
		strcpy(place, "中心地段");
		break;
	case 2:
		strcpy(place, "冷清小巷");
		break;
	case 3:
		strcpy(place, "商业街");
		break;
	}
	prints("这里是%s的%s，实在是%s的好地方。", CENTER_NAME, place,
	       actionDesc[type]);
	if (!getOkUser("你要向谁下手？", uident, 6, 4)) {
		move(7, 4);
		prints("查无此人");
		pressanykey();
		return 0;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(7, 4, "牛魔王：“老婆～快来看神经病啦～”", YEA);
		return 0;
	}
	move(7, 4);
	if ((type % 2 == 0 && !seek_in_file(ROBUNION, currentuser->userid))
	    || (type % 2 == 1 && !seek_in_file(BEGGAR, currentuser->userid))) {
		prints("怎么看你也不像是会%s的人啊！", actionDesc[type]);
		pressanykey();
		return 0;
	}
	if (!t_search(uident, NA, 1)) {
		if (type == 0)
			prints("你看错人了吧？刚过这个人不是%s啊！", uident);
		else if (type == 1)
			prints("%s不在家，你敲了半天门也没人应。", uident);
		else if (type == 2)
			prints("冷清小巷确实冷清啊，居然一个人都没有。。。。");
		else
			prints("十字路口人来人往，你找了半天没找到要找的%s。",
			       uident);
		pressanykey();
		return 0;
	}
	cost_health = 5 + (type % 2 * 10);
	if (check_health(cost_health, 12, 4, "你哪有那么多体力做事啊？", YEA))
		return 0;

	currtime = time(NULL);
	if (currtime < 1200 + myInfo->lastActiveTime) {
		if (type % 2 == 0)
			prints
			    ("你刚做完坏事，人家记忆犹新。%s一看见你就远远的躲开了，压根不往这边走。",
			     uident);
		else if (type == 1)
			prints
			    ("%s怒不可遏，骂道：“臭要饭的，烦死了，还不快滚！”",
			     uident);
		else		//妙手空空 
			prints
			    ("你正要下手，就听见边上有人喊：”啊，我的钱包不见了！“ \n    %s一听，马上把钱包给捂住了。”",
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
		prints("你手里晃着指甲刀, 对着%s嘿嘿奸笑道: “收取过路费！”\n",
		       uident);
		break;
	case 1:
		prints
		    ("你对着%s哭喊道：“买朵玫瑰花吧～我已经好几分钟没吃饭了，我好饿啊！”\n",
		     uident);
		break;
	case 2:
		prints
		    ("你手拿片刀对着%s恶狠狠的喊道：“IC~IP~IQ~~卡，统统告诉我密码！！”\n",
		     uident);
		break;
	case 3:
		prints
		    ("你不动声色的靠近%s，神不知鬼不觉的把手伸向他的衣兜。。。\n",
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
			prints("    %s一脚把你踹开。骂道：“臭叫化子一边去！”\n",
			       uident);
		} else {
			prints
			    ("    %s飞起一脚把你踢飞。哼道：“我最瞧不起勒索打劫的，一点技术含量都没有。”\n",
			     uident);
		}
		prints("\n    你脸一红，赶紧灰溜溜的跑开了");
		pressanykey();
		return 0;
	}

	money = MIN_XY(mcuInfo->cash / 10, 100000);
	move(8, 4);
	if (money == 0) {
		prints("%s身上没钱，碰上了穷鬼真倒霉...", uident);
		goto UNMAP;
	}
	if (type == 0) {
		if (random() % 3 && currtime > mcuInfo->insuranceTime) {
			mcuInfo->cash -= money;
			myInfo->cash += money;
			myInfo->robExp += 1;
			prints
			    ("%s吓得直打哆嗦, 赶紧从身上拿出 %d %s给你。\n\n\033[32m    你的胆识增加了！\033[m",
			     uident, money, MONEY_NAME);
			if (myInfo->luck > -100) {
				myInfo->luck -= 1;
				prints("\n\033[31m    你的人品降低了！");
			}
			sprintf(genbuf, "你被%s 勒索了 %d%s，太不幸了。",
				currentuser->userid, money, MONEY_NAME);
			sprintf(buf, "你遭到勒索");
			mcLog("勒索到", money, uident);
		} else {
			prints("啊，有警察过来了！风紧，扯乎～\n");
			pressanykey();
			return 0;
		}

	} else if (type == 1) {
		if (random() % 3 && currtime > mcuInfo->insuranceTime) {
			mcuInfo->cash -= money;
			myInfo->cash += money;
			myInfo->begExp += 1;
			prints
			    ("%s眼圈顿时红了，赶紧从身上拿出 %d %s从你手里接过一朵。\n\n\032[m    你的身法提高了！\033[m",
			     uident, money, MONEY_NAME);
			if (myInfo->luck > -100) {
				myInfo->luck -= 1;
				prints("\n\033[31m    你的人品降低了！");
			}
			sprintf(genbuf,
				"你一时好心，花了%d%s从%s那买了朵花 ，过后发现是狗尾巴草。。。",
				money, MONEY_NAME, currentuser->userid);
			sprintf(buf, "你遇到卖花小孩");
			mcLog("卖花赚了", money, uident);
		} else {
			prints("哇哇，城管来了，快跑啊～\n");
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
			    ("%s吓得把身上的%d%s全掏了出来，哭道：”您可千万别劫色啊～“\n\n\033[32m    你的胆识增加了！\033[m",
			     uident, money, MONEY_NAME);
			if (myInfo->luck > -100) {
				myInfo->luck = MAX_XY((myInfo->luck - 5), -100);
				prints("\n\033[31m    你的人品降低了！");
			}
			sprintf(genbuf,
				"你遇到劫匪，被 %s 抢走 %d%s，真是欲哭无泪啊。\n",
				currentuser->userid, money, MONEY_NAME);
			sprintf(buf, "你被抢劫");
			mcLog("抢劫到", money, uident);
		} else {
			money = MIN_XY(myInfo->cash / 2, 10000000);
			mcuInfo->cash += money;
			myInfo->cash -= money;
			myInfo->robExp = MAX_XY((myInfo->robExp - 10), 0);
			prints
			    ("%s不慌不忙的从兜里掏出一把手枪，顶住你脑门：还是你把钱交出来吧。\n\n    你遇到亡命之徒，损失了%d%s...\n\n\033[31m    你的胆识大减！\033[m",
			     uident, money, MONEY_NAME);
			sprintf(genbuf,
				"劫匪 %s 被你黑吃黑，抢到 %d%s，恭喜你～\n",
				currentuser->userid, money, MONEY_NAME);
			sprintf(buf, "你黑吃黑成功");
			mcLog("抢劫遇到黑吃黑，损失", money, uident);
		}
	} else {
		if (((myInfo->begExp >= (mcuInfo->begExp * 5 + 50)) || (random() % 4))
				&& currtime > mcuInfo->insuranceTime){
			money = MIN_XY(mcuInfo->cash / 2, 10000000);
			mcuInfo->cash -= money;
			myInfo->cash += money;
			myInfo->begExp += 5;
			prints
			    ("哈哈，成功了！你用刀片在%s的衣兜割了一个口子,偷到%d%s。\n\n\033[32m    你的身法提高了！\033[m",
			     uident, money, MONEY_NAME);
			if (myInfo->luck > -100) {
				myInfo->luck = MAX_XY((myInfo->luck - 5), -100);
				prints("\n\033[31m    你的人品降低了！");
			}
			sprintf(genbuf,
				"你去看《天下无贼》，结果在电影院被偷走 %d%s，真是欲哭无泪啊。\n",
				money, MONEY_NAME);
			sprintf(buf, "你遇到窃贼");
			mcLog("偷到", money, uident);
		} else {
			money = MIN_XY(myInfo->cash / 2, 10000000);
			mcuInfo->cash += money;
			myInfo->cash -= money;
			myInfo->begExp = MAX_XY((myInfo->begExp - 10), 0);
			prints
			    ("哈哈，成功了！你用刀片在%s的衣兜割了一个口子，偷到...咦？\n    一张字条：“21世纪最缺什么？。。。。”\n    你遇到黎叔，损失了%d%s...\n\n\033[31m    你的身法大降！\033[m",
			     uident, money, MONEY_NAME);
			sprintf(genbuf,
				"%s 偷鸡不成反蚀把米，被你顺手牵羊拿走 %d%s，嘿嘿～\n",
				currentuser->userid, money, MONEY_NAME);
			sprintf(buf, "移花接木成功");
			mcLog("行窃遇到黎叔，损失", money, uident);
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
		showAt(4, 4, "黑帮成员就是要无恶不作。", YEA);
        	if (!getOkUser("你想绑架谁？", uident, 5, 4)) {
			showAt(7, 4, "查无此人", YEA);
			return 0;   
		}
		if (!strcmp(uident, currentuser->userid)) {
			showAt(7, 4, "牛魔王：“老婆～快来看神经病啦～”", YEA);
			return 0;
		}
		move(6, 4);
		if (askyn("你确定要绑架他吗？", NA, NA) == NA) {
			showAt(7, 4, "什么？你还没想好？？想好了再来！！", YEA);
			return 0;
		}
		update_health();
		if (check_health(100, 7, 4, "你没有那么多体力绑架别人。", YEA)) {
			return 0;
		}
		if (myInfo->robExp < 1000 || myInfo->begExp < 1000) {
			showAt(7, 4, "你没有足够的经验来干这么大的事情。", YEA);
			return 0;
		}
		if (myInfo->luck < 80) {
			showAt(7, 4, "由于你有前科，警察时刻监控着你的活动，你没法行动", YEA);
			return 0;
		}
		currtime = time(NULL);
		if (currtime <myInfo->lastActiveTime + 7200) {
			showAt(7, 4, "你刚做了个大案子，还是继续躲避一下风声吧。", YEA);
			return 0;
		}
		showAt(7, 4, "你看见有警察在他的住所外面巡逻。", YEA);
		myInfo->lastActiveTime = currtime;
		myInfo->luck -= 10;
		if (random() % 5 != 0) {
			showAt(8, 4, "你被迫取消了绑架行动。", YEA);
			sprintf(title, "【黑帮】有人在策划绑架行动");
			sprintf(content, "    大家要多加小心。\n");
			deliverreport(title, content);
			return 0;
		}
		if (askyn("你还要继续进行绑架行动吗？", NA, NA) == NA) {
			showAt(8, 4, "你被迫取消了绑架行动。", YEA);
			myInfo->luck -= 10;
			myInfo->health -= 50;
			sprintf(title, "【黑帮】有人预谋绑架别人");
			sprintf(content, "    幸亏他在" MY_BBS_NAME "上走漏了风声，在大家的教诲下他改邪归正了。\n");
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
			prints("    你被警察抓住了！\n"
				"    你的胆识和身法各降50点！\n"
				"    你的体力降为0！\n"
				"    你的人品下降20点！\n"
				"    你被警察关押3小时！\n"
				"    你偷鸡不成反蚀把米，真是欲哭无泪啊！");
			sprintf(title, "【黑帮】%s绑架失败", currentuser->userid);
			sprintf(content, "    黑帮成员%s妄图绑架他人，幸好被警察及时发现。\n"
					"    他被警察关押3小时。\n", currentuser->userid);
			deliverreport(title, content);
			saveData(myInfo, sizeof (struct mcUserInfo));
			saveData(mcEnv, sizeof (struct MC_Env));
			pressanykey();
			mcLog("绑架失败", 0, uident);
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
		prints("    你绑架成功！\n"
			"    你的胆识和身法各加50点！\n"
			"    你的人品下降20点！\n");
		if (currtime < mcuInfo->insuranceTime)
			prints("    他投保了，损失减少一半！\n");
		prints("    他拿出了%d来赎身！\n"
			"    经历了这么大的事，你虚脱了。", transcash + transcredit);
		mcLog("绑架成功，获得现金", transcash, uident);
		mcLog("绑架成功，获得存款", transcredit, uident);
		sprintf(title, "【黑帮】黑帮又实施一次绑架行动");
		if (currtime < mcuInfo->insuranceTime)
			sprintf(content, "    某富翁不幸被黑帮绑架。\n"
					"    幸好他投保了，交出了财产的1/10作为赎身费才重获自由。");
		else
			sprintf(content, "    某富翁不幸被黑帮绑架。\n"
					"    交出了财产的1/5作为赎身费才重获自由。\n");
		saveData(mcuInfo, sizeof (struct mcUserInfo));
		deliverreport(title, content);
		sprintf(title, "你被黑帮绑架！");
		sprintf(content, "    你不幸被黑帮绑架，交了%d的赎身费", transcash + transcredit);
		system_mail_buf(content, strlen(content), uident, title, currentuser->userid);
		pressanykey();
		break;
	case 1:
		showAt(4, 4, "丐帮弟子总是受欺压，你练成了吸星大法有他们好看的！", YEA);
		if (!getOkUser("你要向谁下手？", uident, 5, 4)) {
			showAt(7, 4, "查无此人", YEA);
			return 0;   
		}                    
	        if (!strcmp(uident, currentuser->userid)) {
			showAt(7, 4, "牛魔王：“老婆～快来看神经病啦～”", YEA);
			return 0;   
		}
		move(6, 4);
		if (askyn("你确定要对他施展吸星大法吗？", NA, NA) == NA) {
			showAt(7, 4, "哦？你还不忍心下手？把心肠练硬了再来吧。", YEA);
			return 0;
		}
		update_health();
		if (check_health(100, 7, 4, "你没有那么多体力施展吸星大法。", YEA)) {
			return 0;
		}
		if (myInfo->robExp < 1000 || myInfo->begExp < 1000) {
			showAt(7, 4, "你的神功还差点火候，继续修炼吧。", YEA);
			return 0;
		}
		if (myInfo->luck < 80) {
			showAt(7, 4, "由于你劣迹斑斑，他不和你接近，你没法下手。", YEA);
			return 0;
		}
		currtime = time(NULL);
		if (currtime < myInfo->lastActiveTime + 7200) {
			showAt(7, 4, "你还需要继续运功才能施展吸星大法。", YEA);
			return 0;
		}
		showAt(7, 4, "你感觉不到他的内息，难道他练成了金钟罩铁布衫？", YEA);
		myInfo->lastActiveTime = currtime;
		myInfo->luck -= 10;
		if (random() % 5 != 0) {
			showAt(8, 4, "你打消了施展吸星大法的念头。", YEA);
			sprintf(title, "【丐帮】有人在修炼吸星大法");
			sprintf(content, "    大家要多加小心，不要着了道。\n");
			deliverreport(title, content);
			return 0;
		}
		if (askyn("你还要再尝试一下吗？", NA, NA) == NA) {
			showAt(8, 4, "你打消了施展吸星大法的念头。", YEA);
			myInfo->luck -= 20;
			myInfo->health -= 50;
			sprintf(title, "【丐帮】有人在施展吸星大法");
			sprintf(content, "    幸亏他悬崖勒马，没有继续做伤天害理的事。\n");
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
			prints("    不好！你被他的内力反震，受了内伤！\n"
				"    你的胆识和身法各降50点！\n"
				"    你的体力降为0！\n"
				"    你的人品下降20点！\n"
				"    你偷鸡不成反蚀把米，真是欲哭无泪啊！");
			sprintf(title, "【丐帮】%s施展吸星大法失败", currentuser->userid);
			sprintf(content, "    丐帮成员%s妄图使用吸星大法吸取他人功力，结果技不如人反被震伤。\n"
					"    需要修养4小时才能再次施展吸星大法。\n", currentuser->userid);
			deliverreport(title, content);
			sprintf(title, "你将%s震伤！", currentuser->userid);
			sprintf(content, "    %s试图用吸星大法对付你，结果反被你震伤。\n"
					"    你的胆识和身法各增加了10点！\n", currentuser->userid);
			system_mail_buf(content, strlen(content), uident, title, currentuser->userid);
			pressanykey();
			mcLog("吸星大法失败", -50, uident);
			return 0;
		}
		if (mcuInfo->robExp < 900 || mcuInfo->begExp < 900) {
			myInfo->luck -= 10;
			move(9, 0);
			prints("    对方那么弱你竟然还忍心下手！\n"
				"    你的人品下降10点！");
			sprintf(title, "【丐帮】%s施展吸星大法未果", currentuser->userid);
			sprintf(content, "    丐帮成员%s不顾江湖正义，使用吸星大法"
					"对付弱者，人品下降10点！\n", currentuser->userid);
			deliverreport(title, content);
			pressanykey();
			mcLog("吸星大法对付弱者", -10, uident);
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
		prints("    你施展吸星大法成功！\n"
			"    你的胆识增加%d点，身法增加%d点！\n"
			"    你的人品下降20点！\n"
			"    经历了这么大的事，你虚脱了。", transrob, transbeg);
		mcLog("吸星大法成功，获得胆识", transrob, uident);
		mcLog("吸星大法成功，获得身法", transbeg, uident);
		saveData(mcuInfo, sizeof (struct mcUserInfo));
		sprintf(title, "【丐帮】%s不幸成为吸星大法的受害者", uident);
		sprintf(content, "    %s功力损失了1/5。\n", uident);
		deliverreport(title, content);
		sprintf(title, "你被吸星大法命中！");
		sprintf(content, "    你不幸被人施了吸星大法，损失了%d的胆识和%d的身法",
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

	sprintf(buf, "%s黑帮", CENTER_NAME);
	money_show_stat(buf);
	move(4, 4);
	if (check_health(5, 12, 4, "您的体力不够了！", YEA))
		return 0;
	prints("这里的板砖质地优良，拿去拍人一定痛快。\n"
	       "    现在大酬宾，一块板砖才 %d %s。", BRICK_PRICE, MONEY_NAME);
	move(6, 4);
	usercomplete("你要拍谁:", uident);
	if (uident[0] == '\0')
		return 0;
	if (!getuser(uident, &lookupuser)) {
		showAt(7, 4, "错误的使用者代号...", YEA);
		return 0;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(7, 4, "牛魔王：“老婆～快来看神经病啦～”", YEA);
		return 0;
	}
	if (t_search(uident, NA, 1)) {
		showAt(7, 4, "他已经有了防备，你没法下手，只好作罢。", YEA);
		return 0;
	}
	count = userInputValue(7, 4, "拍", "块砖", 1, 100);
	if (count < 0)
		return 0;
	num = count * BRICK_PRICE;
	if (myInfo->cash < num) {
		move(9, 4);
		prints("您的钱不够...需要 %d %s", num, MONEY_NAME);
		pressanykey();
		return 0;
	}
	if (myInfo->luck > -100) {
		myInfo->luck = MAX_XY((myInfo->luck - 1), -100);
		prints("\033[31m    你的人品降低了！\033[m");
	}
	mcLog("买了", num, "元钱的板砖");
	myInfo->cash -= num;
	mcEnv->prize777 += after_tax(num);	//拍砖的钱流回777 
	move(10, 8);
	prints("经过几天的偷窥和跟踪，你发现每天早上7点10分%s会路过僻静的",
	       uident);
	move(11, 4);
	prints("三角地。今天你拿着买来的板砖，准备行动了...");
	move(12, 8);
	prints("拍砖是很危险的喔！ 搞不好会出人命的，小心啊！");
	move(13, 4);
	if (askyn("废话少说，你还想拍么？", YEA, NA) == NA) {
		move(15, 4);
		myInfo->robExp = MAX_XY((myInfo->robExp - 1), 0);
		update_health();
		myInfo->health--;
		myInfo->Actived++;
		prints
		    ("唉，最后关头你害怕了，所以不拍了。\n    你的胆识减少了。");
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
	num = count * BRICK_PRICE/2 + 1000;	//医药费 
	update_health();
	myInfo->health -= 5;
	myInfo->Actived += 2;
	move(16, 4);
	if (r + count < 101) {	//目标胆识大于自己约10倍就会必反弹，嘿嘿 
		prints("很不幸，你没有拍中。反而被砸中小脑袋瓜...");
		move(17, 4);
		prints("你流血不止，被送到医院缝了十多针，好惨啊！");
		move(18, 4);
		if (random() % 2) {
			myInfo->robExp = MAX_XY((myInfo->robExp - 1), 0);
			prints("你的胆识减少了。\n    ");
		}
		if (random() % 2) {
			myInfo->robExp = MAX_XY((myInfo->robExp - 1), 0);
			prints("你的身法降低了。\n    ");
		}
		mcEnv->prize777 += after_tax(MIN_XY(myInfo->cash, num * 2));
		myInfo->cash = MAX_XY((myInfo->cash - num * 2), 0);
		update_health();
		myInfo->health = 0;
		prints("最后还结了 %d %s的医药费，看你以后还敢不。", num * 2,
		       MONEY_NAME);
		pressanykey();
		mcLog("拍砖失败，花了", num*2, "的医疗费");
		return 0;
	}
	if (myInfo->begExp > mcuInfo->begExp * 20 + 50) {
		myInfo->luck--;
		prints("对方那么弱你居然忍心下手！\n    你的人品降低了。");
		pressanykey();
		return 0;
	}
	if (mcuInfo->guard > 0 && rand() % 3) {
		prints("斜刺里突然冲出一只大狼狗，你一慌神，板砖全砸空了。\n"
		       "    哇，大狼狗朝你扑过来了！");
		if (askyn("你要不要跑？", YEA, NA) == YEA) {
			if (random() % 2) {
				myInfo->robExp =
				    MAX_XY((myInfo->robExp - 1), 0);
				prints("你的胆识减少了。\n    ");
			}
			if (random() % 4) {
				prints
				    ("\n    你一看不妙，飞快的钻进一条小巷跑掉了。好险啊～");
				pressanykey();
				return 0;
			} else
				prints
				    ("\n    你居然跑进了一条死胡同，狼狗跟上来了。。。");
		}

		if (random() % 3) {
			mcuInfo->guard--;
			myInfo->health -= 5;
			sleep(1);
			prints("\n    经过激烈搏斗，你终于干掉了大狼狗，哈哈");
			sprintf(buf,
				"你的一只大狼狗被%s干掉了，你现在还剩%d只大狼狗。",
				currentuser->userid, mcuInfo->guard);
			system_mail_buf(buf, strlen(buf), uident,
					"你的一只大狼狗壮烈牺牲",
					currentuser->userid);
			if (!(random() % 4)) {
				myInfo->robExp++;
				prints("\n    你的胆识增加了！");
			}
			prints("\n    你气喘吁吁，体力下降！");
			mcLog("干掉大狼狗", 1, uident);
		} else {
			sleep(1);
			prints
			    ("\n    你虽奋力搏斗，最后还是被大狼狗在腿上咬了一口。");
			if (!(random() % 3)) {
				if (myInfo->begExp) {
					myInfo->begExp--;
					prints("\n    你的身法降低了！");
				}
				update_health();
				myInfo->health = myInfo->health / 2;
				prints("\n    你的体力减半！");
				sprintf(buf,
					"%s被你的大狼狗咬伤，记得奖励狼狗几块骨头哦！",
					currentuser->userid);
				system_mail_buf(buf, strlen(buf), uident,
						"你的大狼狗成功保护你",
						currentuser->userid);
			}
		}
		goto UNMAP;
	}
	if ((random() % 5 || (myInfo->begExp > mcuInfo->begExp * 10 + 50)) &&
	    !(mcuInfo->begExp > myInfo->begExp * 10 + 50)) {
		prints("你这坏蛋，背后偷袭，砸中%s的小脑袋瓜。", uident);
		if (mcuInfo->cash < num) {
			move(17, 4);
			update_health();
			mcuInfo->health = 0;
			mcLog("拍砖成功，他花医疗费", mcuInfo->cash, uident);
			mcEnv->prize777 += after_tax(mcuInfo->cash);
			mcuInfo->cash = 0;
			prints("你都拍到人家没钱了治伤了...积点阴德吧！");
			sprintf(buf,
				"你被%s拍了板砖，你没钱治伤，只能咬牙忍痛...",
				currentuser->userid);
			if (!(random() % 5)) {
				myInfo->robExp++;
				prints("\n    你的胆识增加了！");
			}
			if (!(random() % 5)) {
				myInfo->begExp++;
				prints("\n    你的身法增加了！");
			}
		} else {
			mcEnv->prize777 += after_tax(num);
			mcuInfo->cash -= num;
			mcuInfo->health -= 10;
			mcLog("拍砖成功，他花医疗费", num, uident);
			if (!(random() % 5))
				mcuInfo->begExp = MAX_XY(0, mcuInfo->begExp--);
			move(17, 4);
			prints("哈哈，%s花了%d%s治伤，在医院里躺了好多天！",
			       uident, num, MONEY_NAME);
			sprintf(buf, "你被%s拍了板砖，花了%d%s治伤，呜呜呜...",
				currentuser->userid, num, MONEY_NAME);
			if (random() % 2) {
				myInfo->robExp++;
				prints("\n    你的胆识增加了！");
			}
			if (random() % 2) {
				myInfo->begExp++;
				prints("\n    你的身法增加了！");
			}
		}
		system_mail_buf(buf, strlen(buf), uident, "你被拍了板砖",
				currentuser->userid);
	} else {
		if (random() % 2) {
			mcuInfo->begExp++;
			sprintf(buf,
				"%s拿板砖拍你落空，你的身法增加了，哦耶～\n",
				currentuser->userid);
		} else {
			mcuInfo->robExp++;
			sprintf(buf,
				"%s拿板砖拍你落空，你的胆识增加了，哦耶～\n",
				currentuser->userid);
		}
		mcLog("拍砖落空", count, uident);
		system_mail_buf(buf, strlen(buf), uident, "你躲过板砖袭击",
				currentuser->userid);
		prints("啊呀呀，没拍中。。。");

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

	if (check_health(90, 12, 4, "这么大的事情没有充沛体力无法完成。", YEA))
		return;
	if (myInfo->robExp < 100) {
		showAt(12, 4, "你犹豫了半天，还是没敢动手。。。", YEA);
		return;
	}
	if (myInfo->begExp < 50) {
		showAt(12, 4,
		       "你刚想动手，结果刚站起来就被边上的凳子绊倒了。。。",
		       YEA);
		return;
	}
	if (myInfo->luck < 60) {
		showAt(12, 4, "你总觉得背后有双眼睛在盯着你看。", YEA);
		return;
	}

	nomoney_show_stat("黑市");
	move(4, 4);
	currtime = time(NULL);
	if (currtime < 3600 + myInfo->lastActiveTime) {
		showAt(12, 4,
		       "刚才出了个案子，有警察在附近巡视，先不要动手为好。",
		       YEA);
		return;
	}

	if (askyn("武装抢劫就要买武器，装备需要20万，你确定要买吗？", NA, NA) ==
	    NA) {
		myInfo->robExp--;
		prints("\n    你决定不买了。\n    你的胆识降低！");
		return;
	}
	if (myInfo->cash < 200000) {
		showAt(12, 4, "一手交钱一手交货，你没带足够的现金。", YEA);
		return;
	}
	update_health();
	myInfo->health -= 10;
	prints("\n    你拎上%s牌冲锋枪，穿上%s牌小马甲，哇！帅呆了！",
	       CENTER_NAME, CENTER_NAME);
	pressanykey();

	nomoney_show_stat("赌场大厅");
	move(4, 4);
	if (askyn("抢赌场很危险的，你真的要动手吗？", NA, NA) == NA) {
		myInfo->robExp -= 2;
		prints
		    ("\n    你决定放弃了！还偷偷把装备扔进了附近的垃圾桶。\n    你的胆识降低！");
		pressanykey();
		mcLog("放弃抢劫赌场", 0, "");
		return;
	}

	myInfo->lastActiveTime = currtime;
	update_health();
	myInfo->health -= 50;
	myInfo->luck -= 10;
	myInfo->Actived += 10;

	move(6, 4);
	prints
	    ("你一把亮出冲锋枪，对着赌客大喊：”喂～安静点，没看这里抢劫吗？！“");
	pressanykey();

	sleep(1);

	move(8, 4);
	if (random() % 2) {
		myInfo->robExp -= 10;
		prints
		    ("周围的人哈哈大笑，神经病，你拿把水枪干吗啊？！\n    啊，被黑店骗了！你转身就跑。\n    你的体力减半\n    你的胆识减少10点。");
		sprintf(buf,
			"    一个拿着水枪身穿马甲的神经病在赌场发彪后逃逸。\n"
			"    希望有知情者能向警方提供有关消息。\n");
		deliverreport("【新闻】神经病赌场发彪", buf);
		pressreturn();
		mcLog("抢劫赌场被当神经病", 0, "");
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
		    ("\n    乘着周围的人目瞪口呆之际，你拿出布袋把777台上的钱往里装满。\n    然后潇洒的一转身走出大门。\n    你的胆识增加！\n    你的身法增加！");
		sprintf(title, "【新闻】蒙面人武装抢劫赌场");
		sprintf(buf,
			"    本站刚刚收到的消息：早些时候，一蒙面人武装抢劫赌场后逃逸。\n"
			"    目前警署正式介入调查此事。希望正义市民提供相关线索。\n");
		mcLog("抢劫赌场成功，获得", num, "");
	} else {
		myInfo->health = 0;
		myInfo->robExp -= 20;
		myInfo->luck -= 20;
		prints
		    ("\n    刷刷刷，周围出现一堆全副武装的警察，你只好束手就擒！\n    原来警方早就得到了线报，cmft\n"
		     "    你的体力全失！\n    你的人品降低！\n    你的胆识减少20点!\n");
		sprintf(title, "【新闻】警方守株待兔 %s束手就擒",
			currentuser->userid);
		sprintf(buf,
			"    警方根据早先得到的线报埋伏在%s赌场，在%s武装抢劫的时候，成功将其擒获\n"
			"    在审讯过程中发现%s为精神病患者，所以免于刑事诉讼，取保就医。\n",
			CENTER_NAME, currentuser->userid, currentuser->userid);
		mcLog("抢劫赌场失败", 0, "");
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
		money_show_stat("黑帮总部");
		move(4, 4);
		prints
		    ("两年前的%s黑帮无恶不作，名噪一时，被警察严打后有一段时间消声匿迹。"
		     "\n    不过最近又逐渐活跃起来，作案手段也趋于隐蔽化多样化。",
		     CENTER_NAME);
		move(7, 4);
		prints
		    ("一个黑衣人走过来小声说：“要板砖么？拍人很疼的。拍好了还能长胆识身法。”");
		move(t_lines - 1, 0);
#if 0
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]拍砖 [2]交保护费 [3]黑帮活动 [4] 帮主号令 [Q]离开\033[m");
#else
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]拍砖 [2]黑帮活动 [3] 帮主号令 [4] 交保护费 [Q]离开\033[m");
#endif
		ch = igetkey();
		switch (ch) {
		case '1':
			money_pat();
			break;
#if 0
		case '2':
			money_show_stat("黑帮保护费收费处");
			move(6, 4);
			prints
			    ("所谓消财免灾，交了保护费，就能保一段时间平安，不在黑帮行动对象之内。");
			showAt(12, 4, "\033[1;32m正在策划中。\033[m", YEA);
			break;
#endif
		case '2':
			if (!seek_in_file(ROBUNION, currentuser->userid)) {
				move(12, 4);
				prints("你不是黑帮的！别想混进来！！！");
				pressanykey();
				break;
			}
			if (seek_in_file(ROBUNION, currentuser->userid)
				&& seek_in_file(BEGGAR, currentuser->userid)) {
				move(12, 4);
				prints
				    ("黑衣人：嘘～最近黑帮跟丐帮水火不容，你脚踏两只船，还是不要暴露为好。");
				pressanykey();
				break;
			}
			while (!quit) {
				money_show_stat("黑帮活动");
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m 选单 \033[1;46m [1]勒索 [2]抢劫 [3]劫赌场 [4]绑票 [5]退出黑帮 [Q]离开\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					money_show_stat("背阴巷");
					forceGetMoney(0);
					break;
				case '2':
					money_show_stat("光天化日");
					forceGetMoney(2);
					break;
				case '3':
					money_show_stat("赌场");
					RobShop();
					break;
				case '4':
					money_show_stat("黑窝");
					RobPeople(0);
					break;
				case '5':
					money_show_stat("教化所");
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
				nomoney_show_stat("黑帮帮主");
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m 选单 \033[1;46m [1]下令分赃 [2]黑吃黑 [3]查看本帮资产 [Q]离开\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					nomoney_show_stat("销金窟");
					showAt(12, 4,
					       "\033[1;32m正在建设中。\033[m",
					       YEA);
					break;
				case '2':
					money_show_stat("黑吃黑");
					showAt(12, 4,
					       "\033[1;32m苦练绝招中。\033[m",
					       YEA);
//forcerobMoney(2); 
					break;
				case '3':
					money_show_stat("小金库");
					showAt(12, 4,
					       "\033[1;32m会计正在点钱，请稍候。\033[m",
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
			money_show_stat("保护费交费点");
			move(12, 4);
			if (askyn("\033[1;32m最近黑帮到处打劫，你要不要找人罩着你？\033[m", YEA, NA)==YEA){
				tempMoney = userInputValue(13, 4, "要出资","万", 5, 100) * 10000;
				if (tempMoney < 0) break;
				if (myInfo->cash < tempMoney){
					showAt(15, 4, "\033[1;37m你胆敢戏耍黑帮，是不是活的不耐烦了！\033[m", YEA);
					break;
				}
				update_health();
				if (check_health(1, 15, 4, "你的体力不够了！", YEA))
					break;
				move(15, 4);
				prints("\033[1;37m你交了%d%s保护费\n"
				       "    你的胆识增加了%d点！\033[m\n", 
				       tempMoney, MONEY_NAME, tempMoney / 50000);
				mcLog("交纳保护费", tempMoney, "元");
				myInfo->health--;
				myInfo->Actived += tempMoney / 25000;
				myInfo->cash -= tempMoney;
				myInfo->robExp += tempMoney / 50000;
				mcEnv->Treasury += tempMoney;
				if (tempMoney == 100000 &&
					(!seek_in_file(ROBUNION, currentuser->userid))) {
					if(askyn("    你要加入黑帮吗？", YEA, NA) == NA)
						break;
					if(seek_in_file(BEGGAR, currentuser->userid)) {
						showAt(17, 4, "你已经是丐帮成员了！\n", NA);
						if(askyn("    你要退出丐帮吗？", YEA, NA) == NA)
							break;
						del_from_file(BEGGAR, currentuser->userid);
						mcLog("退出丐帮", 0, "");
						showAt(19, 4, "你已经退出丐帮了。", YEA);
					}
					if(seek_in_file(POLICE, currentuser->userid)) {
						showAt(17, 4, "你已经是警察了！    \n", NA);
						if(askyn("    你要退出警察队伍吗？", YEA, NA) == NA)
							break;
						del_from_file(POLICE, currentuser->userid);
						myInfo->luck = MAX_XY(-100, myInfo->luck - 50);
						mcLog("退出警界", 0, "");
						showAt(19, 4, "你已经不再是警察了。", YEA);
					}
					addtofile(ROBUNION, currentuser->userid);
					showAt(20, 4, "你已经加入了黑帮！", NA);
					sprintf(title, "【黑帮】%s加入黑帮", currentuser->userid);
					sprintf(content, "    大家小心，他可能会抢赌场或者绑架亿万富翁！\n");
					deliverreport(title, content);
					mcLog("加入黑帮", 0, "");
				}
				pressanykey();
			}else{
				showAt(14, 4, "\033[1;33m遇到人打劫可别说我没警告过你！\033[m", YEA);
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

	if (check_health(90, 12, 4, "这么大的事情没有充沛体力无法完成。", YEA))
		return;
	if (myInfo->robExp < 50) {
		showAt(12, 4, "别怕别怕。。。你的手抖的太厉害了。。。", YEA);
		return;
	}
	if (myInfo->begExp < 100) {
		showAt(12, 4, "凭你现在的身法，必死无疑！再去练练吧。。。",
		       YEA);
		return;
	}
	if (myInfo->luck < 60) {
		showAt(12, 4, "你根本打听不到关键的情报。。。", YEA);
		return;
	}

	currtime = time(NULL);
	if (currtime < 3600 + myInfo->lastActiveTime) {
		showAt(12, 4, "银行系统刚刚换过，你还要花点时间熟悉一下。",
		       YEA);
		return;
	}
	move(4, 4);
        if (askyn("入侵银行系统需要购买电脑，软件，需要20万，你确定要买吗？"
			, NA, NA) == NA) {
		myInfo->begExp--;
		prints("\n    你决定不买了。\n    你的身法降低！");
		return;
	}
	myInfo->health -= 10;
	
	if (myInfo->cash < 200000) {
		showAt(12, 4, "一手交钱一手交货，你没带足够的现金。", YEA);
		return;
	}
		 
	myInfo->lastActiveTime = currtime;
	update_health();
	myInfo->health /= 2;
	myInfo->luck -= 10;
	myInfo->Actived += 10;
	move(5, 4);
	prints
	    ("你坐在电脑前面，从容的点上一根烟，然后快速敲了几下键盘。\n    屏幕上显示：开始入侵%s银行系统！",
	     CENTER_NAME);
	pressanykey();
	move(7, 4);
	prints("正在连接。。。请稍候");
	sleep(1);

	move(8, 4);
	if (random() % 2) {
		myInfo->begExp -= 10;
		prints
		    ("嘟嘟嘟，银行的超级计算机发现了你的入侵，你赶紧断开了连接。好险！\n    你的体力减半\n    你的身法减少10点。");
		sprintf(buf,
			"    超级计算机刚刚监视到了一次入侵银行系统的行为，在试图将入侵者锁定的时候，\n"
			"    被其察觉而逃脱。希望有知情者能向警方提供有关消息。\n");
		deliverreport("【新闻】不明人士入侵银行系统失败", buf);
		pressreturn();
		mcLog("入侵银行失败", 0, "");
		return;
	}
	prints
	    ("恭喜你！已经成功连接到了银行转帐系统。\n    你有一次机会从别人的帐户转帐到你帐户。\n");
	usercomplete("    你要从谁那里转帐过来？按Enter取消。", uident);
	if (uident[0] == '\0') {
		myInfo->robExp -= 10;
		prints
		    ("\n    你胆怯放弃了。。。。\n    你的体力减半\n    你的胆识减少10点。");
		return;
	}
	if (!getuser(uident, &lookupuser)) {
		myInfo->begExp -= 10;
		showAt(12, 4,
		       "\n    你手一抖输错了，还把热咖啡碰倒在裤子上了\n    你的体力减半\n    你的身法减少10点。",
		       YEA);
		return;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(7, 4, "啊哦，青山医院欢迎您～！”", YEA);
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
	sprintf(buf, "要从%s的帐户转过来", uident);
	num = userInputValue(12, 4, buf, MONEY_NAME, 0, num);
	if (num < 0) {
		myInfo->robExp -= 20;
		prints("\n    你胆怯放弃了。。。。\n    你的体力减半\n    你的胆识减少20点。");
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
		sprintf(buf, "记得经常去你的银行帐户查查余额哦。");
		system_mail_buf(buf, strlen(buf), uident, "友情提醒",
				"deliver");
		prints
		    ("\n    恭喜你！转帐成功！\n    你的胆识增加！\n"
		       "    你的身法增加！\n    你的体力耗尽！\n");
		if (currtime < mcuInfo->insuranceTime)
			prints("    由于他投保了，你只得到了一半的钱。");
		sprintf(title, "【谣言】可疑人物入侵银行转帐系统");
		sprintf(buf,
			"    据坊间流传的谣言，有人成功入侵了银行系统。\n"
			"    但银行的超级计算机并无监视到入侵行为。\n"
			"    目前警署正在调查此消息的真实度。\n"
			"    请各位关注自己的帐户，及时提供相关消息。\n");
		mcLog("入侵银行成功，获得", num, uident);
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
		    ("    超级计算机监视到了你的可疑行动，你被抓住了！\n    你的体力归零！\n"
		     "    你的人品减少20点！\n    你的身法减少20点!\n");
		if (currtime < myInfo->insuranceTime)
			prints("    幸好你投保了，你被罚款%d%s!", num, MONEY_NAME);
		else
			prints("    你被罚款%d%s!", num, MONEY_NAME);
		sprintf(title, "【新闻】%s入侵银行系统被抓获",
			currentuser->userid);
		sprintf(buf,
			"    超级计算机刚刚监视到了%s入侵银行系统的行为，并成功将其锁定！\n"
			"    鉴于其认罪态度较好，本着治病救人的原则。罚款%d%s!\n"
			"    其中一半归受害者做为压惊费，一半注入777。\n"
			"    诸君引以为戒！\n",
			currentuser->userid, num, MONEY_NAME);
		mcLog("入侵银行被抓，损失", num, uident);
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
		money_show_stat("丐帮总舵");
		showAt(4, 4,
		       "丐帮自古天下第一大帮，现在贫富差距越来越大，做乞丐的人也多起了。\n"
		       "    为了生计，免不得有偷鸡摸狗肯蒙拐骗之事，当然也有劫富济贫之举。\n\n"
		       "    一个乞丐走过来问道：“要打听消息么？丐帮天上地下无所不知，无所不晓。”",
		       NA);
		move(t_lines - 1, 0);
#if 0
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]打探 [2]鸡毛信 [3]丐帮活动 [4]帮主号令 [Q]离开\033[m");
#else
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]打探 [2]丐帮活动 [3]帮主号令 [4]救济穷人 [Q]离开\033[m");
#endif
		ch = igetkey();

		switch (ch) {
		case '1':
			update_health();
			if (check_health
			    (2, 12, 4, "歇会歇会，神志不清就不要查了！", YEA))
				break;
			if (!getOkUser("查谁的家底？", uident, 8, 4))
				break;
			if (myInfo->cash < 1000) {
				showAt(9, 4, "啊，你只带了这么点钱吗？", YEA);
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
			    ("\033[1;31m%s\033[m 大约有 \033[1;31m%d\033[m 万%s的现金"
			     "，以及 \033[1;31m%d\033[m 万%s的存款。", uident,
			     mcuInfo->cash / 100000 * 10, MONEY_NAME,
			     mcuInfo->credit / 100000 * 10, MONEY_NAME);
			move(11, 4);
			prints
			    ("\033[1;31m%s\033[m 的胆识 \033[1;31m%d\033[m  身法 "
			     "\033[1;31m%d\033[m  人品 \033[1;31m%d\033[m  体力 "
			     "\033[1;31m%d\033[m  大狼狗 \033[1;31m%d\033[m 条\n",
			     uident, mcuInfo->robExp, mcuInfo->begExp,
			     mcuInfo->luck, MAX_XY(100, mcuInfo->health),
			     mcuInfo->guard);
			mcLog("打探", 0, uident);
			if (myInfo->cash < 10000) {
				saveData(mcuInfo, sizeof (struct mcUserInfo));
				pressanykey();
				break;
			}
			move(13, 4);
			if (askyn("你要雇佣私家侦探调查他的详细经济情况吗？", NA, NA) == YEA) {
				myInfo->cash -= 10000;
				mcEnv->Treasury += 10000;
				move(14, 4);
				prints("\033[1;31m%s\033[m 有 \033[1;31m%d\033[m %s的现金，"
				       "\033[1;31m%d\033[m %s的存款，\n    "
				       "\033[1;31m%s\033[m 还有 \033[1;31m%d\033[m %s的利息，"
				       "\033[1;31m%d\033[m %s的贷款，\n    "
				       "以及 \033[1;31m%d\033[m %s放在保险箱里。",
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
			money_show_stat("养鸡场");
			move(6, 4);
			prints
			    ("有财在身，免不得有人暗地里眼红。不过好在丐帮消息灵通。\n"
			     "预交一笔鸡毛信邮费，有人暗算你的时候，就能获知鸡毛信，\n"
			     "使得安然无恙的几率增加。");
			showAt(12, 4, "\033[1;32m正在策划中。\033[m", YEA);
			break;
#endif
		case '2':
			if (!seek_in_file(BEGGAR, currentuser->userid)) {
				move(12, 4);
				prints("你衣冠整整，怎么做乞丐啊。。。");
				pressanykey();
				break;
			}
			if (seek_in_file(ROBUNION, currentuser->userid)
				&& seek_in_file(BEGGAR, currentuser->userid)) {
				move(12, 4);
				prints
				    ("黑衣人：嘘～最近黑帮跟丐帮水火不容，你脚踏两只船，还是不要暴露为好。");
				pressanykey();
				break;
			}
			while (!quit) {
				money_show_stat("丐帮活动");
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m 选单 \033[1;46m [1]卖花 [2]妙手空空 [3] 瞒天过海 [4]吸星大法 [5] 退出丐帮 [Q]离开\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					money_show_stat("闹市区");
					forceGetMoney(1);
					break;
				case '2':
					money_show_stat("商业街");
					forceGetMoney(3);
					break;
				case '3':
					money_show_stat("黑客帝国");
					stealbank();
					break;
				case '4':
					money_show_stat("闭关小屋");
					RobPeople(1);
					break;
				case '5':
					money_show_stat("从良局");
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
				nomoney_show_stat("丐帮帮主");
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m 选单 \033[1;46m [1]均贫富 [2]打狗棒法 [3]查看本帮资产 [Q]离开\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					nomoney_show_stat("大义厅");
					showAt(12, 4,
					       "\033[1;32m正在建设中。\033[m",
					       YEA);
					break;
				case '2':
					money_show_stat("天下无狗");
					showAt(12, 4,
					       "\033[1;32m练习中。\033[m", YEA);
//forcerobMoney(2); 
					break;
				case '3':
					money_show_stat("小金库");
					showAt(12, 4,
					       "\033[1;32m会计正在点钱，请稍候。\033[m",
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
			money_show_stat("粥棚");
			move(12, 4);
			if(askyn("\033[1;33m天下大旱，颗粒无收。你要不要救济一下穷人？\033[m", YEA, NA)==YEA){
				tempMoney = userInputValue(13, 4, "要出资","万", 5, 100) * 10000;
				if (tempMoney < 0) break;
				if (myInfo->cash < tempMoney){
					showAt(15, 4, "\033[1;37m没钱还当什么善人。\033[m", YEA);
					break;
				}
				update_health();
				if (check_health(1, 15, 4, "你的体力不够了！", YEA))
					break;
				move(15, 4);
				prints("\033[1;37m你给穷人发了%d%s的粮食\n"
				       "    你的身法增加了%d点！\033[m\n", 
				       tempMoney, MONEY_NAME, tempMoney / 50000);
				mcLog("救济灾民，买了", tempMoney, "元的粮食");
				myInfo->health--;
				myInfo->Actived += tempMoney / 25000;
				myInfo->cash -= tempMoney;
				myInfo->begExp += tempMoney / 50000;
				mcEnv->Treasury += tempMoney;
				if (tempMoney == 100000 && 
					(!seek_in_file(BEGGAR, currentuser->userid))) {
					if(askyn("    你要加入丐帮吗？", YEA, NA) == NA)
						break;
					if(seek_in_file(ROBUNION, currentuser->userid)) {
						showAt(17, 4, "你已经是黑帮成员了！\n", NA);
						if(askyn("    你要退出黑帮吗？", YEA, NA) == NA)
							break;
						del_from_file(ROBUNION, currentuser->userid);
						mcLog("退出黑帮", 0, "");
						showAt(19, 4, "你已经退出了黑帮。", YEA);
					}
					if(seek_in_file(POLICE, currentuser->userid)) {
						showAt(17, 4, "你已经是警员了！    \n", NA);
						if(askyn("    你要退出警察队伍吗？", YEA, NA) == NA)
							break;
						del_from_file(POLICE, currentuser->userid);
						myInfo->luck = MAX_XY(-100, myInfo->luck - 50);
						mcLog("退出警界", 0, "");
						showAt(19, 4, "你已经不再是警察了。", YEA);
					}
					addtofile(BEGGAR, currentuser->userid);
					showAt(20, 4, "你已经加入了丐帮！", NA);
					sprintf(title, "【丐帮】%s加入丐帮", currentuser->userid);
					sprintf(content, "    大家小心，他可能会抢银行或者施展吸星大法！\n");
					deliverreport(title, content);
					mcLog("加入丐帮", 0, "");
				}
				pressanykey();
			}else{
				showAt(14, 4, "\033[1;32m葛朗台！要那么多钱有什么用！\033[m", YEA);
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

//---------------------------------  股市  ---------------------------------// 

static int
userSelectStock(int n, int x, int y)
{
	int stockid;
	char buf[8];

	sprintf(genbuf, "请输入股票的代号[0-%d  \033[1;33mENTER\033[m放弃]:",
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
{				//管理股市 
	int ch, quit = 0, stockid;
	struct BoardStock *bs;

	while (!quit) {
		nomoney_show_stat("证监会");
		move(4, 4);
		prints("这里是管理股市的机构。请谨慎操作，对广大股民负责。");
		showAt(6, 0, "        1. 股票上市        2. 开启/关闭股市\n"
		             "        3. 股票退市        4. 暂停/恢复股票 Q. 退出\n\n"
		             "    请选择操作代号: ", NA);
		ch = igetkey();
		update_health();
		if (check_health
		    (1, 12, 4, "不要太辛苦，喝杯咖啡歇一会吧！", YEA))
			continue;
		switch (ch) {
		case '1':
			myInfo->health--;
			myInfo->Actived++;
			newStock(stockMem, n);
			break;
		case '2':
			if (!mcEnv->stockOpen)
				sprintf(genbuf, "%s", "确定要开启股市吗");
			else
				sprintf(genbuf, "%s", "确定要关闭股市吗");
			move(10, 4);
			if (askyn(genbuf, NA, NA) == NA)
				break;
			mcEnv->stockOpen = !mcEnv->stockOpen;
			mcLog(mcEnv->stockOpen?"关闭股市":"开启股市", 0, "");
			update_health();
			myInfo->health--;
			myInfo->Actived++;
			showAt(11, 4, "操作完成。", YEA);
			break;
		case '3':
			if ((stockid = userSelectStock(n, 10, 4)) == -1)
				break;
			bs = stockMem + stockid * sizeof (struct BoardStock);
			sprintf(genbuf,
				"\033[5;31m警告：\033[0m确定要将股票 %s 退市么",
				bs->boardname);
			move(11, 4);
			if (askyn(genbuf, NA, NA) == YEA) {
				bs->timeStamp = 0;	//股票废弃标志 
				bs->status = 3;
				deleteList(stockid);
				showAt(12, 4, "操作完成。", YEA);
			}
			mcLog("股票退市", 0, bs->boardname);
			update_health();
			myInfo->health--;
			myInfo->Actived++;
			break;
		case '4':
			if ((stockid = userSelectStock(n, 10, 4)) == -1)
				break;
			bs = stockMem + stockid * sizeof (struct BoardStock);
			sprintf(genbuf, "确定要\033[1;32m%s\033[0m股票 %s 吗",
				(bs->status == 3) ? "恢复" : "暂停",
				bs->boardname);
			move(11, 4);
			if (askyn(genbuf, NA, NA) == YEA) {
				mcLog((bs->status == 3) ? "恢复股市":"暂停股市",
						0, bs->boardname);
				bs->status = (bs->status == 3) ? 0 : 3;
				showAt(12, 4, "操作完成。", YEA);
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
{				//显示大盘走势 
	int i, yeste, today, change, stockid;
	char buf[STRLEN];
	struct BoardStock *bs;
	time_t currTime = time(NULL);
	struct tm *thist = localtime(&currTime);

	today = thist->tm_wday;
	yeste = (today <= 1) ? 6 : (today - 1);	//周日休市, 故周一的昨日算周六 
	change = mcEnv->stockPoint[today] - mcEnv->stockPoint[yeste];
	nomoney_show_stat("即时电子公告牌");
	sprintf(buf, "当前指数: %d        %s: %d点", mcEnv->stockPoint[today],
		change > 0 ? "涨" : "跌", abs(change));
	showAt(4, 4, buf, NA);
	showAt(6, 0,
	       "                  周一\t  周二\t  周三\t  周四\t  周五\t  周六\n\n"
	       "    \033[1;36m大盘走势\033[0m    ", NA);
	showAt(11, 0, "    \033[1;36m  成交手\033[0m    ", NA);
	move(8, 16);
	for (i = 1; i < 7; i++)
		prints("%6d\t", mcEnv->stockPoint[i]);
	move(11, 16);
	for (i = 1; i < 7; i++)
		prints("%6d\t", mcEnv->tradeNum[i] / 100);
	showAt(14, 0, "    \033[1;33m一周收盘\033[0m    ", NA);
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
{				//试图将一支新股票加入空位，成功返回下标值 
//若股票已存在或没有空位, 返回-1 
	int i, slot = -1;
	struct BoardStock *bs;

	for (i = 0; i < n; i++) {
		bs = stockMem + i * sizeof (struct BoardStock);
		if (!strcmp(bs->boardname, new_bs->boardname) && bs->timeStamp)
			return -1;
		if (bs->timeStamp == 0 && slot == -1)	//放到第一个空位 
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
{				//股票上市 
	int i, money, num;
	float price;
	char boardname[24], buf[256], title[STRLEN];
	struct boardmem *bp;
	struct BoardStock bs;

	make_blist_full();
	move(10, 4);
	namecomplete("请输入将要上市的讨论区: ", boardname);
	FreeNameList();
	if (boardname[0] == '\0') {
		showAt(11, 4, "错误的讨论区名称...", YEA);
		return 0;
	}
	bp = getbcache(boardname);
	if (bp == NULL) {
		showAt(11, 4, "错误的讨论区名称...", YEA);
		return 0;
	}
	while (1) {
		getdata(11, 4, "请输入市值[单位:千万, [1-10]]:", genbuf, 4,
			DOECHO, YEA);
		money = atoi(genbuf);
		if (money >= 1 && money <= 10)
			break;
	}
	price = bp->score / 1000.0;
	money *= 10000000;
	num = money / price;
	sprintf(buf, "    该讨论区将发行 %d %s的股票。\n"
		"    共发行 %d 股, 每股价格 %.2f %s。\n", money, MONEY_NAME, num,
		price, MONEY_NAME);
	showAt(12, 0, buf, NA);
	move(15, 4);
	if (askyn("确定发行吗", NA, NA) == NA)
		return 0;
//初始化 
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
	sprintf(title, "【股市】%s版今日上市", boardname);
	deliverreport(title, buf);
	showAt(16, 4, "操作完成。", YEA);
	mcLog("股票上市，市值", money, boardname);
	mcLog("股票上市，每股", price, boardname);
	return 1;
}

static void
deleteList(short stockid)
{				//删除一支股票的报价队列 
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
			"股票%s因恶意操纵估价被证监会勒令停止本周交易。\n",
			bs->boardname);
	} else if (n < 30) {
		rate = random() % 6 + 5;
		bs->boardscore *= (1 + rate / 100.0);
		sprintf(newsbuf, "股票%s今日在利好消息刺激下股价有望上扬%d%%。\n",
			bs->boardname, rate);
	} else if (n < 50) {
		rate = random() % 6 + 5;
		bs->boardscore *= (1 - rate / 100.0);
		sprintf(newsbuf, "股票%s据传盈利悲观，有分析指股价将下挫%d%%。\n",
			bs->boardname, rate);
	}
	if (n < 50) {
		sprintf(titlebuf, "【股市】%s股市最新消息", CENTER_NAME);
		deliverreport(titlebuf, newsbuf);
		mcLog(newsbuf, 0, "");
	}
}

static void
UpdateBSStatus(struct BoardStock *bs, short today, int newday, int stockid)
{				//更新一支股票的状态 

	int yeste = (today <= 1) ? 6 : (today - 1);
	float delta = bs->todayPrice[3] / bs->weekPrice[yeste];
	struct boardmem *bp;
	char buf[80];

	if (bs->status == 3)
		return;
	if (delta >= 1.1)
		bs->status = 1;	//涨停 
	else if (delta <= 0.9)
		bs->status = 2;	//跌停 
	else
		bs->status = 0;	//恢复正常 
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
			sprintf(buf, "没有找到%s版数据，无法更新股票", bs->boardname);
			deliverreport(buf, "");
		}
	}
}

static int
updateStockEnv(void *stockMem, int n, int flag)
{				//更新股市统计信息 

	int i, hour, newday = 0, totalTradeNum = 0;
	float megaStockNum = 0, megaTotalValue = 0, avgPrice;
	struct BoardStock *bs;
	struct tm thist, lastt;
	time_t currTime = time(NULL);

	if (currTime < mcEnv->lastUpdateStock + 60)	//不用频繁更新 
		return 0;
	localtime_r(&currTime, &thist);
	hour = thist.tm_hour;
	mcEnv->stockTime = (hour >= 9 && hour < 22) && !(hour >= 13
							 && hour < 15)
	    && thist.tm_wday;
	if (!mcEnv->stockTime)	//休市状态下不更新 
		return 0;
	localtime_r(&mcEnv->lastUpdateStock, &lastt);
	mcEnv->lastUpdateStock = currTime;
	if (thist.tm_wday != lastt.tm_wday || flag)	//自动删除昨日交易单 
		newday = 1;
	for (i = 0; i < n; i++) {
		bs = stockMem + i * sizeof (struct BoardStock);
		if (!bs->timeStamp)	//无效股票 
			continue;
		megaStockNum += bs->totalStockNum / 1000000.0;
		megaTotalValue += megaStockNum * bs->todayPrice[3];
		UpdateBSStatus(bs, thist.tm_wday, newday, i);	//更新股票状态 
		totalTradeNum += bs->tradeNum;
	}
// 更新股市指数 
	avgPrice = megaTotalValue / megaStockNum;
//	mcEnv->stockPoint[thist.tm_wday] = avgPrice * n;
	mcEnv->stockPoint[thist.tm_wday] = bbsinfo.utmpshm->mc.ave_score;
// 更新成交量 
	mcEnv->tradeNum[thist.tm_wday] = totalTradeNum;
	return 0;
}

static void
showTradeInfo(void *stockMem, int n)
{				//显示一支股票的当前交易信息 
	int i, j, m, stockid, col, line;
	int buyerNum = 0, sellerNum = 0;
	size_t filesize;
	char listpath[2][256];
	void *buffer = NULL, *listMem;
	struct BoardStock *bs;
	struct TradeRecord *tr;

	money_show_stat("交易信息中心");
	showAt(4, 4, "这里可以查询股票的当前报价信息。", NA);
	if ((stockid = userSelectStock(n, 6, 4)) == -1)
		return;
	bs = stockMem + stockid * sizeof (struct BoardStock);
	sprintf(genbuf, "确定要查询股票 %s 的交易信息么", bs->boardname);
	move(7, 4);
	if (askyn(genbuf, NA, NA) == NA)
		return;
	showAt(9, 0, "    查询结果如下[前10条] ：\n"
	       "    [序号  买入报价    余交易量]  ========  [序号  卖出报价    余交易量]",
	       NA);
	sprintf(listpath[0], "%s%d.buy", DIR_STOCK, stockid);
	sprintf(listpath[1], "%s%d.sell", DIR_STOCK, stockid);
	for (i = 0; i < 2; i++) {
		col = (i == 0) ? 5 : 45;
		m = get_num_records(listpath[i], sizeof (struct TradeRecord));
		if (m <= 0) {
			showAt(11, col, "没有交易报价。", NA);
			continue;
		}
		filesize = sizeof (struct TradeRecord) * m;
		listMem = loadData(listpath[i], buffer, filesize);
		if (listMem == (void *) -1)
			continue;
		for (j = 0, line = 11; j < m && j < 10; j++) {	//仅显示前10 
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
	bs->buyerNum = buyerNum;	//顺便统计买卖家数, 但不准确 
	bs->sellerNum = sellerNum;	//因为只scan了前10个记录 
	sleep(1);
	pressanykey();
}

static void
showStockDetail(struct BoardStock *bs)
{				//显示一支股票的详细信息 
	char strTime[16];
	struct tm *timeStamp = localtime(&(bs->timeStamp));
	char *status[] = { "\033[1;37m正常\033[0m", "\033[1;31m涨停\033[0m",
		"\033[1;32m跌停\033[0m", "\033[1;34m暂停\033[0m"
	};

	sprintf(strTime, "%4d-%2d-%2d", timeStamp->tm_year + 1900,
		timeStamp->tm_mon + 1, timeStamp->tm_mday);
	money_show_stat("交易大厅");
	move(4, 4);
	prints("以下是股票 %s 的详细信息:", bs->boardname);
	move(6, 0);
	prints("      上市日期: %10s\t  发行量: %10d\n"
	       "      系统控股: %10d\t  散户数: %10d\n"
	       "      当前卖家: %10d\t当前买家: %10d\n"
	       "        交易量: %10d\t    状态:   %s\n",
	       strTime, bs->totalStockNum, bs->remainNum,
	       bs->holderNum, bs->sellerNum, bs->buyerNum, bs->tradeNum,
	       status[bs->status]);
	sprintf(genbuf,
		"       今日开盘: %7.2f\t最高: %7.2f\t最低: %7.2f\t平均: %7.2f\n"
		"       历史最高: %7.2f\t最低: %7.2f\n",
		bs->todayPrice[0], bs->todayPrice[1], bs->todayPrice[2],
		bs->todayPrice[3], bs->high, bs->low);
	move(11, 0);
	prints("%s", genbuf);
}

static int
getMyStock(struct mcUserInfo *mcuInfo, int stockid)
{
//按给定的stockid搜索一支股票 
//如果没有该股票, 那么找到一个空位, 初始化 
//返回值是股票的位置下标, 如果已经满了, 那么返回 -1 
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
{				//更新一支股票的历史记录 
	time_t currTime = time(NULL);
	struct tm *thist = localtime(&currTime);

	if (tradeNum == 0)
		return;
	bs->high = MAX_XY(tradePrice, bs->high);
	bs->low = MIN_XY(tradePrice, bs->low);
	if (bs->todayPrice[0] == 0)	//每日开盘价 
		bs->todayPrice[0] = tradePrice;
	bs->todayPrice[1] = MAX_XY(tradePrice, bs->todayPrice[1]);	//每日最高价 
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
{				//将一个待交易单加入队列 
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
	if (n <= 0) {		//直接append_record 
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
		if (slot == -1 && tr->num <= 0)	//找到第一个无效的交易请求 
			slot = i;
	}
	if (slot >= 0) {	//替换 
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
{				//股票交易的核心函数 
	int idx, slot, iCanBuyNum, tradeNum, tradeMoney;

	if (buyer->cash < trsell->price || trbuy->price < trsell->price)
		return 0;	//没钱买或开价不够 
	idx = getMyStock(seller, trsell->stockid);
	if (idx == -1)		//卖方已经没有这支股票了 
		return 0;
	tradeNum = MIN_XY(trbuy->num, trsell->num);
	iCanBuyNum = buyer->cash / trsell->price;
	tradeNum = MIN_XY(tradeNum, iCanBuyNum);
	tradeNum = MIN_XY(tradeNum, seller->stock[idx].num);
	if (tradeNum <= 0)	//上面确定最终双方可以交易的数量 
		return 0;
	tradeMoney = tradeNum * trsell->price;
	if ((slot = getMyStock(buyer, trbuy->stockid)) == -1)
		return 0;
	buyer->cash -= tradeMoney;	//买方扣钱 
	seller->stock[idx].num -= tradeNum;	//卖方持有股数要减 
	if (seller->stock[idx].num <= 0)	//卖方卖完,持股人-- 
		bs->holderNum--;
	if (buyer->stock[slot].num <= 0)	//新买主,持股人++ 
		bs->holderNum++;
	buyer->stock[slot].num += tradeNum;	//买方买入 
	buyer->stock[slot].stockid = trbuy->stockid;
	seller->cash += 0.999 * tradeMoney;	//卖方得钱 
	trsell->num -= tradeNum;	//卖方卖出单数量减少 
	trbuy->num -= tradeNum;	//买方购买需求也要减 
	return tradeNum;
}

static int
tradeWithSys(struct BoardStock *bs, struct TradeRecord *mytr, float sysPrice)
{				//用户与系统交易 
	int sysTradeNum;
	struct mcUserInfo mcuInfo;
	struct TradeRecord systr;

	if (mytr->num <= 0)	//如果没有量 
		return 0;
	bzero(&systr, sizeof (struct TradeRecord));
	if (mytr->tradeType == 0) {	//用户买入 
		systr.price = MAX_XY(mytr->price, sysPrice);
		systr.num = bs->remainNum;
	} else {
		systr.price = MIN_XY(sysPrice, mytr->price);
		systr.num = mytr->num;	//系统购买总是有购买欲望 
	}
//下面构造系统的交易单和交易者 
	systr.stockid = mytr->stockid;
	systr.tradeType = 1 - mytr->tradeType;
	strcpy(systr.userid, "_SYS_");

	bzero(&mcuInfo, sizeof (struct mcUserInfo));
	mcuInfo.cash = 1000000000;	//系统总是有足够的钱 
	mcuInfo.stock[0].stockid = mytr->stockid;
	mcuInfo.stock[0].num = bs->remainNum;	//! 
	if (mytr->tradeType == 0)	//用户买入 
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
{				//信件通知交易对方 
	char content[STRLEN], title[STRLEN];
	char *actionDesc[] = { "买入", "卖出" };
	char *moneyDesc[] = { "支付", "获取" };
	int type = utr->tradeType;

	sprintf(title, "你%s股票 %d 的交易完成", actionDesc[type],
		utr->stockid);
	sprintf(content, "此次交易量为 %d 股, 成交价 %.2f, %s%s %d", tradeNum,
		tradePrice, moneyDesc[type], MONEY_NAME,
		(int) (tradeNum * tradePrice));
	system_mail_buf(content, strlen(content), utr->userid, title,
			currentuser->userid);
}

static int
tradeWithUser(struct BoardStock *bs, struct TradeRecord *mytr)
{				//用户间的交易 
	int i, n, tradeNum, tradeMoney = 0;
	float tradePrice;
	char mcu_path[256], list_path[256];
	void *buffer_mcu = NULL, *buffer_list = NULL, *tradeList;
	struct mcUserInfo *mcuInfo;
	struct TradeRecord *utr;

	if (mytr->tradeType == 0)	//currentuser买入 
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
	for (i = 0; i < n && mytr->num > 0; i++) {	//报价队列 
		utr = tradeList + i * sizeof (struct TradeRecord);
		if (utr->userid[0] == '\0' || utr->num <= 0)
			continue;
		sethomefile(mcu_path, utr->userid, "mc.save");
		mcuInfo =
		    loadData(mcu_path, buffer_mcu, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			continue;
		if (mytr->tradeType == 0)	//currentuser买入 
			tradeNum =
			    tradeArithmetic(bs, mytr, utr, myInfo, mcuInfo);
		else		//currentuser卖出, 相当于对方买入 
			tradeNum =
			    tradeArithmetic(bs, utr, mytr, mcuInfo, myInfo);
		if (tradeNum > 0) {	//更新记录和通知交易方 
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
{				//交易的交互过程 
	int idx, inList = 0;
	int iHaveNum, inputNum, userTradeNum, sysTradeNum;
	int userTradeMoney, sysTradeMoney, totalMoney;
	int yeste = (today <= 1) ? 6 : (today - 1);
	float sup = bs->weekPrice[yeste] * 1.1;
	float inf = bs->weekPrice[yeste] * 0.9;
	float inputPrice, sysPrice, guidePrice;
	char buf[256];
	char *actionDesc[] = { "买入", "卖出", NULL };
	struct TradeRecord mytr;

	idx = getMyStock(myInfo, stockid);
	if (idx == -1 && type == 0) {
		showAt(14, 4, "你已经买了10支其它股票了, 不能再多买了", YEA);
		return 0;
	}
	if (myInfo->stock[idx].num <= 0 && type == 1) {
		showAt(14, 4, "你没有持这支股票", YEA);
		return 0;
	}
	if (bs->status == 3) {
		showAt(14, 4, "这支股票已停止交易", YEA);
		return 0;
	}
	iHaveNum = myInfo->stock[idx].num;
	move(14, 4);
	sprintf(genbuf, "目前您持有 %d 股这支股票, 控股率 %.4f％.",
		iHaveNum, iHaveNum / (bs->totalStockNum / 100.0));
	prints("%s", genbuf);
	sprintf(genbuf, "请输入要%s的股数[100]: ", actionDesc[type]);
	while (1) {
		inputNum = 100;
		getdata(15, 4, genbuf, buf, 8, DOECHO, YEA);
		if (buf[0] == '\0' || (inputNum = atoi(buf)) >= 100)
			break;
	}
	guidePrice = bs->boardscore / 1000.0;
	if (type == 0)		//currentuser买入, 系统要取高价卖 
		sysPrice = MAX_XY(guidePrice, bs->todayPrice[3]);
	else			//currentuser卖出, 系统低价买入 
		sysPrice = MIN_XY(0.95 * guidePrice, bs->todayPrice[3]);
	sprintf(genbuf, "请输入%s报价[%.2f]: ", actionDesc[type], sysPrice);
	while (1) {
		inputPrice = sysPrice;
		getdata(16, 4, genbuf, buf, 7, DOECHO, YEA);
		if (buf[0] != '\0')
			inputPrice = atof(buf);
		if (inputPrice < inf - 0.01 || inputPrice > sup + 0.01) {
			showAt(17, 4, "对不起, 你的报价超过涨停(跌停)价位!",
			       YEA);
		} else
			break;
	}
	sprintf(buf, "确定以 %.2f 的报价%s %d 股的 %s 股票吗?", inputPrice,
		actionDesc[type], inputNum, bs->boardname);
	move(17, 4);
	if (askyn(buf, NA, NA) == NA)
		return 0;
	if (type == 0 && myInfo->cash < inputNum * inputPrice) {
		showAt(18, 4, "你的现金不够此次交易...", YEA);
		return 0;
	}
	if (type == 1 && myInfo->stock[idx].num < inputNum) {
		showAt(18, 4, "你没有这么多股票...", YEA);
		return 0;
	}
//构造我的交易单 
	bzero(&mytr, sizeof (struct TradeRecord));
	mytr.num = inputNum;
	mytr.tradeType = type;
	mytr.price = inputPrice;
	mytr.stockid = stockid;
	strcpy(mytr.userid, currentuser->userid);
//用户间交易 
	userTradeMoney = tradeWithUser(bs, &mytr);
	userTradeNum = abs(inputNum - mytr.num);
	if (userTradeNum > 0) {
		sprintf(buf, "用户%s了 %d 股, 成交价每股 %.2f %s",
			actionDesc[1 - type], userTradeNum,
			1.0 * userTradeMoney / userTradeNum, MONEY_NAME);
		showAt(18, 4, buf, YEA);
	}
//如果还有剩余量未完成, 那么与系统交易 
	sysTradeMoney = tradeWithSys(bs, &mytr, sysPrice);
	sysTradeNum = abs(inputNum - mytr.num - userTradeNum);
	if (sysTradeNum > 0) {
		sprintf(buf, "系统%s了 %d 股, 成交价每股 %.2f %s",
			actionDesc[1 - type], sysTradeNum,
			1.0 * sysTradeMoney / sysTradeNum, MONEY_NAME);
		showAt(19, 4, buf, YEA);
	}
//如果还有较多剩余量未交易, 那么加入交易队列 
	if (mytr.num >= 100)
		inList = addToWaitingList(&mytr, stockid);
	move(20, 4);
	if (mytr.num < inputNum) {	//如果有交易量 
		totalMoney = userTradeMoney + sysTradeMoney;
		prints("交易成功! 共%s %d股, 交易金额 %d %s(含手续费)",
		       actionDesc[type], abs(inputNum - mytr.num), totalMoney,
		       MONEY_NAME);
	}
	if (mytr.num > 0) {	//交易未完成 
		myInfo->stock[idx].status = type + 1;
		move(21, 4);
		prints("剩余 %d 股未交易%s", mytr.num,
		       inList ? ", 已经加入报价队列" : ".");
	}
	pressanykey();
	sleep(1);
	return 0;
}

static int
stockTrade(void *stockMem, int n, int type)
{				//交易大厅界面 
	int i, j, yeste, today, pages, offset, stockid;
	float delta;
	char buf[256];
	struct BoardStock *bs;
	struct tm *thist;
	time_t currTime = time(NULL);
	char *status[] = { "\033[1;37m正常\033[0m", "\033[1;31m涨停\033[0m",
		"\033[1;32m跌停\033[0m", "\033[1;34m暂停\033[0m"
	};

	thist = localtime(&currTime);
	today = thist->tm_wday;
	yeste = (today <= 1) ? 6 : (today - 1);
	move(4, 4);
	prints("目前有 %d 支股票, 昨日收盘指数 %d 点, 当前指数 %d 点  状态: %s",
	       n, mcEnv->stockPoint[yeste], mcEnv->stockPoint[today],
	       mcEnv->stockTime ? "交易中" : "\033[1;31m休市中\033[0m");
	sprintf(buf, "\033[1;36m代号  %20s  %8s  %8s  %8s \t%s\033[0m",
		"版名", "昨日收盘", "今日成交", "涨幅", "状态");
	showAt(6, 4, buf, NA);
	move(7, 4);
	prints
	    ("----------------------------------------------------------------------------------");
	pages = n / 10 + 1;
	for (i = 0;; i++) {	//i用于控制页数 
		for (j = 0; j < 10; j++) {	//每屏显示最多10支股票 
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
		getdata(19, 4, "[B]前页 [C]下页 [T]交易 [Q]退出: [C]", buf, 2,
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
{				//显示个人持有股票 
	int i, count = 0;
	char *status[] = { "持有", "报价买入", "报价卖出", NULL };

	nomoney_show_stat("股市个人服务中心");

	showAt(4, 0, "    以下是你个人持有股票情况: \n"
	       "    [代号]\t\t持  有  量\t\t状  态", NA);
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
		showAt(6, 16, "\033[1;31m股市暂停交易  请稍后再来\033[0m", YEA);
		return 0;
	}
	if (!file_exist(DIR_STOCK "stock"))
		initData(2, DIR_STOCK "stock");
	n = get_num_records(DIR_STOCK "stock", sizeof (struct BoardStock));
	if (n <= 0)
		return 0;
	filesize = sizeof (struct BoardStock) * n;
//加载股市信息 
	stockMem = loadData(DIR_STOCK "stock", buffer, filesize);
	if (stockMem == (void *) -1)
		return -1;
	while (!quit) {
		limit_cpu();
		sprintf(buf, "%s股市", CENTER_NAME);
		money_show_stat(buf);
		showAt(4, 4, "\033[1;33m股票有风险，涨跌难预料。\n"
		       "    入市需谨慎，破产责自负!\033[0m\n\n", NA);
		move(t_lines - 1, 0);
		prints("\033[1;44m 选单 \033[1;46m [1]买入 [2]卖出 [3]信息 "
		       "[4]走势 [5]个人 [9]证监会 [Q]离开\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
		case '2':
			money_show_stat("交易大厅");
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
		if (!mcEnv->stockOpen)	//踢掉呆在里面的人 
			break;
	}
	saveData(stockMem, filesize);
	return 0;
}

// -------------------------   商店   --------------------------  // 
static int
buy_card(char *cardname, char *filepath)
{
	int i;
	char uident[IDLEN + 1], note[3][STRLEN], tmpname[STRLEN];

	clear();
	ansimore2(filepath, 0, 0, 25);
	move(t_lines - 2, 4);
	sprintf(genbuf, "你确定要买贺卡%s么", cardname);
	if (askyn(genbuf, YEA, NA) == NA)
		return 0;
	if (myInfo->cash < 1000) {
		showAt(t_lines - 2, 60, 
			"你的钱不够啊！", YEA);
		return 0;
	}
	myInfo->cash -= 1000;
	mcEnv->Treasury += 1000;
	clear();
	if (!getOkUser("要把这卡片送给谁? ", uident, 0, 0))
		return 0;
	move(2, 0);
	prints("有话要在卡片里说吗？[可以写3行喔]");
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
		fprintf(fp, "\n以下是 %s 的附言:\n", currentuser->userid);
		for (j = 0; j < i; j++)
			fprintf(fp, "%s", note[j]);
		fclose(fp);
	}
	if (mail_file
	    (tmpname, uident, "看看我从商店里挑给你的贺卡，喜欢吗？",
	     currentuser->userid) >= 0) {
		showAt(8, 0, "你的贺卡已经发出去了", YEA);
		mcLog("发送贺卡成功", 0, uident);
	} else {
		showAt(8, 0, "贺卡发送失败，对方信件超容", YEA);
		mcLog("发送贺卡失败", 0, uident);
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
	"    本店贺卡均非本店制作，部分贺卡由于种种原因，未能标明作者。\n"
	"    如贺卡创作者对其作品用于本店持有异议，请与本站系统维护联系\n"
	"    本站将及时根据作者意愿作出调整。\n\n"
	"                    \033[1;32m%s贺卡店\033[0m\n", CENTER_NAME);
	showAt(6, 0, buf, YEA);
	sprintf(buf, "%s贺卡店", CENTER_NAME);
	nomoney_show_stat(buf);
	showAt(4, 4, "本店出售如下类型的贺卡: ", NA);
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
		getdata(16, 4, "请选择类型:", buf, 3, DOECHO, YEA);
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
		"%s 类型的卡片共有 %d 张. 请选择要预览的卡号[ENTER放弃]: ",
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
	sprintf(buf, "%s当铺", CENTER_NAME);
	money_show_stat(buf);
	convertRate = MIN_XY(bbsinfo.utmpshm->mc.ave_score / 1000.0 + 0.1, 10) * 1000;
	sprintf(genbuf, "    您可以通过变卖经验值获得%s。今天的报价是\n"
			"    [1]经验值：%.1f每点\t"
//			"[2]胆识：30000每点\t"
//			"[3]身法：30000每点\n    [4]人品：6000每点\t\t"
			"[Q]退出\t\t请选择：",
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
				showAt(7, 4, "你无法变卖经验值。", YEA);
				quit = 1;
				break;
			}
			sprintf(genbuf, "按现在的经验值,当铺最多给您 %d %s, 还要变卖吗",
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
				prints("交易成功，这里是您的 %d %s。", convertMoney, MONEY_NAME);
				mcLog("卖经验获得", convertMoney, "");
			}else{
				move(8, 4);
				prints("国库没有那么多钱……");
			}
			break;
/*		case '2':
			convertMoney = userInputValue(7, 4, "卖", "点胆识", 1, 100);
			if (convertMoney < 0)	break;
			if (convertMoney > myInfo->robExp) {
				showAt(8, 4, "你没有那么高的胆识！", YEA);
				break;
			}
			if (mcEnv->Treasury < convertMoney * 30000) {
				showAt(8, 4, "国库没有那么多钱……", YEA);
				break;
			}
			myInfo->Actived++;
			myInfo->robExp -= convertMoney;
			myInfo->cash += convertMoney * 30000;
			mcEnv->Treasury -= convertMoney * 30000;
			showAt(9, 4, "交易成功！", YEA);
			break;
		case '3':
                        convertMoney = userInputValue(7, 4, "卖", "点身法", 1, 100);
			if (convertMoney < 0)   break;
			if (convertMoney > myInfo->begExp) {
				showAt(8, 4, "你没有那么高的身法！", YEA);
				break;
			}
			if (mcEnv->Treasury < convertMoney * 30000) {
				showAt(8, 4, "国库没有那么多钱……", YEA);
				break;
			}
			myInfo->Actived++;
			myInfo->begExp -= convertMoney;
			myInfo->cash += convertMoney * 30000;
			mcEnv->Treasury -= convertMoney * 30000;
			showAt(9, 4, "交易成功！", YEA);
			break;
		case '4':
                        convertMoney = userInputValue(7, 4, "卖", "点人品", 1, 100);
			if (convertMoney < 0)   break;
			if (convertMoney > myInfo->luck + 100) {
				showAt(8, 4, "你没有那么好的人品！", YEA);
				break;
			}
			if (mcEnv->Treasury < convertMoney * 6000) {
				showAt(8, 4, "国库没有那么多钱……", YEA);
				break;
			}
			myInfo->Actived++;
			myInfo->luck -= convertMoney;
			myInfo->cash += convertMoney * 6000;
			mcEnv->Treasury -= convertMoney * 6000;
			showAt(9, 4, "交易成功！", YEA);
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

	nomoney_show_stat("猛兽园");
	move(4, 4);
	prints
	    ("%s猛兽园出售纯种大狼狗给有需要保护的人士，每只10000%s。",
	     CENTER_NAME, MONEY_NAME);
#if 0
	if (myInfo->cash < 100000) {
		showAt(8, 4, "你还是省省吧?没人会打你的主意的。", YEA);
		return;
	}
#endif
	guard_num = (myInfo->robExp / 100) + 1;
	guard_num = (guard_num > 8) ? 8 : guard_num;
	if (myInfo->guard >= guard_num) {
		showAt(8, 4, "你已经有足够的大狼狗了啊。不用这么胆小吧？", YEA);
		return;
	}

	move(6, 4);
	prints("按照您目前的身份地位，再买%d只大狼狗就够了。",
	       guard_num - myInfo->guard);
	guard_num =
	    userInputValue(7, 4, "买", "只大狼狗", 1,
			   guard_num - myInfo->guard);
	if (guard_num == -1) {
		showAt(8, 4, "哦，您不想买了啊。。。那欢迎下次再来！", YEA);
		return;
	}

	if (myInfo->cash < guard_num * 10000) {
		showAt(8, 4, "不好意思，您的钱不够，慢走不送～", YEA);
		return;
	}
	myInfo->cash -= guard_num * 10000;
	mcEnv->prize777 += after_tax(guard_num * 10000);
	myInfo->guard += guard_num;
	move(8, 4);
	showAt(9, 4, "买大狼狗成功,你可以有一段时间安享太平了。", YEA);
	mcLog("买大狼狗", guard_num, "只");
	return;
}

static int
money_shop()
{
	int ch, i, quit = 0, bonusExp = 0;
	char buf[256], uident[IDLEN + 1];

	while (!quit) {
		sprintf(buf, "%s百货公司", CENTER_NAME);
		nomoney_show_stat(buf);
		move(6, 4);
		prints("%s商场最近生意红火，大家尽兴！", CENTER_NAME);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m 选项 \033[1;46m [1]道具 [2]贺卡 [3]当铺 [4]商场经理办公室 [Q]离开\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			money_show_stat("商场购物柜台");
			move(5, 0);
			prints("\033[1;42m编号 物品    价  格 功  能  简  介             \033[m\n"
					 "  1  大狼狗  1万/只 减少被拍砖可能\n"
					 "  2  肉包子  1万/个 随机增加体力1～5点\n"
					 "  3  平衡器 10万/次 平衡胆识和身法）\n"
					 "  4  菩提子 20万/个 随机增加胆识和身法各1～10点\n"
					 "  5  跑步机  5万/次 增加25次活动次数\n"
					 "  6  时光机 10万/次 提前下次活动时间10分钟"
#if 0
			       "好运丸（使用对象：自己。作用：短期加运气）\n"
			       "厄运丹（使用对象：可指定。作用：成功后可短期减其运气）\n"
			       "长筒袜 (使用对象：自己。作用：短期加胆识)\n"
			       "偷拍机（使用对象：可指定。作用：成功后可减其胆识）\n"
			       "自行车（使用对象：自己。作用：短期加身法)\n"
			       "西瓜皮（使用对象：可指定。作用：成功后可短期减其身法）"
#endif
			    );

			move(t_lines - 2, 0);
#if 0
			prints
			    ("\033[1;44m 您要购买 \033[1;46m [1]大狼狗 [2]肉包子 [3]好运丸 [4]厄运丹\n"
			     "\033[1;44m          \033[1;46m [5]长筒袜 [6]偷拍机 [7]自行车 [8]西瓜皮 [Q]离开\033[m");
#else
			prints
			    ("\033[1;44m 您要购买 \033[1;46m [1] 大狼狗 [2] 肉包子 [3] 平衡器         \n"
			     "\033[1;44m          \033[1;46m [4] 菩提子 [5] 跑步机 [6] 时光机 [Q] 离开\033[m");
#endif
			ch = igetkey();
			switch (ch) {
			case '1':
				update_health();
				if (check_health(1, 13, 4, "体力不够了！", YEA))
					break;
				myInfo->health--;
				myInfo->Actived++;
				buydog();
				break;
			case '2':
				bonusExp = userInputValue(13, 4, "要吃","个肉包子", 1, 20);
				if (bonusExp < 0)
					break;
				if (myInfo->cash < bonusExp * 10000) {
					showAt(15, 4, "你没带那么多现金！", YEA);
					break;
				}
				myInfo->cash -= bonusExp * 10000;
				mcEnv->Treasury += bonusExp * 10000;
				update_health();
				for (i = 0; i < bonusExp; i++)
					myInfo->health += random() % 5 + 1;
				myInfo->Actived++;
				showAt(15, 4, "\033[1;32m你的体力增加了！\033[m", YEA);
				mcLog("买", bonusExp, "个肉包子");
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
				if (askyn("平衡器100,000元一个，确实要购买吗？", NA, NA)==NA)
					break;
				if (check_health(1, 14, 4, "你的体力不够了！", YEA))
					break;
				myInfo->health--;
				myInfo->Actived += 10;
				if (myInfo->cash < 100000) {
					showAt(15, 4, "你没带那么多现金！", YEA);
					break;
				}
				myInfo->cash -= 100000;
				mcEnv->Treasury += 100000;
				myInfo->robExp = (myInfo->robExp + myInfo->begExp) / 2;
				myInfo->begExp = myInfo->robExp;
				showAt(16, 4, "转换成功！", YEA);
				mcLog("买平衡器，胆识身法均为", myInfo->robExp, "");
				break;
			case '4':
				update_health();
				move(13, 4);
				if (askyn("菩提子200,000元一个，确实要购买吗？", NA, NA)==NA)
					break;
				if (check_health(20, 14, 4, "你的体力不够了！", YEA))
					break;
				myInfo->health -= 20;
				myInfo->Actived += 20;
				if (myInfo->cash < 200000) {
					showAt(15, 4, "你没带够现金。", YEA);
					break;
				}
				myInfo->cash -= 200000;
				mcEnv->Treasury += 200000;
				bonusExp = random()% 10 + 1;
				myInfo->robExp += bonusExp;
				myInfo->begExp += bonusExp;
				move(16, 4);
				prints("\033[1;31m恭喜！你的身法和胆识各增加了%d点！！", bonusExp);
				mcLog("吃了菩提子，胆识身法各增加", bonusExp, "点");
				pressanykey();
				break;
			case '5':
				update_health();
				move(13, 4);
				if (askyn("跑步机50,000元一个，确实要购买吗？", NA, NA) == NA)
					break;
				if (check_health(10, 14, 4, "你的体力不够了！", YEA))
					break;
				myInfo->health -= 10;
				if (myInfo->cash < 50000) {
					showAt(14, 4, "你没带够现金。", YEA);
					break;
				}
				myInfo->cash -= 50000;
				mcEnv->Treasury += 50000;
				myInfo->Actived += 25;
				myInfo->luck -= 5;
				showAt(15, 4, "\033[1;32m你的活动次数增加了25点！", NA);
				showAt(16, 4, "你的人品下降5点！！\033[m", YEA);
				mcLog("使用跑步机", 0, "");
				break;
			case '6':
				update_health();
				move(13, 4);
				if (askyn("时光机100,000元一个，确实要购买吗？", NA, NA) == NA)
					break;
				if (check_health(20, 14, 4, "你的体力不够了！", YEA))
					break;
				myInfo->health -= 20;
				if (myInfo->cash < 100000) {
					showAt(14, 4, "你没带够现金。", YEA);
					break;
				}
				myInfo->cash -= 100000;
				mcEnv->Treasury += 100000;
				myInfo->lastActiveTime -= 600;
				myInfo->luck -= 5;
				myInfo->Actived += 10;
				showAt(15, 4, "\033[1;33m你下次活动时间提前了10分钟！", NA);
				showAt(16, 4, "你的人品下降5点！！\033[m", YEA);
				mcLog("使用时空机", 0, "");
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
			money_show_stat("当铺柜台");
			update_health();
			if (check_health(1, 12, 4, "体力不够了！", YEA))
				break;
			myInfo->health--;
			shop_sellExp();
			break;
		case '4':
			money_show_stat("商场经理办公室");
			whoTakeCharge(7, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			while (!quit) {
				nomoney_show_stat("商场业务");
				update_health();
				if (check_health
				    (1, 12, 4, "歇会歇会，体力不够了！", YEA))
					break;
				move(t_lines - 1, 0);
				prints
				    ("\033[1;44m 选单 \033[1;46m [1]采购 [2]定价 [3]查看商店资产 [Q]离开\033[m");
				ch = igetkey();
				switch (ch) {
				case '1':
					nomoney_show_stat("采购部");
					showAt(12, 4,
					       "\033[1;32m规划中。\033[m", YEA);
					break;
				case '2':
					money_show_stat("市场部");
					showAt(12, 4,
					       "\033[1;32m规划中。\033[m", YEA);
					break;
				case '3':
					money_show_stat("小金库");
					showAt(12, 4,
					       "\033[1;32m会计正在点钱，请稍候。\033[m",
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

//  ---------------------------   警署     --------------------------  // 

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
	prints("如果您遭遇抢劫或偷窃，如果您有任何线索，请向警方报告。");
	move(5, 4);
	prints("\033[1;32m警民合作，共创安定大好局面！(举报费10000元)\033[0m");
	if (myInfo->cash < 10000) {
		showAt(7, 4, "你没有带够现金。", YEA);
		return 0;
	}
	if (!getOkUser("举报谁？", uident, 7, 4))
		return 0;
	if (seek_in_file(POLICE, uident)) {
		showAt(8, 4, "大胆！想诬陷警务人员吗？！", YEA);
		return 0;
	}
	award = userInputValue(8, 4, "你出资", MONEY_NAME "捉拿他",
			10000, myInfo->cash);
	if (award < 0) {
		showAt(10, 4, "你不想举报他了。", YEA);
		return 0;
	}
	move(10, 4);
	if (askyn("\033[1;33m你向警方提供的上述信息真实吗\033[0m", NA, NA) ==
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
		showAt(8, 4, "这个人最近很安分啊！你不要诽谤别人哦！", YEA);
		goto UNMAP;
	}
	if (mcuInfo->freeTime > 0) {
		showAt(11, 4, "这个人已经被警署监禁了。", YEA);
		goto UNMAP;
	}
	mcLog("举报", award, uident);
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
	showAt(11, 4, "警方非常感谢您提供的线索，我们将尽力尽快破案。", YEA);
	return 1;

      UNMAP:saveData(mcuInfo, 1);
	return 0;
}

static int
cop_arrange(int type)
{
	int found;
	char uident[IDLEN + 1], buf[STRLEN], title[STRLEN];
	char *actionDesc[] = { "-", "招募", "解职", NULL };
	void *buffer = NULL;
	struct mcUserInfo *mcuInfo;

	if (!getOkUser("请输入ID: ", uident, 12, 4))
		return 0;
	found = seek_in_file(POLICE, uident);
	move(13, 4);
	if (type == 1 && found) {
		showAt(13, 4, "该ID已经是警员了。", YEA);
		return 0;
	} else if (type == 2 && !found) {
		showAt(13, 4, "该ID不是警署警员。", YEA);
		return 0;
	}
	if (type == 1 && 
		(seek_in_file(ROBUNION, currentuser->userid)
			|| seek_in_file(BEGGAR, currentuser->userid))) {
		showAt(13, 4, "此人社会关系不明，不宜雇用为警员。", YEA);
		return 0;
	}

	sprintf(buf, "%s确定%s吗？",
		type == 2 ? "被解雇警员会损失一半胆识跟身法，" 
		          : "你需要花费100000元招募警员，",
		actionDesc[type]);
	if (askyn(buf, NA, NA) == NA)
		return 0;
	if (type == 1) {
		if (myInfo->cash < 100000) {
			showAt(14, 4, "你没有足够的现金招募警员！", YEA);
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
	sprintf(title, "【警署】%s警员 %s", actionDesc[type], uident);
	sprintf(buf, "    警署署长%s %s警员 %s\n", currentuser->userid,
		actionDesc[type], uident);
	deliverreport(title, buf);
	system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
	showAt(14, 4, "操作成功。", YEA);
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
		showAt(12, 4, "警察执行任务，闲杂人等请回避，以免误伤。", YEA);
		return;
	}
	if (seek_in_file(ROBUNION, currentuser->userid)
		|| seek_in_file(BEGGAR, currentuser->userid)) {
		showAt(12, 4, "《无间道》看多了吧你？", YEA);
		return;
	}

	if (myInfo->robExp < 50 || myInfo->begExp < 50) {
		showAt(12, 4, "新警察吧？先去练练基本功，不然就是送死了。",
		       YEA);
		return;
	}
	if (check_health(60, 12, 4, "你没有充沛的体力来执行任务。", YEA))
		return;

	currtime = time(NULL);
	if (currtime < 3600 + myInfo->lastActiveTime) {
		showAt(12, 4, "不要着急，上级还没有行动指示。", YEA);
		return;
	}

	move(4, 4);
	prints("刚刚接到上级指示，马上要进行一次抓捕行动。");

	if (!getOkUser("\n请选择你的目标：", uident, 6, 4)) {
		move(8, 4);
		prints("查无此人");
		pressanykey();
		return;
	}
	if (!strcmp(uident, currentuser->userid)) {
		showAt(8, 4, "牛魔王：“老婆～快来看神经病啦～”", YEA);
		return;
	}
	if (!seek_in_file(CRIMINAL, uident)) {
		showAt(8, 4, "此人不在通缉名单上面。”", YEA);
		return;
	}
	myInfo->Actived += 5;
	move(10, 4);
	prints("你埋伏在大富翁世界的必经之路，等待目标的出现。");

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
		showAt(12, 4, "你等了整整一天，目标还是没有出现，只好放弃了。",
		       YEA);
		mcLog("警察没发现罪犯", 0, uident);
		return;
	}

	if (!(random() % 3)) {
		myInfo->health -= 10;
		showAt(12, 4, "你眼看老远处人影一晃，目标得到风声跑掉了。",
		       YEA);
		mcLog("警察没追到罪犯", 0, uident);
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
		mcLog("警察抓住罪犯，没收现金", mcuInfo->cash, uident);
		mcuInfo->cash = 0;
		mcuInfo->begExp = MAX_XY(0, mcuInfo->begExp - 10);
		mcuInfo->robExp = MAX_XY(0, mcuInfo->robExp - 10);
		mcuInfo->luck = MAX_XY(-100, mcuInfo->luck - 10);
		mcuInfo->freeTime = currtime + 7200;

		myInfo->luck = MIN_XY(100, myInfo->luck + 10);
		del_from_file(CRIMINAL, uident);
		prints("\n    远处有人走了过来，你定睛一看，正是你要抓的%s。"
		       "\n    你跳出来拔出手枪大吼到：“不许动！你被捕了！”"
		       "\n    %s目瞪口呆，只好束手就擒。"
		       "\n    你的胆识增加了！"
		       "\n    你的身法增加了！"
		       "\n    你的人品增加了！"
		       "\n    没收罪犯所有现金做为补助！", uident, uident);
		sprintf(title, "【警署】警方抓获通缉犯%s", uident);
		sprintf(buf,
			"    警员%s耐心守候，孤身勇擒在榜通缉犯%s，特此公告，以示表彰。\n",
			currentuser->userid, uident);
		deliverreport(title, buf);
		sprintf(buf,
			"你被警员%s设伏抓获，被关禁闭2小时。没收所有现金，胆识身法下降。",
			currentuser->userid);
		system_mail_buf(buf, strlen(buf), uident, "你被警方抓获",
				currentuser->userid);
		saveData(mcuInfo, sizeof (struct mcUserInfo));
		pressreturn();
	} else {
		myInfo->begExp -= 5;
		myInfo->robExp -= 5;
		myInfo->health = 0;
		myInfo->freeTime = currtime + 3600;
		prints("\n    远处有人走了过来，你定睛一看，正是你要抓的%s。"
		       "\n    你跳出来拔出手枪大吼到：“不许动！你被捕了！”"
		       "\n    正得意时，不料背后有人打了你一个闷棍。"
		       "\n    你的胆识减少了！"
		       "\n    你的身法减少了！" "\n    你昏了过去！", uident);
		sprintf(title, "【警署】警员%s受伤住院", currentuser->userid);
		sprintf(buf,
			"警员%s在执行任务的时候，被罪犯打伤住院。目前情况稳"
			"定，不久即可出院。\n", currentuser->userid);
		deliverreport(title, buf);
		sprintf(buf,
			"条子%s设伏抓你，还好你艺高人胆大将其干掉，自己安然"
			"无恙。", currentuser->userid);
		system_mail_buf(buf, strlen(buf), uident, "你金蝉脱壳",
				currentuser->userid);
		mcLog("警察被罪犯打伤住院", 0, uident);
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
		sprintf(buf, "%s警署", CENTER_NAME);
		nomoney_show_stat(buf);
		move(8, 16);
		prints("打击犯罪，维持治安！");
		move(10, 4);
		prints("近期犯罪活动大大增加，刑警们也开始日以继夜的加班。");
		move(t_lines - 1, 0);
		prints("\033[1;44m 选单 \033[1;46m [1]报案 [2]通缉榜"
		       " [3]刑警队 [4] 资助警方 [5]署长办公室 [Q]离开\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			nomoney_show_stat("警署接待厅");
			cop_accuse();
			break;
		case '2':
			clear();
			move(1, 0);
			prints("%s警署当前通缉的犯罪嫌疑人:\n"
				"\033[1;32m罪犯名单     \033[33m奖金\033[m", CENTER_NAME);
			listfilecontent(CRIMINAL);
			FreeNameList();
			pressanykey();
			break;
		case '3':
			nomoney_show_stat("刑警队");
//showAt(6, 4, "接到上级命令,行动被取消...", YEA); 
			cop_Arrest();
			break;
		case '4':
			money_show_stat("警署出纳室");
			if (myInfo->cash < 10000) {
				showAt(6, 4, "你带的现金太少了，cmft", YEA);
				break;
			}
			num =userInputValue(6, 4, "资助警方", "万" MONEY_NAME, 
				1, myInfo->cash / 10000);
			if (num <= 0)
				break;
			myInfo->cash -= num * 10000;
			mcEnv->Treasury += num * 10000;
			myInfo->luck = MIN_XY(100, myInfo->luck + num);
			showAt(8, 4, "警署收到了您的捐款，谢谢！", YEA);
			mcLog("资助警方", num, "万元");
			if (num == 10 &&
				(!seek_in_file(POLICE, currentuser->userid))) {
				move(9, 4);
				if(askyn("你要加入警察队伍吗？", NA, NA) == NA)
					break;
				if(seek_in_file(BEGGAR, currentuser->userid)) {
					showAt(10, 4, "你已经是丐帮成员了！\n", NA);
					if(askyn("    你要退出丐帮吗？", YEA, NA) == NA)
						break;
					del_from_file(BEGGAR, currentuser->userid);
					mcLog("退出丐帮", 0, "");
					showAt(12, 4, "你已经退出丐帮了。", YEA);
				}
				if(seek_in_file(ROBUNION, currentuser->userid)) {
					showAt(10, 4, "你已经是黑帮成员了！    \n", NA);
					if(askyn("    你要退出黑帮吗？", YEA, NA) == NA)
						break;
					del_from_file(ROBUNION, currentuser->userid);
					mcLog("退出黑帮", 0, "");
					showAt(12, 4, "你已经退出黑帮了。", YEA);
				}
				addtofile(POLICE, currentuser->userid);
				showAt(14, 4, "你已经加入了警察队伍！", YEA);
				sprintf(title, "【警署】%s成为警察", currentuser->userid);
				sprintf(buf, "    被通缉的人小心，他可能会随时逮捕你！\n");
				deliverreport(title, buf);
				mcLog("成为警察", 0, "");
				break;
			}				
			if (num < 100)
				break;
			move(16, 0);
			if (askyn("    您想保释罪犯吗？", NA, NA) == NA)
				break;
			if (!getOkUser("请输入ID: ", uident, 17, 4))
	                	break;
		        if (!seek_in_file(CRIMINAL, uident)) {
				prints("    %s 并不在通缉榜上。", uident);
				break;
			}
			del_from_file(CRIMINAL, uident);
			showAt(18, 4, "你已经成功得保释了他！", YEA);
			sprintf(title, "【警署】%s被保释", uident);
			if (!strcmp(currentuser->userid, uident)) {
				sprintf(buf, "    %s 花了 %d 万 %s 保释了自己。\n",
					currentuser->userid, num, MONEY_NAME);
				deliverreport(title, buf);
			} else {
				sprintf(buf, "    %s 花了 %d 万 %s 保释了 %s。\n",
					currentuser->userid, num, MONEY_NAME, uident);
				deliverreport(title, buf);
				sprintf(title, "你被 %s 保释", currentuser->userid);
				sprintf(buf, "    %s 花了 %d 万 %s 将你保释出来。",
					currentuser->userid, num, MONEY_NAME);
                		system_mail_buf(buf, strlen(buf), uident, title,
						currentuser->userid);
			}
			mcLog("保释罪犯，花费", num, uident);
			break;
		case '5':
			nomoney_show_stat("署长办公室");
			whoTakeCharge(8, uident);
			if (strcmp(currentuser->userid, uident))
				break;
			showAt(6, 0, "    请选择操作代号:\n"
			       "        1. 招募警员         2. 解职警员\n"
			       "        3. 警员名单         Q. 退出",
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
				prints("目前%s警署警员名单：", CENTER_NAME);
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

//  --------------------------    管理    ------------------------  // 
static void
special_MoneyGive()
{
	int money;
	char uident[IDLEN + 1], reason[256], buf[256], title[STRLEN];
	struct mcUserInfo *mcuInfo;
	void *buffer = NULL;

	money_show_stat("特别拨款小金库");
	if (!getOkUser("你拨款给谁？", uident, 16, 4)) {
		move(17, 4);
		prints("查无此人");
		pressanykey();
		return;
	}
	sethomefile(buf, uident, "mc.save");
	if (!file_exist(buf))
		initData(1, buf);
	money = userInputValue(17, 4, "拨", "万", 10, 1000);
	if (money < 0)
		return;
	getdata(19, 4, "理由：", reason, 40, DOECHO, YEA);

        if (reason[0] == '\0' || reason[0] == ' '){
                showAt(20, 4, "没有理由不许私自拨款！", YEA);
                return;
        }
	move(20, 4);
	if (askyn("你确定要拨款吗？", YEA, NA) == YEA) {
		mcuInfo = loadData(buf, buffer, sizeof (struct mcUserInfo));
		if (mcuInfo == (void *) -1)
			return;
		mcuInfo->credit += 10000 * money;
		mcEnv->Treasury -= 10000 * money;
		myInfo->Actived++;
		sprintf(title, "总管%s特别拨款%d万给%s", currentuser->userid,
			money, uident);
		sprintf(buf, "    总管%s特别拨款%d万给%s \n理由：%s\n",
			currentuser->userid, money, uident, reason);
		system_mail_buf(buf, strlen(buf), uident, title,
				currentuser->userid);
		deliverreport(title, buf);

		saveData(mcuInfo, sizeof (struct mcUserInfo));
		showAt(21, 4, "拨款成功", YEA);
		mcLog("拨款", money * 10000, uident);
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
		"职位: [0.总管 1.银行 2.彩票 3.赌场 4.黑帮 5.丐帮 6.股市 7.商场 8.警署 ]",
		buf, 2, DOECHO, YEA);
	pos = atoi(buf);
	if (pos > 8 || pos < 0)
		return;
	whoTakeCharge(pos, boss);
	move(17, 4);
	if (boss[0] != '\0' && type == 0) {
		prints("%s已经负责该职位。", boss);
		pressanykey();
		return;
	}
	if (boss[0] == '\0' && type == 1) {
		showAt(17, 4, "目前并无人负责该职位。", YEA);
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
		("您的代号：\033[1;33m%s\033[m      胆识 \033[1;33m%d\033[m  身法 \033[1;33m%d\033[m  人品 \033[1;33m%d\033[m  体力 \033[1;33m%d\033[m \n",
		 currentuser->userid, myInfo->robExp, myInfo->begExp, myInfo->luck,myInfo->health);
		prints
		("\033[1;32m欢迎光临%s大富翁世界，当前位置是\033[0m \033[1;33m大富翁世界管理\033[0m", CENTER_NAME);
		move(3, 0);
		prints
		("\033[1m--------------------------------------------------------------------------------\033[m");
					
		sprintf(buf, "    这里负责%s大富翁世界的人事管理。\n"
			     "    现任总管是: \033[1;32m%s\033[m\n"
			     "    国库储备金: \033[1;33m%15Ld\033[m %s"
			     "    用户总财产: \033[1;32m%15Ld\033[m %s\n",
		             CENTER_NAME, admin, mcEnv->Treasury, MONEY_NAME, 
			     mcEnv->AllUserMoney, MONEY_NAME);
		showAt(5, 0, buf, NA);
		mcLog("总管查看", 0, buf);
		sprintf(buf, "    大富翁用户总人数: \033[1;32m%9d\033[m 人    "
			     "    用户人均财富: \033[1;33m%13Ld\033[m %s",
			     mcEnv->userNum, mcEnv->userNum==0?
			     0:(mcEnv->AllUserMoney/mcEnv->userNum), MONEY_NAME);
		showAt(8, 0, buf, NA);
		mcLog("总管查看", 0, buf);
		showAt(10, 0,
		       "        1. 任命职位                    2. 免去职位\n"
		       "        3. 列出职位名单                4. 关闭/开启大富翁世界\n"
		       "        5. 特别拨款                    6. 发行货币\n"
		       "        7. 设定坏人名单                8. 销毁货币\n"
		       "        9. 更新用户总财产              0. 查询用户\n"
		       "        Q. 退出", NA);
		ch = igetkey();
		switch (ch) {
		case '1':
			promotion(0, "任命谁？");
			break;
		case '2':
			promotion(1, "免去谁？");
			break;
		case '3':
			clear();
			move(1, 0);
			prints("目前%s大富翁世界各职位情况：", CENTER_NAME);
			listfilecontent(MC_BOSS_FILE);
			FreeNameList();
			pressanykey();
			break;
		case '4':
			move(16, 4);
			if (mcEnv->closed)
				sprintf(buf, "%s", "确定要开启么");
			else
				sprintf(buf, "%s", "确定要关闭么");
			if (askyn(buf, NA, NA) == NA)
				break;
			mcEnv->closed = !mcEnv->closed;
			if (!mcEnv->closed)
				mcEnv->openTime = time(NULL);
			showAt(17, 4, "操作成功", YEA);
			mcLog(mcEnv->closed?"关闭大富翁世界":"开启大富翁世界", 0, "");
			break;
		case '5':
			special_MoneyGive();
			break;
		case '6':
			move(17, 4);
			if (askyn("确实要发行一千万货币吗？", NA, NA) == NA)
				break;
			mcEnv->Treasury += 10000000;
			move(18, 4);
			prints("你已经印了一千万%s并注入了国库。", MONEY_NAME);
			sprintf(title, "【国库】%s总管%s向国库注入了10000000%s",
					CENTER_NAME, currentuser->userid, MONEY_NAME);
			sprintf(buf, "    为缓解经济危机，总管%s批准印钞10000000%s\n",
					currentuser->userid, MONEY_NAME);
			deliverreport(title, buf);
			mcLog("发行", 10000000, "货币");
			break;
		case '7':
			banID();
			break;
		case '8':
			move(17, 4);
			if (askyn("确实要销毁一千万货币吗？", NA, NA) == NA)
				break;
			mcEnv->Treasury -= 10000000;
			move(18, 4);
			prints("你已经销毁了一千万残破%s。", MONEY_NAME);
			sprintf(title, "【国库】%s总管%s销毁了10000000残破%s",
					CENTER_NAME, currentuser->userid, MONEY_NAME);
			sprintf(buf, "    总管%s销毁了10000000残破的%s\n",
					currentuser->userid, MONEY_NAME);
			deliverreport(title, buf);
			mcLog("销毁", 10000000, "残破货币");
			break;
		case '9':
			showAt(17, 4, "更新中……", NA);
			refresh();
			all_user_money();
			showAt(18, 4, "更新完成！", YEA);
			break;
		case '0':
			clear();
			if (!getOkUser("查询谁的情况？", uident, 0, 0))
				break;
			myInfo->Actived++;
			sethomefile(buf, uident, "mc.save");
			if (!file_exist(buf))
				initData(1, buf);
			mcuInfo = loadData(buf, buffer,	sizeof (struct mcUserInfo));
			if (mcuInfo == (void *) -1)
				break;
			prints("版本：%12d\tmutex：%11d\ntemp[0]：%9d\ttemp[1]：%9d\n",
				mcuInfo->version, mcuInfo->mutex, mcuInfo->temp[0], mcuInfo->temp[1]);
			prints("现金：%12d\t存款：%12d\n贷款：%12d\t利息：%12d\t保险箱：%10d\n",
				mcuInfo->cash, mcuInfo->credit, mcuInfo->loan, mcuInfo->interest, mcuInfo->safemoney);
			prints("胆识：%12d\tpoliceExp：%7d\t身法：%12d\n已卖经验值：%6d\t活动次数：%8d\n",
				mcuInfo->robExp, mcuInfo->policeExp, mcuInfo->begExp, mcuInfo->soldExp, mcuInfo->Actived);
			prints("体力：%12d\t人品：%12d\n大狼狗：%10d\t免死金牌：%8d\n",
				mcuInfo->health, mcuInfo->luck, mcuInfo->guard, mcuInfo->antiban);
			prints("当前时间：      %s", ctime(&mcuInfo->aliveTime));
			prints("出狱/出院时间： %s", ctime(&mcuInfo->freeTime));
			prints("贷款时间：      %s", ctime(&mcuInfo->loanTime));
			prints("还贷时间：      %s", ctime(&mcuInfo->backTime));
			prints("上次存款时间：  %s", ctime(&mcuInfo->depositTime));
			prints("上次领工资时间：%s", ctime(&mcuInfo->lastSalary));
			prints("上次活动时间：  %s", ctime(&mcuInfo->lastActiveTime));
			prints("上次藏钱时间：  %s", ctime(&mcuInfo->secureTime));
			prints("保险到期时间：  %s", ctime(&mcuInfo->insuranceTime));
			mcLog("查询",1,uident);
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
	
        if (!getOkUser("请输入ID: ", uident, 16, 4))
		return;
	found = seek_in_file(BADID, uident);
	if (found) {
		sprintf(buf, "%s已在坏人名单，要将其释放吗？", uident);
		move(17, 4);
		if (askyn(buf, NA, NA) == NA)
			return;
		del_from_file(BADID, uident);
		sprintf(title, "【总管】允许%s进入大富翁世界", uident);
		sprintf(buf, "    鉴于%s已有改悔之心，大富翁总管%s允许他重新进入大富翁世界。\n", 
				uident, currentuser->userid);
		deliverreport(title, buf);
		system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
		showAt(18, 4, "操作成功。", YEA);
		mcLog("允许该ID进入大富翁世界", 0, uident);
		return;
	}else{
		sprintf(buf, "你确定将%s加入坏人名单吗？他的所有指数将被清零！", uident);
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
			prints("%s持有免死金牌%d块，本次操作减去1块。", uident, mcuInfo->antiban);
			mcuInfo->antiban--;
			mcLog("禁止该ID进入大富翁世界。他有免死金牌", mcuInfo->antiban, uident);
			saveData(mcuInfo, sizeof (struct mcUserInfo));
			sprintf(title, "【总管】%s使用了一块免死金牌", uident);
			sprintf(buf,
				"    由于%s扰乱大富翁世界金融秩序，"
				"被总管%s取消进入大富翁世界的资格。\n"
				"    由于他拥有免死金牌，这次免于刑罚，免死金牌数量减一。\n",
				uident, currentuser->userid);
			deliverreport(title, buf);
			system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
			showAt(19, 4, "他用了免死金牌。", YEA);
			return;
		}
		mcuZero(uident);
		addtofile(BADID, uident);
		sprintf(title, "【总管】大富翁总管%s取消%s进入大富翁世界的资格",
				currentuser->userid, uident);
		sprintf(buf, "    由于%s扰乱大富翁世界金融秩序，被取消进入大富翁世界的资格。\n"
			     "    其所有指数已被清零。如果想重新进入大富翁世界，请与总管联系。\n",
			      uident);
		deliverreport(title, buf);
		system_mail_buf(buf, strlen(buf), uident, title, currentuser->userid);
		showAt(18, 4, "操作成功。", YEA);
		mcLog("禁止该ID进入大富翁世界，他的指数均被清零", 0, uident);
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
	money_show_stat("幕后黑手办公室");
	move(6, 4);
	if(askyn("免死金牌1000万一块，你要买吗？", NA, NA) == NA)
		return;
	if(myInfo->cash < 10000000) {
		showAt(7, 4, "你没带够现金！", YEA);
		return;
	}
	num = userInputValue(7, 4, "买","块免死金牌",1, myInfo->cash / 10000000);
	if (num < 1) {
		showAt(9, 4, "你不买了。", YEA);
		return;
	}
	myInfo->cash -= num * 10000000;
	mcEnv->Treasury += num * 10000000;
	myInfo->antiban += num;
	mcLog("买免死金牌", num, "块");
	showAt(10, 4, "交易成功！", YEA);
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
		nomoney_show_stat("大富翁世界中央大厅");
		move(6, 4);
		prints
		    ("这里是%s大富翁世界的中央大厅，可以通往各地。人来人往很是热闹。"
		     "\n    据说最近还经常有人在这里遇上神秘事件。。。",
		     CENTER_NAME);
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m 选单 \033[1;46m [1] 大富翁排行榜 [2] 个人状态 [3] 免死金牌 [4] 总管办公室 [Q] 离开\033[m");
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
	{ "总管", "银行行长", "彩票经理", "赌场老板", "黑帮老大",
		"丐帮帮主", "证监会经理", "商店经理", "警察局长"};
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
		sprintf(title, "【国库】%s总管销毁了5000000残破%s",
				CENTER_NAME, MONEY_NAME);
		sprintf(buf, "    总管销毁了5000000残破的%s\n", MONEY_NAME);
		deliverreport(title, buf);
		mcLog("销毁", 5000000, "残破货币");
	}
	if (mcEnv->Treasury < -2500000) {
		mcEnv->Treasury += 5000000;
		sprintf(title, "【国库】%s总管向国库注入了5000000%s",
				CENTER_NAME, MONEY_NAME);
		sprintf(buf, "    为缓解经济危机，总管批准印钞5000000%s\n",
				MONEY_NAME);
		deliverreport(title, buf);
		mcLog("发行", 5000000, "货币");
	}
	clear();
	update_health();
	money_show_stat("个人状态");
	move(5, 0);
	prints("    你的保险箱有：%11d %s\n", 
			myInfo->safemoney, MONEY_NAME);
	prints("    你的贷款：%15d %s\n", 
			myInfo->loan, MONEY_NAME);
	userSalary = makeSalary();
	prints("    你的工资：%15d %s\n", 
		userSalary, MONEY_NAME);
	convertRate = MIN_XY(bbsinfo.utmpshm->mc.ave_score / 1000.0 + 0.1, 10) * 1000;
	sellexpmoney = (countexp(currentuser) - myInfo->soldExp) * convertRate;
	prints("    你卖经验可得：%11d %s\n",
		sellexpmoney, MONEY_NAME);
	prints("    777奖金库有：%12d %s\n",
		mcEnv->prize777, MONEY_NAME);
	sprintf(buf, "    国库金额：%15Ld %s\n",
		mcEnv->Treasury, MONEY_NAME);
	prints(buf);
	prints("    你属于：  ");
	if (seek_in_file(ROBUNION, currentuser->userid))
		prints("黑帮分子 ");
	if (seek_in_file(BEGGAR, currentuser->userid))
		prints("丐帮弟子 ");
	if (seek_in_file(POLICE, currentuser->userid))
		prints("警察 ");
	prints("\n");
	prints("    你的职务：");
	for (pos = 0; pos < 9; pos++) {
		whoTakeCharge(pos, boss);
		if (!strcmp(boss, currentuser->userid))
			prints("%s ", feaStr[pos]);
	}
	prints("\n");
	prints("    你有免死金牌：%11d 块\n", myInfo->antiban);
	prints("    你的活动次数：%11d 次\n", myInfo->Actived);
	prints("    当前系统时间是：  %s",
		ctime(&myInfo->aliveTime));
	if (userSalary > 0)
		prints("    下次发工资的时间：%s",
			ctime(&mcEnv->salaryEnd));
	if (myInfo->loan > 0)
		prints("    你的贷款到期时间：%s",
			ctime(&myInfo->backTime));
	prints("    你上次藏钱的时间：%s",
		ctime(&myInfo->secureTime));
	prints("    你的保险到期时间：%s",
		ctime(&myInfo->insuranceTime));
	prints("    你上次行动的时间：%s",
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
		money_show_stat("抽奖处");
		move (4, 0);
		prints ("如果您玩游戏进入高手排行榜，可以在这里抽奖。一次上榜记录只能抽一次奖。");
		move (t_lines - 1, 0);
		prints
		    ("\033[1;37;44m 选单 \033[1;46m [1]推箱子 [2]扫雷 [3]感应式扫雷 [4]俄罗斯方块 [5]打字 [Q]离开\033[m");
		ch = igetkey();
		switch (ch) {
		case '1':
			choujiang(MY_BBS_HOME "/etc/worker/worker.rec", "推箱子");
			break;
		case '2':
			choujiang(MY_BBS_HOME "/etc/winmine/mine2.rec", "扫雷");
			break;
		case '3':
			choujiang(MY_BBS_HOME "/etc/winmine2/mine3.rec",
				  "感应式扫雷");
			break;
		case '4':
			choujiang(MY_BBS_HOME "/etc/tetris/tetris.rec",
				  "俄罗斯方块");
			break;
		case '5':
			choujiang(MY_BBS_HOME "/etc/tt/tt.dat", "打字练习");
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
	if (!strcmp(gametype, "推箱子")){
		for (n = 0; n <= 19; n++){
			fscanf(fp, "%s %d %d %s %s\n",
				topID[n], &topS[n], &topT[n], topFROM[n], prize[n]);
			if (!strcmp(currentuser->userid, topID[n])
				&& !strcmp(prize[n], "未")){
				chou = n + 1;
			}
		}
	}else{
		for (n = 0; n <= 19; n++) {
			fscanf(fp, "%s %d %s %s\n", topID[n], &topT[n], topFROM[n],
		       		prize[n]);
			if (!strcmp(currentuser->userid, topID[n])
			    && !strcmp(prize[n], "未")) {
				chou = n + 1;
			}
		}
	}
	fclose(fp);
	
	if (chou == 0) {
		showAt(6, 4, "\033[1;37m您目前无法抽奖\033[m", YEA);
		return;
	}
	move(6, 4);
	prints("\033[1;37m您是 %s 第 %d 名\033[m", gametype, chou);
	move(7, 4);
	if (askyn("要抽奖吗？", YEA, NA) == YEA) {
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
			strcpy(prize[chou-1], "抽");
			myInfo->Actived += 2;
			sprintf(buf, "\033[1;32m您抽中了 %d %s，扣税后实得 %d %s\033[m", 
				jiang, MONEY_NAME, jiang - tax, MONEY_NAME);
			showAt(8, 4, buf, YEA);
			sprintf(title, "【抽奖】%s 抽中了 %d %s", 
				currentuser->userid, jiang, MONEY_NAME);
			sprintf(buf, "    %s 玩 %s 得了第 %d 名，\n抽奖抽到 %d %s，\n"
					"    扣税后实得 %d %s\n",
				currentuser->userid, gametype, chou, jiang, MONEY_NAME, 
				jiang - tax, MONEY_NAME);
			deliverreport(title, buf);
			mcLog("抽奖获得", jiang, gametype);
		}else{
			showAt(8, 4, "奖金库没钱了！等会再抽吧。", YEA);
		}
		fp = fopen(recfile, "w");
		if (!strcmp(gametype, "推箱子")){
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
		prints("\033[1;32m您不想抽奖了\033[m");
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
		if(askyn("你确实要退出黑帮吗？", NA, NA) == NA)
			return;
		del_from_file(ROBUNION, currentuser->userid);
		showAt(8, 4, "你已经退出了黑帮，要好好做人啊！", YEA);
		sprintf(title, "【黑帮】%s退出黑帮", currentuser->userid);
		sprintf(content, "    他已经金盆洗手了。\n");
		break;
	case 1:
		if(askyn("你确实要退出丐帮吗？", NA, NA) == NA)
			return;
		del_from_file(BEGGAR, currentuser->userid);
		showAt(8, 4, "你已经退出了丐帮，要好好做人啊！", YEA);
		sprintf(title, "【丐帮】%s退出丐帮", currentuser->userid);
		sprintf(content, "    他已经衣锦还乡了。\n");
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

//先买定义每次游戏花费MONKEY_UNIT，开始后出现3行5列的图案。
//共有5根线：(0,0)-(0,1)-(0,2)-(0,3)-(0,4)，(0,0)-(1,1)-(2,2)-(1,3)-(0,4)
//           (1,0)-(1,1)-(1,2)-(1,3)-(1,4)
//           (2,0)-(1,1)-(0,2)-(1,3)-(2,4)，(2,0)-(2,1)-(2,2)-(2,3)-(2,4)
//如果前3，4，5个是一样的图案，就会奖励，赔率在DIR_MC "monkey_rate"
//前12个数是4类东西3，4，5个的赔率，供打印游戏规则用
//不同图案赔率不一样，○可以代替任何图案，人和猴子不参与这些组合。
//如果前3，4，5列出现人，获得10次免费游戏。
//免费游戏中如果第三列出现猴子，台面奖金×(人数目-1)
//免费游戏中出现的猴子数目最多只能是8，5，4次
static void
monkey_business(void)
{
	int i,j,MONKEY_UNIT, TempMoney=0, FreeGame=0, total=0;
	int quit=0, multi=0, man=0, monkey=1;
	char ch,
	     pic[12][2] ={"Ａ","Ｋ","Ｑ","Ｊ","10","象",
		          "蛇","鸟","狮","○","人","猴"};
	int pattern[3][5], sum[5]={12345,12345,12345,12345,12345}, winrate[100012];
	FILE *fp;

	MONKEY_UNIT = userInputValue(6, 4, "每次游戏花费",MONEY_NAME, 200, 10000);
	if(MONKEY_UNIT < 0) {
		showAt(8, 4, "不玩别捣乱！", YEA);
		return;
	}
	mcLog("进入MONKEY，每次花费", MONKEY_UNIT, "");
	if (myInfo->cash < MONKEY_UNIT) {
		showAt(8, 4, "没钱就别赌了。", YEA);
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
		prints("      --== 游戏规则 ==--      |\n");
		prints("                              |\n");
		prints("在以下5条线中：               |\n");
		prints("(1,1)\033[1;31m-\033[0;37m(1,2)\033[1;31m-\033[0;37m(1,3)"
			"\033[1;31m-\033[0;37m(1,4)\033[1;31m-\033[0;37m(1,5) |\033[m\n");
		prints("     \033[1;32m╲\033[0;37m   \033[1;35m╱\033[0;37m     "
			"\033[1;35m╲\033[0;37m   \033[1;32m╱\033[0;37m      |\033[m\n");
		prints("(2,1)\033[1;33m-\033[0;37m(2,2)\033[1;33m-\033[0;37m(2,3)"
			"\033[1;33m-\033[0;37m(2,4)\033[1;33m-\033[0;37m(2,5) |\033[m\n");
		prints("     \033[1;35m╱\033[0;37m   \033[1;32m╲\033[0;37m     "
			"\033[1;32m╱\033[0;37m   \033[1;35m╲\033[0;37m      |\033[m\n");
		prints("(3,1)\033[1;36m-\033[0;37m(3,2)\033[1;36m-\033[0;37m(3,3)"
			"\033[1;36m-\033[0;37m(3,4)\033[1;36m-\033[0;37m(3,5) |\033[m\n");
		prints("如果前3，前4，前5个图案相同： |\n");
		prints("奖金(倍):  \033[1;44;37m 3\033[40m \033[46;37m 4\033[40m "
			"\033[44;37m 5\033[0;40;37m           |\033[m\n");
		prints("\033[1mＡＫＱＪ10\033[32m \033[44;32m%2d\033[40m \033[46;32m%2d"
			"\033[40m \033[44;32m%2d\033[0;40;37m           |\033[m\n",
			winrate[0], winrate[1], winrate[2]);
		prints("\033[1m     象 蛇\033[33m \033[44;33m%2d\033[40m \033[46;33m%2d"
			"\033[40m \033[44;33m%2d\033[0;40;37m           |\033[m\n",
			winrate[3], winrate[4], winrate[5]);
		prints("\033[1m        鸟\033[35m \033[44;35m%2d\033[40m \033[46;35m%2d"
			"\033[40m \033[44;35m%2d\033[0;40;37m "
			"\033[5;1;31m○\033[0;37m可以代替|\033[m\n",
			winrate[6], winrate[7], winrate[8]);
		prints("\033[1m     狮 ○\033[31m \033[44;31m%2d\033[40m \033[46;31m%2d"
			"\033[40m \033[44;31m%2d\033[0;40;37m 任何图案。|\033[m\n",
			winrate[9], winrate[10], winrate[11]);
		prints("如果前3，前4，前5列出现\033[1;33m人\033[0;37m，则 |\033[m\n");
		prints("可获得10次免费游戏机会，此时如|\n");
		prints("果第3列出现\033[1;33m猴子\033[0;37m，则台面筹码分 |\033[m\n");
		prints("别×2，×3，×4。             |\n");
		
		move(5, 34);
		prints("筹码：%d\t玩一次需要：%d", myInfo->cash, MONKEY_UNIT);
		if (FreeGame > 0) {
			move(6, 34);
			prints("免费次数：%d\t台面筹码：%d", FreeGame, TempMoney);
		}
		
		showAt(7, 34, "按空格键开始游戏。按\033[1;32mQ\033[m退出。", NA);

		ch=igetkey();

		if(myInfo->cash < MONKEY_UNIT) {
			showAt(8, 34, "你没钱了，只好退出游戏。", YEA);
			ch = 'q';
		}
		if(myInfo->health == 0)	{
			showAt(9, 34, "你没体力了。只好退出游戏。", YEA);
			ch = 'q';
		}
		switch(ch) {
		case 'q':
		case 'Q':
			mcLog("退出MONKEY，赢了",TempMoney,"");
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
				prints("你赢了 %d %s", total,  MONEY_NAME);
			
			if(FreeGame > 0) {
				TempMoney += total;
				mcEnv->prize777 -= total;
				mcLog("玩MONKEY.FreeGame赢了", total, "");
				multi = 0;
				for(i=0;i<3;i++)
					if(pattern[i][2]==11)
						multi = 1;
				if(multi) {
					mcLog("MONKEY.FreeGame台面有", TempMoney, "");
					mcEnv->prize777 -= TempMoney * (man - 2);
					TempMoney *= man - 1;
					monkey--;
					move(16, 34);
					prints("出现了猴子，你台面的奖金×%d。",
							man - 1);
					mcLog("出现猴子，台面钱×", man-1, "");
				}
				FreeGame--;
				if(FreeGame == 0) {
					TempMoney = after_tax(TempMoney);
					mcLog("MONKEY.FreeGame共赢得", TempMoney, "");
					mcLog("MONKEY.FreeGame共出现", 9 - man - man/4 - monkey,"只猴子");
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
					prints("出现了%d个人，你获得10次免费游戏机会。\n",
							man);
					mcLog("MONKEY中出现", man, "个人，10次FreeGame");
					move(17, 34);
					prints("免费游戏中如果出现猴子，"
						"你台面的奖金将会×%d！", man-1);
				}
				total = after_tax(total - MONKEY_UNIT);
				myInfo->cash += total;
				mcEnv->prize777 -= total;
				mcLog("玩MONKEY赢了", total, "");
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
	nomoney_show_stat("警官巡视");
	move(5, 8);
	prints("=============== 警 官 巡 视 ============");
	move(8, 0);
	prints
		("      最近赌场劫案频发，所以经常会有警官进来巡视，盘查可疑目标。"
		 "\n  啊呀，不好！警官向你走了过来。");
	pressanykey();
	move(10, 6);
	if (random() % 3)
		prints
			("还好还好，警官目不斜视的从你身边走过去了。");
	else {
		prints
			("警官上下打量了你一眼，慢条斯理的说：“需要身份确认！”");
		getdata(11, 6, "请输入密码: ",
				passbuf, PASSLEN, NOECHO, YEA);
		if (passbuf[0] == '\0' || passbuf[0] == '\n'
				|| !checkpasswd(currentuser->passwd,
					currentuser->salt, passbuf)) {
			getdata(13, 6, "密码错误！请再次输入(最后一次机会): ",
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
			("\n      警官点点头：“嗯，没错，继续玩吧。”");
		} else {
			prints
				("\n      警官怒吼：“身份验证失败！跟我走一趟！”"
				 "\n      555，你被没收所有现金，并且被赶出大富翁世界。");
			mcLog("玩777被警察赶走，损失", myInfo->cash, "现金");
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
		money_show_stat("糊涂良民生财之路...");
		move(4, 4);
		prints("\033[1;31m开动脑筋赚钱啊~~~\033[m");
		move(5, 4);
		prints("最小压 100 糊涂币，上限999");
		move(t_lines - 1, 0);
		prints
		("\033[1;44m 选单 \033[1;46m [1]下注 [Q]离开                                                \033[m");
		ch = igetkey();
		switch (ch) {
			case '1':
				win = 0;
				getdata(8, 4, "您压多少糊涂币？[999]", genbuf, 4,
						DOECHO, YEA);
				num = atoi(genbuf);
				if (!genbuf[0])
					num = 999;
				if (num < 100) {
					move(9, 4);
					prints("有没有钱啊？那么点钱我们不带玩的");
					pressanykey();
					break;
				}
				sprintf(genbuf,
						"您压了了 \033[1;31m%d\033[m 糊涂币，确定么？",
						num);
				move(9, 4);
				if (askyn(genbuf, YEA, NA) == YEA) {
					money = load_money(currentuser.userid);
					if (money < num) {
						move(11, 4);
						prints("去去去，没那么多钱捣什么乱");
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
								("请输入四个不重复的数字");
							getdata(11, 4,
									"请猜[q - 退出] → ",
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
									("输入数字有问题!!");
								pressanykey();
								move(12, 4);
								prints
									("                ");
							}
						} while (!genbuf[0]);
						move(count + 13, 0);
						prints("  第 %2d 次： %s  ->  %dA %dB ",
								count, genbuf, an(genbuf, ans),
								bn(genbuf, ans));
						if (an(genbuf, ans) == 4)
							break;
					}
					move(12, 4);
					if (count > 6) {
						sprintf(genbuf,
								"你输了呦！正确答案是 %s，下次再加油吧!!",
								ans);
						sprintf(genbuf,
								"\033[1;31m可怜没猜到，输了 %d 元！\033[m",
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
									"恭喜！总共猜了 %d 次，净赚奖金 %d 元",
									count, num);
							save_money(currentuser.userid,
									num);
							win = 1;
						} else if (num - oldmoney == 0)
							sprintf(genbuf,
									"唉～～总共猜了 %d 次，没输没赢！",
									count);
						else {
							utmpshm->allprize += oldmoney;
							if (utmpshm->allprize >
									1000000000)
								utmpshm->allprize =
									1000000000;
							sprintf(genbuf,
									"啊～～总共猜了 %d 次，赔钱 %d 元！",
									count,
									oldmoney - money);
							save_money(currentuser.userid,
									-num);
						}
					}
					prints("结果: %s", genbuf);
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
