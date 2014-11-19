#include "bbs.h"
#include "bbstelnet.h"
#define ALL_TEST_FILE          MY_BBS_HOME "/etc/friendshipTest.list"
#define TOPTENHOT_FILE         MY_BBS_HOME "/etc/friendshipTest.top10"
#define NEW_IN_FILE            MY_BBS_HOME "/etc/friendshipTest.newin"
#define MAX_LINE_LENGTH        128
#define QUESTION_START_FLAG    "#"
#define OPTION_START_FLAG      "$"
#define ANSWER_START_FLAG      "@"
#define VALUE_START_FLAG       "+"
#define HINT_START_FLAG        "?"
#define ERROR_CODE             -1
#define NEED_REPEAT             1
#define ABORT_ALL               2
#define PERSONAL                1
#define ALL_TESTER              0

struct TOP_TEN_UNIT {
	char name[IDLEN + 1];
	int value;
	time_t time;
};

static void welcomePage();
static void pageProfile();
static int createTest();
static int joinTest();
static int cancelTest();
static int dealWithQuestionInput(FILE * fp, int qCount, int isModify,
				 int _qType, char _question[STRLEN],
				 char _option[4][STRLEN], char _hint[STRLEN],
				 char _ans[STRLEN], int _value);
static int showTopTen(int showType, char *top10File, char *userid);
static int showNextQuestion(FILE * fp, int qCount, int *score,
			    int canSeeAnswer, int allowError);
static int sortTopTen(char *top10File, struct TOP_TEN_UNIT recordToAdd);
static int countValue(char *ans, char *buf, int value);
static int addToNewIn();
static int showNewIn();
static int topTenList();
static int isInTopTen(char *top10File, char *userid);
static int cmp_record(const void *a, const void *b);
static int mail_top10_file(char *my_top10_path);
static int management();
static int modify_question();
static int manage_top10();
static int show_adage();

int
friendshipTest()
{
	int ch;
	int quit = 0;

	modify_user_mode(FRIENDTEST);
	welcomePage();
	while (!quit) {
		clear();
		pageProfile();
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]新建 [2]加入 [3]TopTen [4]查看所有 [5]管理 [Q]离开\033[0m");
		ch = igetkey();
		switch (ch) {
		case '1':
			createTest();
			break;
		case '2':
			joinTest();
			break;
		case '3':
			topTenList();
			break;
		case '4':
			listfilecontent(ALL_TEST_FILE);
			FreeNameList();
			pressanykey();
			break;
		case '5':
			management();
			break;
		case 'q':
		case 'Q':
			quit = 1;
		}
	}
	return 0;
}

static void
welcomePage()
{
	clear();
	move(4, 4);
	prints("快来测试你和朋友的友谊！");
	move(6, 4);
	prints
	    ("\033[1;31m你了解朋友多少？\033[0m  \033[1;31m谁是你最好的朋友？\033[0m");
	move(8, 4);
	prints("\033[1;33m答案尽在友谊测试！\033[0m");
	// print heart-like logo
	move(12, 40);
	prints
	    ("      \033[36m  .\033[1;35m★\033[32m*\033[33m★\033[35m.   \033[m");
	move(13, 40);
	prints
	    (" \033[1;36m.\033[32m*\033[34m★ \033[32m*\033[31m.\033[34m* \033[0;37m   \033[1;36m★\033[0;37m   \033[m");
	move(14, 40);
	prints("\033[1;31m★\033[0;37m           \033[1;32m*    \033[m");
	move(15, 40);
	prints("\033[1;32m★\033[0;37m          \033[1;33m.’   \033[m");
	move(16, 40);
	prints
	    ("\033[1;35m‘\033[34m*\033[32m. \033[0;37m  　  \033[1;35m.      \033[m");
	move(17, 40);
	prints("    \033[1;33m‘  \033[36m. ．      \033[m");
	pressanykey();
}

static void
pageProfile()
{
	move(1, 30);
	prints("\033[1;32m欢迎加入友谊测试\033[0m");
	move(2, 0);
	prints
	    ("\033[1;37m--------------------------------------------------------------------------------\033[0m");
}

static int
topTenList()
{
	char ch;
	int quit = 0;

	while (!quit) {
		clear();
		pageProfile();
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]最强人气TopTen [2]闪亮登场TopTen [Q]离开\033[0m");
		ch = igetkey();
		switch (ch) {
		case '1':
			showTopTen(ALL_TESTER, TOPTENHOT_FILE, NULL);
			break;
		case '2':
			showNewIn();
			break;
		case 'q':
		case 'Q':
			quit = 1;
		}
	}
	return 0;
}

