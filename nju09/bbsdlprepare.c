#include "bbslib.h"

int
bbsdlprepare_main()
{
	char taskfile[512];
	char *action;
	int done;
	html_header(1);
	if (!loginok || isguest)
		http_fatal("您尚未登录, 请先登录");
	sprintf(taskfile, "dllist/%s.task", currentuser->userid);
	printf("<body>　　现在提供个人文集打包给用户本人下载。"
	       "文集将以 rar 格式打包。请输入的 rar 文件加密密码，"
	       "系统将使用该密码生成个人文集的压缩文件 %s.***.Personal.rar，"
	       "然后您就可以下载该文件，并以设定的密码解压缩该文件。"
	       "打包文件不是即时生成的，需要等候 1 小时左右。<br><br>",
	       currentuser->userid);
	if (!file_exist(taskfile)) {
		printf("<hr>");
		http_fatal("您没有个人文集");
	}
	action = getparm("action");
	if (!strcmp(action, "setpass")) {
		trySetDLPass(taskfile);
	}
	done = reportStatus(taskfile);
	printSetDLPass();
	return 0;
}

int
trySetDLPass(char *taskfile)
{
	char *passwd, *p1, *p2;
	char buf[256], path[512];
	passwd = getparm("passwd");
	p1 = getparm("pass");
	p2 = getparm("passverify");
	printf("<hr>");
	if (!checkpasswd(currentuser->passwd, currentuser->salt, passwd)) {
		printf("<font color=red>BBS 用户密码输入不正确！</font><br>");
		return -1;
	}
	if (strcmp(p1, p2)) {
		printf
		    ("<font color=red>压缩文件密码输入错误，请重新输入</font><br>");
		return -1;
	}
	savestrvalue(taskfile, "pass", p1);
	savestrvalue(taskfile, "done", "0");
	printf("<font color=red>加密打包文件的密码设置成功！</font><br>");
	readstrvalue(taskfile, "rarfile", buf, sizeof (buf));
	if (*buf) {
		sprintf(path, HTMPATH "/download/%s", buf);
		unlink(path);
		savestrvalue(taskfile, "rarfile", "");
	}
	return 0;
}

int
printSetDLPass()
{
	printf("<hr>设置打包密码（现有打包将被删除，打包文件将重新生成）<br>");
	printf("<form action=bbsdlprepare?action=setpass method=post>"
	       "BBS 用户的密码<input type=password name=passwd maxlenght=%d size=%d><br>"
	       "加密打包文件(*.rar)的密码<input type=password name=pass maxlength=20 size=20><br>"
	       "再输入一次加密密码<input type=password name=passverify maxlength=20 size=20><br>"
	       "<input type=submit value=\"确定\">"
	       "</form>", PASSLEN - 1, PASSLEN - 1);
	return 0;
}

int
reportStatus(char *taskfile)
{
	char buf[512];
	printf("<hr><font color=green>目前状态：<br>");
	if (readstrvalue(taskfile, "done", buf, sizeof (buf)) >= 0
	    && *buf == '1') {
		readstrvalue(taskfile, "rarfile", buf, sizeof (buf));
		if (!*buf) {
			printf("文章打包出现错误，请通知 sysop。<br>");
		} else {
			printf("打包已经完成，请下载。<br>");
			printf
			    ("打包文件连接：<a href=/download/%s>这里</a><br>",
			     buf);
			printf("如果使用的是 IE 浏览器，而且 ID 包含汉字，"
			       "并且发生无法下载的现象，需要在 IE 的菜单里面找到：<br>"
			       "　　Tools  =>> Internet Options...  =>>  Advanced =>> Always send URLs as UTF-8<br>"
			       "取消这个选项，并重新启动 IE 浏览器。</font><br>");

		}
		return 1;
	}
	if (readstrvalue(taskfile, "pass", buf, sizeof (buf)) >= 0 && *buf) {
		printf("已经设定加密打包文件的密码，请等候打包。<br>");
		printf("打包文件连接：暂无。<br>");
	} else {
		printf("尚未设定加密打包文件的密码，请输入打包密码。<br>");
		printf("打包文件连接：暂无。<br>");
	}
	printf("</font>");
	return 0;
}
