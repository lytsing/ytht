#include "bbslib.h"

char bbsfwd_tmpfn[80];

int
bbsfwd_main()
{
	struct fileheader x;
	char board[80], file[80], maildir[80], target[80], dir[80], path[80], buf[256];
	char *ptr;
	struct userdata currentdata;
	struct userec *up;
	FILE *fp, *fp1;
	int i, retv, filetime;
	int ret, ent;
	html_header(1);
	check_msg();
	getparmboard(board, sizeof(board));
	strsncpy(file, getparm("F"), 30);
	if (!file[0])
		strsncpy(file, getparm("file"), 30);
	filetime = atoi(file + 2);
	strsncpy(target, getparm("target"), 30);
	if (!loginok || isguest)
		http_fatal("匆匆过客不能进行本项操作");
	changemode(SMAIL);
	sprintf(maildir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]),
		currentuser->userid);
	if ((ptr=check_mailperm(currentuser)))
		http_fatal(ptr);
	if (!getboard(board))
		http_fatal("错误的讨论区");
	sprintf(dir, "boards/%s/.DIR", board);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &ent, 1);
	if (ret == -1)
		http_fatal("错误的文件名");
	printf("<center>%s -- 转寄/推荐给好友 [使用者: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	if (target[0]) {
		if (!strstr(target, "@")) {
			if (getuser(target, &up) <= 0)
				http_fatal("错误的使用者帐号");
			strcpy(target, up->userid);
		}
		snprintf(bbsfwd_tmpfn, 80, "bbstmpfs/tmp/bbsfwd.%s.%d",
			 currentuser->userid, thispid);
		sprintf(path, "boards/%s/%s", board, fh2fname(&x));
		fp = fopen(bbsfwd_tmpfn, "w");
		fp1 = fopen(path, "r");
		if (!fp || !fp1) {
			if (fp)
				fclose(fp);
			if (fp1)
				fclose(fp1);
			http_fatal
			    ("信件内容已丢失，或由于其他内部错误导致转寄失败");
		}
		if (fgets(buf, 256, fp1) != 0) {
			if (!strncmp(buf, "发信人: ", 8)
			    || !strncmp(buf, "寄信人: ", 8)) {
				for (i = 0; i < 6; i++) {
					if (fgets(buf, 256, fp1) == 0)
						break;
					if (buf[0] == '\n')
						break;
				}
			} else
				rewind(fp1);
		}
		fprintf(fp,
			"\033[m\033[1m【 以下文字转载自 \033[32m%s \033[m\033[1m讨论区 】\n",
			hideboard(board) ? "隐藏" : board);
		fprintf(fp,
			"\033[m\033[1m【 原文由 \033[32m%s \033[m\033[1m于 \033[0m%s\033[1m 发表 】\033[m\n",
			fh2owner(&x), Ctime(x.filetime));
		while (1) {
			retv = fread(buf, 1, sizeof (buf), fp1);
			if (retv <= 0)
				break;
			fwrite(buf, 1, retv, fp);
		}
		fclose(fp1);
		fclose(fp);
		do_fwd(&x, board, target);
		unlink(bbsfwd_tmpfn);
		return 0;

	}
	loaduserdata(currentuser->userid, &currentdata);
	printf("<table><tr><td>\n");
	printf("文章标题: %s<br>\n", nohtml(x.title));
	printf("文章作者: %s<br>\n", fh2owner(&x));
	printf("原讨论区: %s<br>\n", board);
	printf("<form action=bbsfwd method=post>\n");
	printf("<input type=hidden name=board value=%s>", board);
	printf("<input type=hidden name=file value=%s>", file);
	printf
	    ("把文章转寄给 <input name=target size=30 maxlength=30 value='%s'> (请输入对方的id或email地址). <br>\n",
	     void1(nohtml(currentdata.email)));
	printf("<input type=submit value=确定转寄></form>");
	return 0;
}

int
do_fwd(struct fileheader *x, char *board, char *target)
{
	char title[512];
	if (!file_exist(bbsfwd_tmpfn))
		http_fatal("文件内容已丢失, 无法转寄");
	sprintf(title, "[转寄] %s", x->title);
	title[60] = 0;
	post_mail(target, title, bbsfwd_tmpfn, currentuser->userid,
		  currentuser->username, fromhost, -1, 0);
	printf("文章已转寄给'%s'<br>\n", nohtml(target));
	printf("[<a href='javascript:history.go(-2)'>返回</a>]");
	return 0;
}