static int
management()
{
	char ch;
	int quit = 0;

	while (!quit) {
		clear();
		pageProfile();
		move(t_lines - 1, 0);
		prints
		    ("\033[1;44m 选单 \033[1;46m [1]修改 [2]取消 [3]TopTen [Q]离开\033[0m");
		ch = igetkey();
		switch (ch) {
		case '1':
			modify_question();
			break;
		case '2':
			cancelTest();
			break;
		case '3':
			manage_top10();
			break;
		case 'q':
		case 'Q':
			quit = 1;
		}
	}
	return 0;
}

static int
createTest()
{
	char temp_path[256], home_path[256];
	char buf[STRLEN];
	int qNum, passwd = 0, canSeeAnswer = 0, canSeeTop10 = 0, allowError = 0;
	FILE *fp;
	int i;

	clear();
	pageProfile();
	move(4, 4);

	sethomefile(home_path, currentuser->userid, "friendshipTest.qest");
	if (file_exist(home_path)) {
		prints("您已经建立了测试。如果要新建，请取消当前测试先。");
		pressanykey();
		return 0;
	}
	if (askyn("要建立自己的友谊测试吗？", NA, NA) == NA) {
		return 0;
	}
	while (1) {
		getdata(5, 4, "问题数[5-20]：", buf, 3, DOECHO, YEA);
		qNum = atoi(buf);
		if (qNum >= 5 && qNum <= 20) {
			break;
		}
	}
	move(6, 4);
	if (askyn
	    ("进入测试是否需要密码[\033[1;33m若选择，密码将寄回邮箱\033[0m]？",
	     NA, NA) == YEA) {
		srandom(time(0));
		passwd = 100000 + random() % 900000;
		sprintf(genbuf,
			"您的友谊测试的密码是：%d。您可以将此信件转发给您的好友。",
			passwd);
		system_mail_buf(genbuf, strlen(genbuf), currentuser->userid, "我的友谊测试密码", currentuser->userid);
	}
	move(7, 4);
	if (askyn("是否公开答案？", NA, NA) == YEA) {
		canSeeAnswer = 1;
	}
	move(8, 4);
	if (askyn("是否公开本测试的排行榜？", YEA, NA) == YEA) {
		canSeeTop10 = 1;
	}
	move(9, 4);
	if (askyn("问答比照是否采用\033[1;32m容错模式\033[0m？", YEA, NA) ==
	    YEA) {
		allowError = 1;
	}
	move(11, 4);
	sethomefile(temp_path, currentuser->userid, "friendshipTest.tmp");
	fp = fopen(temp_path, "w");
	if (fp == NULL) {
		prints("\033[1;31m意外错误！请重试或报告问题。\033[0m");
		pressanykey();
		return ERROR_CODE;
	}
	fprintf(fp, "%d %d %d %d %d\n", qNum, passwd,
		canSeeAnswer, canSeeTop10, allowError);
	prints("\033[1;33m初始化完成，下面开始输入测试问题。\033[0m");
	pressanykey();

	for (i = 0; i < qNum; i++) {
		int retv;
		while ((retv =
			dealWithQuestionInput(fp, i + 1, 0, 0, NULL, NULL, NULL,
					      NULL, 0)) == NEED_REPEAT) {
			;	//do nothing
		}
		if (retv == ABORT_ALL) {
			move(20, 4);
			if (askyn
			    ("\033[1;31m真的要放弃建立测试吗？\033[0m", NA,
			     NA) == NA) {
				i--;
			} else {
				fclose(fp);
				unlink(temp_path);
				return 0;
			}
		}
	}
	fclose(fp);
	clear();
	pageProfile();
	move(4, 4);
	if (askyn("\033[1;32m终于完毕了，发布你的测试吗？\033[0m", YEA, NA) ==
	    YEA) {
		rename(temp_path, home_path);
		addToNewIn();
		addtofile(ALL_TEST_FILE, currentuser->userid);
		move(6, 4);
		prints("\033[1;36m好了，赶快去邀请你的朋友来测试吧！\033[0m");
		pressanykey();
	} else {
		unlink(temp_path);
		move(6, 4);
		prints("......");
		pressanykey();
	}
	return 0;
}

