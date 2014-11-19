#include "bbslib.h"

static int show_form(char *board);

int
bbsbfind_main()
{
	FILE *fp;
	int num = 0, total = 0, type, dt, mg = 0, og = 0, at = 0;
	char dir[80], title[80], title2[80], title3[80], board[80], userid[80];
	struct boardmem *brd;
	struct fileheader x;
	html_header(1);
	changemode(READING);
	check_msg();
	printf("</head><body><center>%s -- 版内文章搜索<hr><br>\n", BBSNAME);
	type = atoi(getparm("type"));
	getparmboard(board, sizeof (board));
	brd = getboard(board);
	if (brd == 0)
		http_fatal("错误的讨论区");
	if (type == 0)
		return show_form(board);
	strsncpy(title, getparm("title"), 60);
	strsncpy(title2, getparm("title2"), 60);
	strsncpy(title3, getparm("title3"), 60);
	strsncpy(userid, getparm("userid"), 60);
	if (!strcasecmp(userid, "Anonymous"))
		userid[0] = 0;
	dt = atoi(getparm("dt"));
	if (!strcasecmp(getparm("mg"), "on"))
		mg = 1;
	if (!strcasecmp(getparm("at"), "on"))
		at = 1;
	if (!strcasecmp(getparm("og"), "on"))
		og = 1;
	if (dt < 0)
		dt = 0;
	if (dt > 9999)
		dt = 9999;
	sprintf(dir, "boards/%s/.DIR", board);
	fp = fopen(dir, "r");
	if (fp == 0)
		http_fatal("讨论区错误或没有目前文章");
	printf("查找讨论区'%s'内, 标题含: '%s' ", board, nohtml(title));
	if (title2[0])
		printf("和 '%s' ", nohtml(title2));
	if (title3[0])
		printf("不含 '%s' ", nohtml(title3));
	printf("作者为: '%s', '%d'天以内的%s%s文章.<br>\n",
	       userid[0] ? userid_str(userid) : "所有作者", dt,
	       mg ? "被标记" : "所有", at ? "有附件" : "");
	printf("<table>\n");
	printf("<tr><td>编号<td>标记<td>作者<td>日期<td>标题\n");
	if (search_filter(title, title2, title3))
		goto E;
	while (1) {
		if (fread(&x, sizeof (x), 1, fp) == 0)
			break;
		num++;
		if (title[0] && !strcasestr(x.title, title))
			continue;
		if (title2[0] && !strcasestr(x.title, title2))
			continue;
		if (userid[0] && strcasecmp(x.owner, userid))
			continue;
		if (title3[0] && strcasestr(x.title, title3))
			continue;
		if (abs(now_t - x.filetime) > dt * 86400)
			continue;
		if (mg && !(x.accessed & FH_MARKED)
		    && !(x.accessed & FH_DIGEST))
			continue;
		if (at && !(x.accessed & FH_ATTACHED))
			continue;
		if (og && !strncmp(x.title, "Re: ", 4))
			continue;
		total++;
		printf("<tr><td>%d</td>", num);
		printf("<td>%s</td>", flag_str(x.accessed));
		printf("<td>%s</td>", userid_str(x.owner));
		printf("<td>%12.12s</td>", 4 + Ctime(x.filetime));
		printf
		    ("<td><a href=con?B=%d&F=%s&N=%d&T=%d>%.40s </a>%s</td>\n",
		     getbnumx(brd), fh2fname(&x), num, feditmark(&x), x.title,
		     size_str(bytenum(x.sizebyte)));
		if (total >= 999)
			break;
	}
      E:
	fclose(fp);
	printf("</table>\n");
	printf("<br>共找到 %d 篇文章符合条件", total);
	if (total > 999)
		printf("(匹配结果过多, 省略第1000以后的查询结果)");
	printf("<br>\n");
	printf
	    ("[<a href=bbsdoc?B=%d>返回本讨论区</a>] [<a href='javascript:history.go(-1)'>返回上一页</a>]</body>",
	     getbnumx(brd));
	http_quit();
	return 0;
}

static int
show_form(char *board)
{
	printf("<table><form action=bbsbfind>\n");
	printf("<input type=hidden name=type value=1>");
	printf
	    ("<tr><td>版面名称: <input type=text maxlength=24 size=24 name=board value='%s'><br>\n",
	     board);
	printf
	    ("<tr><td>标题含有: <input type=text maxlength=50 size=20 name=title> 和 ");
	printf("<input type=text maxlength=50 size=20 name=title2>\n");
	printf
	    ("<tr><td>标题不含: <input type=text maxlength=50 size=20 name=title3>\n");
	printf
	    ("<tr><td>作者帐号: <input type=text maxlength=12 size=12 name=userid><br>\n");
	printf
	    ("<tr><td>时间范围: <input type=text maxlength=4  size=4  name=dt value=7> 天以内<br>\n");
	printf("<tr><td>精华文章: <input type=checkbox name=mg><br>");
	printf("含有附件: <input type=checkbox name=at><br>");
	printf("不含跟贴: <input type=checkbox name=og><br><br>\n");
	printf("<tr><td><input type=submit value=递交查询结果>\n");
	printf("</form></table>");

	if (!strcmp(MY_BBS_DOMAIN, "tttan.com")) {
		//showfile("wwwtmp/estseek.form");
		printf("<br><hr>"
		       "<form action=/cgi-bin/estseek.cgi method=get accept-charset=utf-8>"
		       "<table><tr><td>"
		       "全文检索：<br>"
		       "<input type=text name=phrase value='' size=75 />"
		       "<input type=submit value='搜！' /><br>"
		       "每页<select name=perpage>"
		       "<option value=10 selected=selected>10</option>"
		       "<option value=20>20</option>"
		       "<option value=30>30</option>"
		       "<option value=40>40</option>"
		       "<option value=50>50</option>"
		       "<option value=100>100</option>"
		       "</select><br>"
		       "要求<select name=attr id=attr>"
		       "<option value=''>--</option>"
		       "<option value='@title ISTRINC'>标题包含</option>"
		       "<option value='@author ISTREQ'>作者为</option>"
		       "<option value='@board ISTREQ' selected=selected>所在版面</option>"
		       "</select>"
		       "<input type=text name=attrval value='%s' size=12 /><br>"
		       "并且<select name=attr2>"
		       "<option value=''>--</option>"
		       "<option value='@title ISTRINC'>标题包含</option>"
		       "<option value='@author ISTREQ'>作者为</option>"
		       "<option value='@board ISTREQ'>版面</option>"
		       "</select>"
		       "<input type=text name=attrval2 value='' size=12 />"
		       "</td></tr></table></form><hr>", board);
	}

	printf
	    ("[<a href='bbsdoc?B=%d'>返回上一页</a>] [<a href=bbsfind>全站文章查询</a>]",
	     getbnum(board));
	http_quit();
	return 0;
}
