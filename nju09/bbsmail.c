#include "bbslib.h"

int
bbsmail_main()
{
	FILE *fp;
	int i, start, total;
	char buf[512], dir[80];
	char *ptr;
	struct fileheader x;
	if (!loginok || isguest)
		http_fatal("您尚未登录, 请先登录");
	sprintf(dir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]),
		currentuser->userid);
	html_header(1);
	check_msg();
	changemode(RMAIL);
	strsncpy(buf, getparm("start"), 10);
	start = atoi(buf);
	if (buf[0] == 0)
		start = 999999;
	i = file_time(dir);
	printf("<script language=javascript>function selectall()\
	{\
	for (i=0;i<document.maillist.length - 1;i++)\
	{\
		document.maillist.elements[i].checked=document.maillist.all.checked;\
	}}");
	printf("function unselectall()\
	{\
	if (document.maillist.all.checked) {\
		document.maillist.all.checked = 0;\
	}\
	}\
</script>");
	printf("</head><body><center>\n");
	printf("%s -- 信件列表 [使用者: %s] [信箱容量: %dk, 已用空间: %dk]\n",
	       BBSNAME, currentuser->userid, max_mailsize(currentuser), get_mailsize(currentuser));
	if ((ptr=check_mailperm(currentuser)))
		printf("<script>alert('%s')</script>\n", ptr);
	total = file_size(dir) / sizeof (struct fileheader);
	if (total < 0 || total > 30000)
		http_fatal("too many mails");
	if (!total) {
		printf
		    ("目前还没有任何信件<br><br>[<a href=bbspstmail>发送信件</a>]");
		http_quit();
		return 0;
	}
	start = getdocstart(total, w_info->t_lines);

	fp = fopen(dir, "r");
	if (fp == 0)
		http_fatal("无法打开目录文件, 请通知系统维护");
	printhr();
	printf("<form action=bbsdelmail name=maillist><table>\n");
	printf
	    ("<tr><td></td><td>序号</td><td>状态</td><td>发信人</td><td>日期</td><td>信件标题</td></tr>\n");
	fseek(fp, (start - 1) * sizeof (struct fileheader), SEEK_SET);
	for (i = 0; i < w_info->t_lines; i++) {
		int type = 'N';
		if (fread(&x, sizeof (x), 1, fp) <= 0)
			break;
		printf("<tr><td><input type=checkbox name=F%d value=%s onclick=unselectall()></td>", i, fh2fname(&x));
		printf("<td>%d</td>", i + start);
		if (x.accessed & FH_READ)
			type = ' ';
		if (x.accessed & FH_MARKED)
			type = (type == 'N') ? 'M' : 'm';
		printf("<td>%s%c%s</td>",
		       (x.accessed & FH_REPLIED) ? "R" : "&nbsp;", type,
		       x.accessed & FH_ATTACHED ? "@" : "");
		printf("<td>%s</td>", userid_str(fh2owner(&x)));
		printf("<td>%12.12s</td>", Ctime(x.filetime) + 4);
		printf("<td><a href=bbsmailcon?file=%s&num=%d>", fh2fname(&x),
		       i + start);
		if (strncmp("Re: ", x.title, 4))
			printf("★ ");
		printf("%s", nohtml(void1(x.title)));
		printf("</a></td></tr>\n");
	}
	printf("</table>");
	fclose(fp);
	printhr();
	printf("[信件总数: %d] ", total);
	printf("[<input type=\"checkbox\" name=\"all\" value=1 onclick=selectall()>选择本页全部邮件] ");
	printf("<input type=hidden name=spam value=0>");
	printf
	    ("[<a onclick='return confirm(\"你真的要删除这些信件吗?\")'"
	     " href='javascript:document.maillist.submit()'>删除选中的邮件</a>] ");
	printf
	    ("[<a onclick='return confirm(\"你真的认为被选中的信件都是垃圾邮件吗? 如果不是请使用删除信件功能\")'"
	     " href='javascript:document.maillist.spam.value=1;document.maillist.submit()'>垃圾邮件</a>] ");
	printf("[<a href=bbspstmail>发送信件</a>] ");
	if (total > w_info->t_lines)
		printf
		    ("<a href=mail?S=1>第一页</a> <a href=mail?S=-100>最后一页</a> ");
	bbsdoc_helper("bbsmail?", start, total, w_info->t_lines);
	printf("</form>\n");
	printf
	    ("<form><input type=submit value=跳转到> 第 <input style='height:20px' type=text name=start size=3> 封</form></body>");
	http_quit();
	return 0;
}
