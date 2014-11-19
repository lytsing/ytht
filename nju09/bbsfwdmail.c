#include "bbslib.h"

int
bbsfwdmail_main()
{
	struct fileheader x;
	char file[80], target[80], dir[80], path[80], buf[256];
	char bbsfwdmail_tmpfn[80];
	char *ptr;
	struct userdata currentdata;
	struct userec *up;
	FILE *fp, *fp1;
	int ent, ret, filetime;
	int i, retv;
	html_header(1);
	check_msg();
	strsncpy(file, getparm("F"), 30);
	if (!file[0])
		strsncpy(file, getparm("file"), 30);
	filetime = atoi(file + 2);
	strsncpy(target, getparm("target"), 30);
	if (!loginok || isguest)
		http_fatal("匆匆过客不能进行本项操作");
	changemode(SMAIL);
	sprintf(dir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]),
		currentuser->userid);
	if ((ptr=check_mailperm(currentuser)))
		http_fatal(ptr);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &ent, 1);
	if (ret == -1)
		http_fatal("错误的信件名");
	printf("<center>%s -- 转寄/推荐给好友 [使用者: %s]<hr>\n", BBSNAME,
	       currentuser->userid);

	if (target[0]) {
		if (!strstr(target, "@")) {
			if (getuser(target, &up) <= 0)
				http_fatal("错误的使用者帐号");
			strcpy(target, up->userid);
		}
		snprintf(bbsfwdmail_tmpfn, 80, "bbstmpfs/tmp/bbsfwdmail.%s.%d",
			 currentuser->userid, thispid);
		snprintf(path, 80, "mail/%c/%s/%s",
			 mytoupper(currentuser->userid[0]), currentuser->userid,
			 fh2fname(&x));
		fp = fopen(bbsfwdmail_tmpfn, "w");
		fp1 = fopen(path, "r");
		if (!fp || !fp1) {
			if(fp)
				fclose(fp);
			if(fp1)
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
			"\033[m\033[1m【 以下文字转寄自 \033[32m%s \033[m\033[1m的信箱 】\n",
			currentuser->userid);
		fprintf(fp,
			"\033[m\033[1m【 原文由 \033[32m%s \033[m\033[1m于 \033[0m%s\033[1m 发表 】\033[m\n",
			fh2owner(&x), Ctime(x.filetime));
		while (1) {
			retv = fread(buf, 1, sizeof (buf), fp1);
			if (retv <= 0)
				break;
			fwrite(buf, 1, retv, fp);
		}
		fprintf(fp,
		"--\n\033[m\033[1;32m※ 转寄:．%s %s．[FROM: %-.20s]\033[m\n",
		MY_BBS_NAME, MY_BBS_DOMAIN, fromhost);
		fclose(fp1);
		fclose(fp);
		do_fwdmail(bbsfwdmail_tmpfn, &x, target);
		unlink(bbsfwdmail_tmpfn);
		return 0;
	}
	loaduserdata(currentuser->userid, &currentdata);
	printf("<table><tr><td>\n");
	printf("文章标题: %s<br>\n", nohtml(x.title));
	printf("文章作者: %s<br>\n", fh2owner(&x));
	printf("信件出处: %s 的信箱<br>\n", currentuser->userid);
	printf("<form action=bbsfwdmail method=post>\n");
	printf("<input type=hidden name=file value=%s>", file);
	printf
	    ("把文章转寄给 <input name=target size=30 maxlength=30 value='%s'> (请输入对方的id或email地址). <br>\n",
	     void1(nohtml(currentdata.email)));
	printf("<input type=submit value=确定转寄></form>");
	return 0;
}

int
do_fwdmail(char *fn, struct fileheader *x, char *target)
{
	char title[512];
	if (!file_exist(fn))
		http_fatal("信件内容已丢失, 无法转寄");
	sprintf(title, "[转寄] %s", x->title);
	title[60] = 0;
	post_mail(target, title, fn, currentuser->userid,
		  currentuser->username, fromhost, -1, 0);
	printf("文章已转寄给'%s'<br>\n", nohtml(target));
	printf("[<a href='javascript:history.go(-2)'>返回</a>]");
	return 0;
}