static int
dealWithQuestionInput(FILE * fp, int qCount, int isModify, int _qType,
		      char _question[STRLEN], char _option[4][STRLEN],
		      char _hint[STRLEN], char _ans[STRLEN], int _value)
{
	int qType;
	char option[4][STRLEN];
	char buf[STRLEN], question[STRLEN], hint[STRLEN];
	char ans;
	int value;

	clear();
	pageProfile();
	if (isModify) {
		int i;
		move(4, 4);
		prints("\033[1;36m回顾第 %d 个问题:\033[0m", qCount);
		move(5, 4);
		if (_qType == 'B')
			strcpy(buf, "是非");
		else if (_qType == 'Q')
			strcpy(buf, "问答");
		else
			strcpy(buf, "选择");
		prints
		    ("\033[1;32m问题类型:\033[0m%4s    \033[1;32m分值:\033[0m%2d",
		     buf, _value);
		move(6, 4);
		prints("\033[1;31m问题描述:\033[0m%s", _question + 5);
		if (_qType == 'B') {
			move(8, 8);
			prints("A. 是");
			move(9, 8);
			prints("B. 否");
		} else if (_qType == 'S') {
			for (i = 0; i < 4; i++) {
				move(8 + i, 8);
				prints("%s", _option[i] + 1);
			}
		}
		move(12, 4);
		prints("\033[1;33m提    示:\033[0m%s", _hint + 1);
		move(13, 4);
		prints("\033[1;35m答    案:\033[0m%s", _ans + 1);
		pressanykey();
		move(20, 4);
		if (askyn("\033[1;36m确定要修改本题吗?\033[0m", NA, NA) == NA)
			return 2;
		clear();
		pageProfile();
	}
	move(4, 4);
	prints("\033[1;36m%s第 %d 个问题：\033[0m",
	       isModify ? "修改" : "请完成", qCount);
	while (1) {
		getdata(5, 4,
			"\033[1;32m请选择问题类型[1. 是非  2. 选择  3. 问答]：\033[0m",
			buf, 2, DOECHO, YEA);
		qType = atoi(buf);
		if (qType >= 1 && qType <= 3) {
			break;
		}
	}
	while (1) {
		getdata(6, 4, "\033[1;32m问题描述：\033[0m", question, STRLEN,
			DOECHO, YEA);
		if (question[0] != '\0') {
			break;
		}
	}
	while (1) {
		getdata(7, 4, "\033[1;32m计分[5-20]：\033[0m", buf, 3, DOECHO,
			YEA);
		value = atoi(buf);
		if (value >= 5 && value <= 20) {
			break;
		}
	}
	getdata(8, 4, "问题提示[按\033[1;36mENTER\033[0m略过]：", hint, STRLEN,
		DOECHO, YEA);
	switch (qType) {
	case 1:		//是非                       
		while (1) {
			getdata(9, 4, "\033[1;31m答案[A. 是 B. 否]：\033[0m",
				buf, 2, DOECHO, YEA);
			ans = toupper(buf[0]);
			if (ans == 'A' || ans == 'B') {
				break;
			}
		}
		getdata(10, 4,
			"\033[1;32m[S]保存本题 [E]重新编辑本题 [A]放弃所有 [S]：\033[0m",
			buf, 2, DOECHO, YEA);
		if (buf[0] == 'e' || buf[0] == 'E') {
			return NEED_REPEAT;
		}
		if (buf[0] == 'a' || buf[0] == 'A') {
			return ABORT_ALL;
		}
		fprintf(fp, "%s%c%-2d.%s\n", QUESTION_START_FLAG, 'B', qCount,
			question);
		fprintf(fp, "%s%d\n", VALUE_START_FLAG, value);
		fprintf(fp, "%sA. 是\n%sB. 否\n", OPTION_START_FLAG,
			OPTION_START_FLAG);
		fprintf(fp, "%s%s\n", HINT_START_FLAG, hint);
		fprintf(fp, "%s%c\n", ANSWER_START_FLAG, ans);
		pressanykey();
		return 0;
	case 2:		//选择
		move(9, 4);
		prints("\033[1;32m请填写选项：\033[0m");
		{
			int i;
			for (i = 0; i < 4; i++) {
				while (1) {
					sprintf(genbuf, "%c. ", 'A' + i);
					getdata(10 + i, 8, genbuf, option[i],
						STRLEN, DOECHO, YEA);
					if (option[i][0] != '\0') {
						break;
					}
				}
			}
		}
		while (1) {
			getdata(14, 4, "\033[1;31m答案\033[0m[A B C D]：", buf,
				2, DOECHO, YEA);
			ans = toupper(buf[0]);
			if (ans == 'A' || ans == 'B' || ans == 'C'
			    || ans == 'D') {
				break;
			}
		}

		getdata(15, 4,
			"\033[1;32m[S]保存本题 [E]重新编辑本题 [A]放弃所有 [S]：\033[0m",
			buf, 2, DOECHO, YEA);
		if (buf[0] == 'e' || buf[0] == 'E') {
			return NEED_REPEAT;
		}
		if (buf[0] == 'a' || buf[0] == 'A') {
			return ABORT_ALL;
		}
		fprintf(fp, "%s%c%-2d.%s\n", QUESTION_START_FLAG, 'S', qCount,
			question);
		fprintf(fp, "%s%d\n", VALUE_START_FLAG, value);
		fprintf(fp, "%sA. %s\n%sB. %s\n%sC. %s\n%sD. %s\n",
			OPTION_START_FLAG, option[0], OPTION_START_FLAG,
			option[1], OPTION_START_FLAG, option[2],
			OPTION_START_FLAG, option[3]);
		fprintf(fp, "%s%s\n", HINT_START_FLAG, hint);
		fprintf(fp, "%s%c\n", ANSWER_START_FLAG, ans);
		pressanykey();
		return 0;
	case 3:		//问答                       
		while (1) {
			getdata(9, 4, "\033[1;31m答案：\033[0m", genbuf, STRLEN,
				DOECHO, YEA);
			if (genbuf[0] != '\0') {
				break;
			}
		}
		getdata(10, 4,
			"\033[1;32m[S]保存本题 [E]重新编辑本题 [A]放弃所有 [S]：\033[0m",
			buf, 2, DOECHO, YEA);
		if (buf[0] == 'e' || buf[0] == 'E') {
			return NEED_REPEAT;
		}
		if (buf[0] == 'a' || buf[0] == 'A') {
			return ABORT_ALL;
		}
		fprintf(fp, "%s%c%-2d.%s\n", QUESTION_START_FLAG, 'Q', qCount,
			question);
		fprintf(fp, "%s%d\n", VALUE_START_FLAG, value);
		fprintf(fp, "%s\n", OPTION_START_FLAG);
		fprintf(fp, "%s%s\n", HINT_START_FLAG, hint);
		fprintf(fp, "%s%s\n", ANSWER_START_FLAG, genbuf);
		pressanykey();
		return 0;
	}
	return 0;
}

