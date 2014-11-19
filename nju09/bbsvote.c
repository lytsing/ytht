#include "bbslib.h"
static char *vote_type[] = { "是非", "单选", "复选", "数字", "问答" };
int addtofile();

int
bbsvote_main()
{
	FILE *fp;
	struct votebal currvote, ent;
	char buf[STRLEN], buf1[512];
	struct ballot uservote;
	struct votelog log;
	int aborted = NA, pos;
	int i;
	unsigned int j, multiroll = 0;
	char board[80];
	char controlfile[STRLEN];
	char *date, *tmp1, *tmp2;
	char flagname[STRLEN];
	char logname[STRLEN];
	int voted_flag;		//用户是否投过该项票
	int num_voted;		//这个是有多少人投过票
	int num_of_vote;	//这个是开启的投票数
	int votenum;		//这个是用户选择进行第几个投票
	int votevalue = 0;
	int procvote;
	time_t closedate;
	struct stat st;
	struct boardmem *x;
	html_header(1);
	check_msg();
	getparmboard(board, sizeof (board));
	votenum = atoi(getparm("votenum"));
	procvote = atoi(getparm("procvote"));
	printf("<body><center>");
	if (!loginok || isguest) {
		printf("匆匆过客不能投票,请先登录!<br><br>");
		printf("<script>openlog();</script>");
		http_quit();
	}
	if (!USERPERM(currentuser, PERM_VOTE))
		http_fatal("对不起，您无权投票");
	x = getboard(board);
	if (!x || !has_vote_perm(currentuser, x))
		http_fatal("对不起，您无权投票");
	printf("<a href=bbsdoc?B=%d><h2>%s讨论区</h2></a></center>",
	       getbnumx(x), board);
	changemode(VOTING);
	if (votenum == 0) {
		sprintf(controlfile, "vote/%s/%s", board, "control");
		num_of_vote =
		    (stat(controlfile, &st) ==
		     -1) ? 0 : st.st_size / sizeof (struct votebal);
		if (num_of_vote == 0)
			http_fatal("抱歉, 目前并没有任何投票举行。");
		fp = fopen(controlfile, "r");
		printf("<center><table border=1><tr>");
		printf("<td>编号</td>");
		printf("<td>开启投票箱者</td>");
		printf("<td>开启日</td>");
		printf("<td>投票主题</td>");
		printf("<td>类别</td>");
		printf("<td>天数</td>");
		printf("<td>人数</td>");
		printf("</tr>");
		for (i = 1; i <= num_of_vote; i++) {
			fread(&ent, sizeof (struct votebal), 1, fp);
			sprintf(flagname, "vote/%s/flag.%d", board,
				(int) ent.opendate);
			num_voted =
			    (stat(flagname, &st) ==
			     -1) ? 0 : st.st_size / sizeof (struct ballot);
			date = ctime(&ent.opendate) + 4;
			printf("<tr>");
			printf("<td>%d</td>", i);
			printf("<td><a href=bbsqry?userid=%s>%s</a></td>",
			       ent.userid, ent.userid);
			printf("<td>%.24s</td>", date);
			printf
			    ("<td><a href=bbsvote?B=%d&votenum=%d>%s<a></td>",
			     getbnumx(x), i, ent.title);
			printf("<td>%s</td>", vote_type[ent.type - 1]);
			printf("<td>%d</td>", ent.maxdays);
			printf("<td>%d</td>", num_voted);
			printf("</tr>");
		}
		printf("</table></center>");
		fclose(fp);
		printf("<p><a href=javascript:history.go(-1)>返回上一页</a>");
	} else {
		sprintf(controlfile, "vote/%s/%s", board, "control");
		num_of_vote =
		    (stat(controlfile, &st) ==
		     -1) ? 0 : st.st_size / sizeof (struct votebal);
		if (num_of_vote == 0)
			http_fatal("抱歉, 目前并没有任何投票举行。");
		if (votenum > num_of_vote)
			http_fatal("参数错误");
		fp = fopen(controlfile, "r");
		printf("<table width=600>");
		fseek(fp, sizeof (struct votebal) * (votenum - 1), 0);
		fread(&currvote, sizeof (struct votebal), 1, fp);
		fclose(fp);
		//add by gluon for sm_vote
		if (!(currentuser->userlevel & PERM_LOGINOK))
			http_fatal("对不起, 您还没有通过注册呢");
		if (currvote.flag & VOTE_FLAG_LIMITED) {
			int retv = valid_voter(board, currentuser->userid);
			if (retv == 0 || retv == -1) {
				http_fatal("对不起,您没有选举权");
			}
		}
		if (currentuser->firstlogin >= currvote.opendate - 7 * 86400)
			http_fatal
			    ("对不起, 您注册还没满7天这个投票就开了, 没有资格投票\n");
		if ((currvote.flag & VOTE_FLAG_LIMITIP)
		    && invalid_voteIP(realfromhost))
			http_fatal("对不起,您从穿梭站而来,没有投票权\n");
		sprintf(flagname, "vote/%s/flag.%d", board,
			(int) currvote.opendate);
		num_voted =
		    (stat(flagname, &st) ==
		     -1) ? 0 : st.st_size / sizeof (struct ballot);
		pos = 0;
		fp = fopen(flagname, "r");
		voted_flag = NA;
		if (fp) {
			j = strlen(currentuser->userid) + 1;
			if (j > IDLEN)
				j = IDLEN;
			for (i = 1; i <= num_voted; i++) {
				fread(&uservote, sizeof (struct ballot), 1, fp);
				if (!memcmp
				    (uservote.uid, currentuser->userid, j)) {
					voted_flag = YEA;
					pos = i;
					break;
				}
			}
			fclose(fp);
		}
		if (!voted_flag)
			(void) memset(&uservote, 0, sizeof (uservote));
		if (procvote == 0) {
			date = ctime(&currvote.opendate) + 4;
			closedate =
			    currvote.opendate + currvote.maxdays * 86400;
			printf("投票主题是 %s<br>",
			       void1(nohtml(currvote.title)));
			printf("投票类型是 %s<br>",
			       vote_type[currvote.type - 1]);
			printf("投票将于 %s 结束<br>", ctime(&closedate));
			printf("投票人ID等将%s<br>",
			       (currvote.flag & VOTE_FLAG_OPENED) ? "公开" :
			       "不公开");
			if (currvote.type != VOTE_ASKING)
				printf("您可以投%d票<br>", currvote.maxtkt);
			printf("<hr>投票说明:<br>");
			sprintf(buf, "vote/%s/desc.%d", board,
				(int) currvote.opendate);
			fp = fopen(buf, "r");
			if (fp == 0)
				http_fatal("投票说明丢失");
			while (1) {
				if (fgets(buf1, sizeof (buf1), fp) == 0)
					break;
				fhhprintf(stdout, "%s", buf1);
			}
			fclose(fp);
			printf("<hr><form name=voteform method=post>");
			if ((currvote.type != VOTE_ASKING)
			    && (currvote.type != VOTE_VALUE))
				multiroll =
				    (num_voted + now_t) % currvote.totalitems;
			switch (currvote.type) {
			case VOTE_SINGLE:
				j = (uservote.voted >> multiroll) +
				    (uservote.voted << (currvote.totalitems -
							multiroll));
				for (i = 0; i < currvote.totalitems; i++) {
					printf
					    ("<input type=radio name=votesingle value=%d %s> %s <br>",
					     (i +
					      multiroll) % currvote.totalitems +
					     1, (j & 1) ? "checked" : "",
					     nohtml(void1
						    (currvote.items
						     [(i + multiroll) %
						      currvote.totalitems])));
					j >>= 1;
				}
				printf
				    ("<input type=hidden name=procvote value=2>");
				break;
			case VOTE_MULTI:
				j = (uservote.voted >> multiroll) +
				    (uservote.voted << (currvote.totalitems -
							multiroll));
				for (i = 0; i < currvote.totalitems; i++) {
					printf
					    ("<input type=checkbox name=votemulti%d value=%d %s> %s <br>",
					     (i +
					      multiroll) % currvote.totalitems +
					     1, 1, (j & 1) ? "checked" : "",
					     nohtml(void1
						    (currvote.items
						     [(i + multiroll) %
						      currvote.totalitems])));
					j >>= 1;
				}
				printf
				    ("<input type=hidden name=procvote value=3>");
				break;
			case VOTE_YN:
				j = (uservote.voted >> multiroll) +
				    (uservote.voted << (currvote.totalitems -
							multiroll));
				for (i = 0; i < currvote.totalitems; i++) {
					printf
					    ("<input type=radio name=voteyn value=%d %s> %s <br>",
					     (i +
					      multiroll) % currvote.totalitems +
					     1, (j & 1) ? "checked" : "",
					     nohtml(void1
						    (currvote.items
						     [(i + multiroll) %
						      currvote.totalitems])));
					j >>= 1;
				}
				printf
				    ("<input type=hidden name=procvote value=1>");
				break;
			case VOTE_VALUE:
				printf("请输入一个值：");
				printf
				    ("<input type=text name=votevalue value=%d><br>",
				     uservote.voted);
				printf
				    ("<input type=hidden name=procvote value=4>");
				break;
			case VOTE_ASKING:
				printf
				    ("<input type=hidden name=procvote value=5>");
				break;
			default:
				http_fatal("没有这种类型的投票啊");
			}
			printf("<p>请留下您的建议(最多三行有效)<br>");
			printf("<textarea name=sug rows=3 cols=79 wrap=off>");
			printf("%s\n", nohtml(void1(uservote.msg[0])));
			printf("%s\n", nohtml(void1(uservote.msg[1])));
			printf("%s\n", nohtml(void1(uservote.msg[2])));
			printf("</textarea><br>");
			printf("<input type=submit name=Submit value=投下去>");
			printf
			    ("<input type=reset name=Submit2 value=我再改改>");
			printf("</form>");
		} else {
			if (procvote != currvote.type)
				http_fatal("faint，你怎么可以这么干?");
			switch (procvote) {
			case 2:	//VOTE_SINGLE
				votevalue = 1;
				votevalue <<= atoi(getparm("votesingle")) - 1;
				if (atoi(getparm("votesingle")) >
				    currvote.totalitems + 1)
					http_fatal("参数错误");
				aborted = (votevalue == uservote.voted);
				break;
			case 3:	//VOTE_MULTI
				votevalue = 0;
				j = 0;
				for (i = currvote.totalitems - 1; i >= 0; i--) {
					votevalue <<= 1;
					sprintf(buf, "votemulti%d", i + 1);
					votevalue += atoi(getparm(buf));
					j += atoi(getparm(buf));
				}
				aborted = (votevalue == uservote.voted);
				if (j > currvote.maxtkt) {
					sprintf(buf,
						"啊，说好了最多只能投%d个的",
						currvote.maxtkt);
					http_fatal(buf);
				}
				break;
			case 1:	//VOTE_YN
				votevalue = 1;
				votevalue <<= atoi(getparm("voteyn")) - 1;
				if (atoi(getparm("voteyn")) >
				    currvote.totalitems + 1)
					http_fatal("参数错误");
				aborted = (votevalue == uservote.voted);
				break;
			case 4:	//VOTE_VALUE
				aborted =
				    ((votevalue = atoi(getparm("votevalue"))) ==
				     uservote.voted);
				if (votevalue > currvote.maxtkt) {
					sprintf(buf, "说好了最多只能填%d的啊",
						currvote.maxtkt);
					http_fatal(buf);
				}
				break;
				//              case 5: //VOTE_ASKING
			}
			if (aborted == YEA) {
				printf("保留 【%s】原来的的投票。<p>",
				       currvote.title);
			} else {
				fp = fopen(flagname, "r+");
				if (fp == 0)
					fp = fopen(flagname, "w+");
				flock(fileno(fp), LOCK_EX);
				if (pos > 0)
					fseek(fp,
					      (pos -
					       1) * sizeof (struct ballot),
					      SEEK_SET);
				else
					fseek(fp, 0, SEEK_END);
				strcpy(uservote.uid, currentuser->userid);
				uservote.voted = votevalue;
				strsncpy(buf1, getparm("sug"), 500);
				tmp2 = buf1;
				if (pos > 0)
					uservote.msg[0][0] =
					    uservote.msg[1][0] =
					    uservote.msg[2][0] = 0;
				for (i = 0; i < 3; i++) {
					tmp1 = strchr(tmp2, '\n');
					if (tmp1 != NULL)
						*tmp1 = 0;
					strsncpy(uservote.msg[i], tmp2, 70);
					if (tmp1 == NULL)
						break;
					tmp2 = tmp1 + 1;
				}
				fwrite(&uservote, sizeof (struct ballot), 1,
				       fp);
				flock(fileno(fp), LOCK_UN);
				fclose(fp);
				if (currvote.flag & VOTE_FLAG_OPENED) {
					strcpy(log.uid, currentuser->userid);
					strsncpy(log.ip, fromhost,
						 sizeof (log.ip));
					log.votetime = now_t;
					log.voted = uservote.voted;
					sprintf(logname, "vote/%s/newlog.%d",
						board, (int) currvote.opendate);
					fp = fopen(logname, "a+");
					flock(fileno(fp), LOCK_EX);
					fwrite(&log, sizeof (struct votelog), 1,
					       fp);
					flock(fileno(fp), LOCK_UN);
					fclose(fp);
				}
				printf("<p>已经帮您投入票箱中...</p>");
				if (!strcmp(board, "SM_Election")) {
					sprintf(buf, "%s %s %s",
						currentuser->userid,
						inet_ntoa(from_addr),
						Ctime(now_t));
					addtofile(MY_BBS_HOME "/vote.log", buf);
				}
			}
			printf("<a href=javascript:history.go(-3)>返回</a>");
		}
	}
	http_quit();
	return 0;
}

