#include "bbslib.h"
int
bbsspamcon_main()
{
	unsigned long long id;
	int magic;
	char title[60], sender[40];
	char path[256];
	char idbuf[100];
	int ret;
	int feedham;
	struct yspam_ctx *yctx;
	if ((!loginok || isguest) && (!tempuser))
		http_fatal("请先登录%d", tempuser);
	changemode(RMAIL);
	id = strtoull(getparm("id"), NULL, 10);
	magic = atoi(getparm("magic"));
	feedham = atoi(getparm("feed"));
	
	if (feedham) {
		html_header(1);
		printf("<body><center>\n");
		yctx = yspam_init("127.0.0.1");
		ret =yspam_feed_ham(yctx, currentuser->userid, id, magic);
		yspam_fini(yctx);
		if (ret == 0)
			printf("成功");
		else
			printf("真失败 %d",ret);
		printf("<br><a href=bbsspam>返回垃圾邮件列表</a>");
		printf("</center></body>\n");
		http_quit();
	}
	snprintf(path, sizeof(path), MY_BBS_HOME"/maillog/%llu.bbs", id);
	if (*getparm("attachname") == '/') {
		showbinaryattach(path);
		return 0;
	}
	html_header(1);
	yctx = yspam_init("127.0.0.1");
	ret = yspam_getspam(yctx, currentuser->userid, id, magic, title, sender);
	yspam_fini(yctx);
	if (ret < 0) {
		printf("%d", ret);
		http_quit();
		http_fatal("内部错误");	
	}
	printf("<body><center>\n");
	printf("%s -- 垃圾信件 [使用者: %s]<hr>\n", BBSNAME, currentuser->userid);
	if (title[0] == 0)
		printf("</center>标题: 无题<br>");
	else
		printf("</center>标题: %s<br>", void1(titlestr(title)));
	printf("发信人: %s<br>", void1(titlestr(sender)));
	printf("<center>");
	showcon(path);
	sprintf(idbuf, "%llu", id);
	printf("这不是一封垃圾邮件，<a onclick='return confirm(\"这封信真的不是垃圾邮件吗？您的准确判断将有助于我们改善系统\")' href=bbsspamcon?id=%s&magic=%d&feed=1>请还给我</a><br>", idbuf, magic);
	printf("<a href=bbsspam>返回垃圾邮件列表</a>");
	printf("</center></body>\n");
	http_quit();
	return 0;
}