static int
cancelTest()
{
	char home_path[256], my_top10_path[256];

	clear();
	pageProfile();
	sethomefile(my_top10_path, currentuser->userid, "friendshipTest.top10");
	move(4, 4);
	sethomefile(home_path, currentuser->userid, "friendshipTest.qest");
	if (!file_exist(home_path)) {
		prints("您并没有建立友谊测试。");
		pressanykey();
		return 0;
	}
	if (askyn("\033[5;31m警告：\033[0m确定要取消当前测试吗？", NA, NA) ==
	    YEA) {
		move(5, 4);
		if (askyn("\033[1;32m是否寄回测试和排行榜？\033[0m", NA, NA) ==
		    YEA) {
			system_mail_file(home_path, currentuser->userid,
				  "我的友谊测试(题目)", currentuser->userid);
			mail_top10_file(my_top10_path);
		}
		unlink(home_path);
		unlink(my_top10_path);
		del_from_file(ALL_TEST_FILE, currentuser->userid);
		saveuservalue(currentuser->userid, "friendTest", "0");
		move(7, 4);
		prints("取消测试完成。");
		pressanykey();
	}
	return 0;
}

static int
isInTopTen(char *top10File, char *userid)
{
	FILE *fp;
	struct TOP_TEN_UNIT ttu[10];
	int find = 0;
	int i, n;

	fp = fopen(top10File, "rb");
	if (fp == NULL)
		return 0;
	n = fread(ttu, sizeof (struct TOP_TEN_UNIT), 10, fp);
	fclose(fp);
	for (i = 0; i < n; i++) {
		if (!strcmp(ttu[i].name, userid)) {
			find = 1;
			break;
		}
	}
	return find;
}

static int
joinTest()
{
	int qNum, passwd, canSeeAnswer, canSeeTop10, allowError;
	char home_path[256], top10_path[256];
	char buf[STRLEN], uident[IDLEN + 1];
	FILE *fp;
	struct TOP_TEN_UNIT record;
	int score[2], iCount;
	struct userec *lookupuser;

	clear();
	pageProfile();

	move(4, 4);
	usercomplete("加入谁的测试？", uident);
	if (uident[0] == '\0')
		return 0;
	move(6, 4);
	if (!getuser(uident, &lookupuser)) {
		prints("错误的使用者代号...");
		pressanykey();
		return 0;
	}
	sethomefile(home_path, uident, "friendshipTest.qest");
	if (!file_exist(home_path)) {
		prints("您的朋友没有建立友谊测试。");
		pressanykey();
		return 0;
	}
	fp = fopen(home_path, "r");
	if (fp == NULL) {
		prints("意外错误，请重试或报告问题。");
		pressanykey();
		return ERROR_CODE;
	}
	fscanf(fp, "%d %d %d %d %d\n", &qNum, &passwd, &canSeeAnswer,
	       &canSeeTop10, &allowError);
	if (passwd != 0) {
		getdata(6, 4,
			"\033[1;31m本测试需要密码才能进入。请输入密码：\033[0m",
			buf, 10, NOECHO, YEA);
		if (passwd != atoi(buf)) {
			move(7, 4);
			prints("密码错误!");
			pressanykey();
			fclose(fp);
			return 0;
		}
	}
	sethomefile(top10_path, uident, "friendshipTest.top10");
	if (canSeeTop10) {
		showTopTen(PERSONAL, top10_path, uident);
	}
	move(t_lines - 3, 15);
	if (isInTopTen(top10_path, currentuser->userid)) {
		prints("\033[1;36m您已经榜上有名了,恭喜哦!\033[0m");
		pressanykey();
		fclose(fp);
		return 0;
	}
	if (askyn("\033[1;36m开始答题吗?\033[0m", NA, NA) == NA) {
		fclose(fp);
		return 0;
	}
	for (iCount = 0, score[0] = 0, score[1] = 0; iCount < qNum; iCount++) {
		int retv = showNextQuestion(fp, iCount + 1, score, canSeeAnswer,
					    allowError);
		if (retv != 0)
			break;
	}
	fclose(fp);
	move(20, 4);
	if (iCount == qNum) {
		prints
		    ("\033[1;31m总分：\033[0m%d  \033[1;32m您的得分是：\033[0m%d",
		     score[0], score[1]);
		//人气计数器加1
		if (readuservalue(uident, "friendTest", buf, 10) != 0) {
			sprintf(buf, "%d", 1);
		} else {
			sprintf(buf, "%d", atoi(buf) + 1);
		}
		saveuservalue(uident, "friendTest", buf);
		strcpy(record.name, uident);
		record.value = atoi(buf);
		record.time = time(0);
		sortTopTen(TOPTENHOT_FILE, record);
		//个人测试成绩排名
		strcpy(record.name, currentuser->userid);
		record.value = score[1];
		record.time = time(0);
		sortTopTen(top10_path, record);
	} else {
		prints("\033[1;31m您没有完成所有的测试题目。\033[0m");
	}
	pressanykey();
	return 0;
}