int
addtofile(filename, str)
char filename[STRLEN], str[256];
{
	FILE *fp;
	int rc;

	if ((fp = fopen(filename, "a")) == NULL)
		return -1;
	flock(fileno(fp), LOCK_EX);
	rc = fprintf(fp, "%s\n", str);
	fflush(fp);
	flock(fileno(fp), LOCK_UN);
	fclose(fp);
	return (rc == EOF ? -1 : 1);
}

int
valid_voter(char *board, char *name)
{
	FILE *in;
	char buf[100];
	int i;

/*	in = fopen(MY_BBS_HOME "/etc/untrust", "r");
	while (fgets(buf, 80, in)) {
		i = strlen(buf);
		if (buf[i - 1] == '\n')
			buf[i - 1] = 0;
		if (!strcmp(buf, currentuser.lasthost)) {
			fclose(in);
			return -1;
		}
	}
	fclose(in);*/
	sprintf(buf, "%s/%s.validlist", MY_BBS_HOME, board);
	in = fopen(buf, "r");
	if (in != NULL) {
		while (fgets(buf, 80, in)) {
			i = strlen(buf);
			if (buf[i - 1] == '\n')
				buf[i - 1] = 0;
			if (!strcmp(buf, name)) {
				fclose(in);
				return 1;
			}
		}
		fclose(in);
	}
	return 0;
}