static int
showNextQuestion(FILE * fp, int qCount, int *score, int canSeeAnswer,
		 int allowError)
{
	int qType, iCount, value, q_score = 0;
	char line[MAX_LINE_LENGTH], buf[STRLEN];
	char ans;

	if (qCount % 6 == 0)
		show_adage();
	clear();
	pageProfile();

	if (fgets(line, MAX_LINE_LENGTH, fp) == NULL) {	//问题描述行
		return ERROR_CODE;
	}
	qType = line[1];
	move(4, 4);
	prints("请回答第 %d 个问题：", qCount);
	move(5, 4);

	prints("\033[1;32m问题描述：%s\033[0m", line + 5);	//line+5是问题的开始字符 
	fgets(line, MAX_LINE_LENGTH, fp);	//分值行
	value = atoi(line + 1);	//line+1是分值
	move(6, 4);
	prints("计分：%d", value);
	switch (qType) {
	case 'B':
		for (iCount = 0; iCount < 2; iCount++) {
			fgets(line, MAX_LINE_LENGTH, fp);	//选项行 
			move(7 + iCount, 8);
			prints("%s", line + 1);
		}
		fgets(line, MAX_LINE_LENGTH, fp);	//提示行
		move(9, 4);
		prints("\033[1;35m提示：%s\033[0m", line + 1);
		fgets(line, MAX_LINE_LENGTH, fp);	//答案行
		while (1) {
			getdata(9, 4, "\033[1;31m答案[A B]：\033[0m", buf, 2,
				DOECHO, YEA);
			ans = toupper(buf[0]);
			if (ans == 'A' || ans == 'B') {
				break;
			}
		}

		if (line[1] == ans) {
			q_score = value;
			score[1] += value;
		}
		score[0] += value;
		break;
	case 'S':
		for (iCount = 0; iCount < 4; iCount++) {
			fgets(line, MAX_LINE_LENGTH, fp);	//选项行 
			move(7 + iCount, 8);
			prints("%s", line + 1);
		}
		fgets(line, MAX_LINE_LENGTH, fp);	//提示行
		move(11, 4);
		prints("\033[1;35m提示：%s\033[0m", line + 1);
		fgets(line, MAX_LINE_LENGTH, fp);	//答案行
		while (1) {
			getdata(12, 4, "\033[1;31m答案[A B C D]：\033[0m", buf,
				2, DOECHO, YEA);
			ans = toupper(buf[0]);
			if (ans == 'A' || ans == 'B' || ans == 'C'
			    || ans == 'D') {
				break;
			}
		}
		;
		if (line[1] == ans) {
			q_score = value;
			score[1] += value;
		}
		score[0] += value;
		break;
	case 'Q':
		fgets(line, MAX_LINE_LENGTH, fp);	//选项行
		fgets(line, MAX_LINE_LENGTH, fp);	//提示行
		move(8, 4);
		prints("\033[1;35m提示：%s\033[0m", line + 1);
		fgets(line, MAX_LINE_LENGTH, fp);	//答案行
		while (1) {
			getdata(9, 4, "\033[1;31m答案：\033[0m", buf, STRLEN,
				DOECHO, YEA);
			if (buf[0] != '\0') {
				move(11, 4);
				if (askyn("确定输入吗？", NA, NA) == YEA)
					break;
			}
		}
		line[strlen(line) - 1] = 0;
		if (!allowError) {
			if (!strcmp(buf, line + 1)) {
				q_score = value;
				score[1] += value;
			}
		} else {	//容错模式
			q_score = countValue(line + 1, buf, value);
			score[1] += q_score;
		}
		score[0] += value;
		break;
	}
	if (canSeeAnswer) {
		move(15, 4);
		prints("\033[1;32m正确答案是：\033[1;36m%s\033[0m", line + 1);
		if (q_score > 0) {
			move(16, 4);
			prints
			    ("\033[1;33m恭喜，本题您得到了 \033[1;36m%d\033[1;33m 分！\033[0m",
			     q_score);
		} else {
			move(16, 4);
			prints("\033[1;36m呜呜呜，没有答对...\033[0m");
		}
	}
	pressanykey();
	return 0;
}

static int
showTopTen(int showType, char *top10File, char *userid)
{
	FILE *fp;
	struct TOP_TEN_UNIT recordArray[10];
	int i, n;
	time_t t;

	clear();
	pageProfile();
	move(4, 28);
	prints("%s友谊测试TOP-TEN", showType == PERSONAL ? userid : "本站");
	move(6, 15);
	if (showType == PERSONAL) {
		prints("\033[1;32m名次  %-12s  成绩  测试时间\033[0m",
		       "UserID");
	} else {
		prints("\033[1;32m名次  %-12s  人气  最近访问\033[0m",
		       "UserID");
	}
	fp = fopen(top10File, "rb");
	if (fp == NULL) {
		return ERROR_CODE;
	}
	n = fread(recordArray, sizeof (struct TOP_TEN_UNIT), 10, fp);
	fclose(fp);
	for (i = 0; i < n; i++) {
		move(8 + i, 15);
		t = recordArray[i].time;
		prints("%-4d  %-12s  %-4d  %s", i + 1, recordArray[i].name,
		       recordArray[i].value, ctime(&t));
	}
	pressanykey();
	return 0;
}

static int
cmp_record(const void *a, const void *b)
{
	return ((struct TOP_TEN_UNIT *) b)->value -
	    ((struct TOP_TEN_UNIT *) a)->value;
}

static int
sortTopTen(char *top10File, struct TOP_TEN_UNIT recordToAdd)
{
	FILE *fp;
	struct TOP_TEN_UNIT recordArray[11];
	int n = 0, find = 0, i;

	fp = fopen(top10File, "rb");
	if (fp != NULL) {
		n = fread(recordArray, sizeof (struct TOP_TEN_UNIT), 10, fp);
		fclose(fp);
		for (i = 0; i < n; i++) {
			if (!strcmp(recordArray[i].name, recordToAdd.name)) {
				recordArray[i] = recordToAdd;
				find = 1;
				break;
			}
		}
		if (!find)
			recordArray[n] = recordToAdd;	//n+1个排序
		else
			n--;	//n个排序
		qsort(recordArray, n + 1, sizeof (struct TOP_TEN_UNIT),
		      cmp_record);
	} else {
		recordArray[0] = recordToAdd;
	}
	fp = fopen(top10File, "wb");
	if (fp == NULL)
		return ERROR_CODE;
	fwrite(recordArray, sizeof (struct TOP_TEN_UNIT), min(n + 1, 10), fp);
	fclose(fp);
	return 0;
}

static int
countValue(char *ans, char *buf, int value)
{
	int lenAns, lenBuf;
	int diff;

	if (!strcasecmp(ans, buf))
		return value;
	lenAns = strlen(ans);
	lenBuf = strlen(buf);
	diff = abs(lenAns - lenBuf);

	if (lenAns > 2 * lenBuf || lenBuf > 2 * lenAns)
		return 0;

	if (lenAns > lenBuf) {
		return strcasestr(ans,
				  buf) == NULL ? 0 : value / (1 + diff / 2);
	} else {
		return strcasestr(buf,
				  ans) == NULL ? 0 : value / (1 + diff / 2);
	}
}

static int
showNewIn()
{
	FILE *fp;
	int iCount, n;
	struct TOP_TEN_UNIT recordArray[10];
	time_t t;

	clear();
	pageProfile();
	move(4, 32);
	prints("\033[1;35m闪亮登场TOP-TEN\033[0m");
	move(6, 20);
	prints("\033[1;32m%-12s  加入时间", "UserID");
	fp = fopen(NEW_IN_FILE, "rb");
	if (fp == NULL) {
		return ERROR_CODE;
	}
	n = fread(recordArray, sizeof (struct TOP_TEN_UNIT), 10, fp);
	fclose(fp);
	for (iCount = 0; iCount < n; iCount++) {
		t = recordArray[iCount].time;
		move(8 + iCount, 20);
		prints("%-12s  %s", recordArray[iCount].name, ctime(&t));
	}
	pressanykey();
	return 0;
}

static int
addToNewIn()
{
	FILE *fp;
	struct TOP_TEN_UNIT recordArray[10];
	int n = 0;

	strcpy(recordArray[0].name, currentuser->userid);
	recordArray[0].value = 0;
	recordArray[0].time = time(0);

	fp = fopen(NEW_IN_FILE, "rb");
	if (fp != NULL) {
		n = fread(recordArray + 1, sizeof (struct TOP_TEN_UNIT), 9, fp);
		fclose(fp);
	}
	fp = fopen(NEW_IN_FILE, "wb");
	if (fp == NULL)
		return ERROR_CODE;
	fwrite(recordArray, sizeof (struct TOP_TEN_UNIT), n + 1, fp);
	fclose(fp);
	return 0;
}

static int
mail_top10_file(char *my_top10_path)
{
	FILE *fp;
	int n = 0, i;
	struct TOP_TEN_UNIT recordArray[10];

	fp = fopen(my_top10_path, "rb");
	if (fp == NULL)
		return ERROR_CODE;
	n = fread(recordArray, sizeof (struct TOP_TEN_UNIT), 10, fp);
	fclose(fp);
	sprintf(genbuf, MY_BBS_HOME "/bbstmpfs/tmp/%s.top10",
		currentuser->userid);
	fp = fopen(genbuf, "wb");
	if (fp == NULL)
		return ERROR_CODE;
	fprintf(fp, "名次  %-12s  成绩     测试时间\n", "UserID");
	for (i = 0; i < n; i++)
		fprintf(fp, "%4d  %-12s  %4d     %s\n", i + 1,
			recordArray[i].name, recordArray[i].value,
			ctime(&(recordArray[i].time)));
	fclose(fp);
	system_mail_file(genbuf, currentuser->userid, "我的友谊测试(排名榜)", currentuser->userid);
	return 0;
}

static int
modify_question()
{
	FILE *fp = NULL, *fp2 = NULL;
	int qNum, passwd, canSeeAnswer, canSeeTop10, allowError;
	char home_path[256], temp_path[256], line[MAX_LINE_LENGTH];
	int num_to_change = 0, add_new = 0, retv = 999;

	sethomefile(home_path, currentuser->userid, "friendshipTest.qest");
	sethomefile(temp_path, currentuser->userid, "friendshipTest.qest.tmp");
	fp = fopen(home_path, "r");
	if (fp == NULL)
		return ERROR_CODE;
	fscanf(fp, "%d %d %d %d %d\n", &qNum, &passwd, &canSeeAnswer,
	       &canSeeTop10, &allowError);
	move(4, 4);
	prints("\033[1;33m当前测试属性:\033[0m");
	move(5, 4);
	prints
	    ("\033[1;35m(N)题    数:%-3d    (P)密    码:%-6d    (A)公布答案:%-4s",
	     qNum, passwd, canSeeAnswer ? "Yes" : "No");
	move(6, 4);
	prints
	    ("\033[1;35m(T)公布排行:%-3s    (M)容错模式:%-6s    (E)退出\033[0m",
	     canSeeTop10 ? "Yes" : "No", allowError ? "Yes" : "No");
	getdata(8, 4, "您要更改什么属性[\033[1;33m输入字母代号\033[0m]:",
		genbuf, 2, DOECHO, YEA);
	switch (genbuf[0]) {
	case 'n':
	case 'N':
		getdata(9, 4, "修改题号[1-20]:", genbuf, 3, DOECHO, YEA);
		num_to_change = min(atoi(genbuf), 20);
		if (num_to_change <= 0)
			break;
		if (num_to_change > qNum) {
			move(10, 4);
			if (askyn("这将添加新的问题,继续吗?", YEA, NA) == NA)
				break;
			qNum++;
			add_new = 1;
		}
		fp2 = fopen(temp_path, "w");
		if (fp2 == NULL) {
			fclose(fp);
			return ERROR_CODE;
		}
		fprintf(fp2, "%d %d %d %d %d\n", qNum, passwd, canSeeAnswer,
			canSeeTop10, allowError);
		while (fgets(line, MAX_LINE_LENGTH, fp)) {
			if (line[0] == '#' && atoi(line + 2) == num_to_change) {
				char option[4][STRLEN];
				char hint[STRLEN];
				char ans[STRLEN];
				int value;

				fgets(genbuf, STRLEN, fp);
				value = atoi(genbuf + 1);
				if (line[1] == 'B') {
					fgets(genbuf, STRLEN, fp);
					fgets(genbuf, STRLEN, fp);
				} else if (line[1] == 'S') {
					int i;
					for (i = 0; i < 4; i++)
						fgets(option[i], STRLEN, fp);
				} else {
					fgets(genbuf, STRLEN, fp);
				}
				fgets(hint, STRLEN, fp);
				fgets(ans, STRLEN, fp);
				do {
					retv =
					    dealWithQuestionInput(fp2,
								  num_to_change,
								  1, line[1],
								  line, option,
								  hint, ans,
								  value);
				} while (retv == NEED_REPEAT);
			} else
				fputs(line, fp2);
		}
		if (add_new) {
			do {
				retv =
				    dealWithQuestionInput(fp2, qNum, 0, 0, NULL,
							  NULL, NULL, NULL, 0);
			} while (retv == NEED_REPEAT);
		}
		fclose(fp2);
		break;
	case 'p':
	case 'P':
	case 'a':
	case 'A':
	case 't':
	case 'T':
	case 'm':
	case 'M':
		move(9, 4);
		if (askyn
		    ("进入测试是否需要密码[\033[1;33m若选择，密码将寄回邮箱\033[0m]？",
		     NA, NA) == YEA) {
			srandom(time(0));
			passwd = 100000 + random() % 900000;
			sprintf(genbuf,
				"您的友谊测试的密码是：%d。您可以将此信件转发给您的好友。",
				passwd);
			system_mail_buf(genbuf, strlen(genbuf), currentuser->userid,
				 "我的友谊测试密码", currentuser->userid);
		} else {
			passwd = 0;
		}
		move(10, 4);
		if (askyn("是否公开答案？", NA, NA) == YEA) {
			canSeeAnswer = 1;
		} else {
			canSeeAnswer = 0;
		}
		move(11, 4);
		if (askyn("是否公开本测试的排行榜？", YEA, NA) == YEA) {
			canSeeTop10 = 1;
		} else {
			canSeeTop10 = 0;
		}
		move(12, 4);
		if (askyn
		    ("问答比照是否采用\033[1;32m容错模式\033[0m？", YEA,
		     NA) == YEA) {
			allowError = 1;
		} else {
			allowError = 0;
		}
		move(15, 4);
		if (askyn("确定修改吗?", NA, NA) == NA)
			break;
		fp2 = fopen(temp_path, "w");
		if (fp2 == NULL) {
			fclose(fp);
			return ERROR_CODE;
		}
		fprintf(fp2, "%d %d %d %d %d\n", qNum, passwd, canSeeAnswer,
			canSeeTop10, allowError);
		while (fgets(line, MAX_LINE_LENGTH, fp)) {
			fputs(line, fp2);
		}
		fclose(fp2);
		retv = 0;
		break;
	}
	fclose(fp);
	move(21, 4);
	if (retv == 0) {
		unlink(home_path);
		rename(temp_path, home_path);
		prints("修改完成!");
	} else {
		unlink(temp_path);
		prints("取消修改...");
	}
	pressanykey();
	return 0;
}

static int
manage_top10()
{
	char ch, top10_path[256];
	int isSYSOP = (currentuser->userlevel & PERM_SYSOP);

	move(4, 4);
	prints("\033[1;36m清除下列哪个Top Ten显示:\033[0m");
	move(6, 8);
	prints("1. 我的测试Top Ten        Q. 退出");
	if (isSYSOP) {
		move(7, 8);
		prints("2. 全站人气Top Ten        3. 最新加入Top Ten");
	}
	ch = igetkey();
	switch (ch) {
	case '1':
		sethomefile(top10_path, currentuser->userid,
			    "friendshipTest.top10");
		break;
	case '2':
		if (isSYSOP)
			strcpy(top10_path, TOPTENHOT_FILE);
		break;
	case '3':
		if (isSYSOP)
			strcpy(top10_path, NEW_IN_FILE);
		break;
	default:
		return 0;
	}
	move(9, 4);
	if (askyn("确定清除吗?", NA, NA) == NA)
		return 0;
	unlink(top10_path);
	move(10, 4);
	prints("操作完成.");
	pressanykey();
	return 0;
}

static int
show_adage()
{
	int select;
	char adages[][100] = {
		"有朋自远方来，不亦乐乎。\t《论语》",
		"一个对朋友和邻居虚伪的人决不可能对公众真诚。\t贝克莱主教",
		"敌人不信你的解释，朋友无需你的解释。\t印度格言",
		"一个人如果抛弃他忠实的朋友，就等于抛弃他最珍贵的生命。\t（古希腊）索福克勒斯",
		"如果你把快乐告诉一个朋友，将得到两个快乐。\t（英）弗.培根",
		"如果你把忧愁向一个朋友倾吐，将被分掉一半忧愁。\t（英）弗.培根",
		"像榕树一般成长起来的友情，要比像瓜蔓般突然蹿起来的友情更为可靠。\t（英）夏洛蒂．勃朗特",
		"友谊和花香一样，还是淡一点比较好。越淡的香气越使人依恋，也越能长久。\t席慕蓉",
		"朋友一生一起走/那些日子不再有,一句话一辈子/一生情一杯酒。\t周华健《朋友》",
		"朋友啊朋友/你可曾想起了我/如果你正承受不幸/请你告诉我。\t臧天朔《朋友》"
	};
	clear();
	pageProfile();
	move(4, 4);
	prints("朋友格言共赏析:");
	srandom(time(0));
	select = random() % 10;
	strtok(adages[select], "\t");
	move(8, 10);
	prints("%s", adages[select]);
	move(10, 40);
	prints("----%s", strtok(NULL, "\0"));
	pressanykey();
	return 0;
}
