#include "bbslib.h"

int
bbsmailcon_main()
{
	FILE *fp = NULL;
	char dir[80], file[80], path[80], *id;
	struct fileheader x;
	int num, total = 0;
	if ((!loginok || isguest) && (!tempuser))
		http_fatal("请先登录%d", tempuser);
	//if (tempuser) http_fatal("user %d", tempuser);
	changemode(RMAIL);
	strsncpy(file, getparm("file"), 32);
	num = atoi(getparm("num")) - 1;
	id = currentuser->userid;
	if (strncmp(file, "M.", 2))
		http_fatal("错误的参数1");
	if (strstr(file, "..") || strstr(file, "/"))
		http_fatal("错误的参数2");
	sprintf(path, "mail/%c/%s/%s", mytoupper(id[0]), id, file);
	if (*getparm("attachname") == '/') {
		showbinaryattach(path);
		return 0;
	}
	if (!tempuser && num != -1) {
		sprintf(dir, "mail/%c/%s/.DIR", mytoupper(id[0]), id);
		total = file_size(dir) / sizeof (x);
		if (total <= 0)
			http_fatal("错误的参数3 %s", dir);
		fp = fopen(dir, "r+");
		if (fp == 0)
			http_fatal("dir error2");
		fseek(fp, sizeof (x) * num, SEEK_SET);
		if (1 != fread(&x, sizeof (x), 1, fp)) {
			fclose(fp);
			http_fatal("dir error3");
		}
	}
	html_header(1);
	check_msg();
	printf("<body><center>\n");
	printf("%s -- 阅读信件 [使用者: %s]<hr>\n", BBSNAME, id);
	if (!tempuser) {
		printf("</center>标题: %s<br>", void1(titlestr(x.title)));
		printf("发信人: %s<br>", titlestr(x.owner));
	}
	if (loginok && !isguest && !(currentuser->userlevel & PERM_LOGINOK) &&
	    !tempuser && !strncmp(x.title, "<注册失败>", 10)
	    && !has_fill_form(currentuser->userid))
		printf
		    ("--&gt;<a href=bbsform><font color=RED size=+1>重新填写注册单</font></a>&lt;--\n<br>  <input type=button value=查看帮助 onclick=\"javascript:{open('/reghelp.html','winreghelp','width=600,height=460,resizeable=yes,scrollbars=yes');}\"><br>");
	printf("<center>");
	sprintf(path, "mail/%c/%s/%s", mytoupper(id[0]), id, file);
	showcon(path);
	if (!tempuser && num != -1) {
		printf
		    ("[<a onclick='return confirm(\"你真的要删除这封信吗?\")' href=bbsdelmail?file=%s>删除</a>]",
		     file);
		printf
		    ("[<a onclick='return confirm(\"你真的认为这封信是垃圾邮件吗? 如果不是，请使用删除信件功能\")' href=bbsdelmail?spam=1&file=%s>垃圾邮件</a>]",
		     file);
		printf("[<a href=bbsfwdmail?F=%s>转寄</a>]", fh2fname(&x));
		printf("[<a href=bbscccmail?F=%s>转贴</a>]", fh2fname(&x));
		if (num > 0) {
			fseek(fp, sizeof (x) * (num - 1), SEEK_SET);
			fread(&x, sizeof (x), 1, fp);
			printf("[<a href=bbsmailcon?file=%s&num=%d>上一篇</a>]",
			       fh2fname(&x), num);
		}
		printf("[<a href=bbsmail>返回信件列表</a>]");
		if (num < total - 1) {
			fseek(fp, sizeof (x) * (num + 1), SEEK_SET);
			fread(&x, sizeof (x), 1, fp);
			printf("[<a href=bbsmailcon?file=%s&num=%d>下一篇</a>]",
			       fh2fname(&x), num + 2);
		}
		if (num >= 0 && num < total) {
			fseek(fp, sizeof (x) * num, SEEK_SET);
			if (fread(&x, sizeof (x), 1, fp) > 0
			    && !(x.accessed & FH_READ)) {
				x.accessed |= FH_READ;
				fseek(fp, sizeof (x) * num, SEEK_SET);
				fwrite(&x, sizeof (x), 1, fp);
				printf
				    ("<script>top.f4.location.reload();</script>");
			}
		}
		printf("[<a href='bbspstmail?file=%s&num=%d&reply=1'>回信</a>]",
		       fh2fname(&x), num + 1);
		fclose(fp);
	}
	printf("</center></body>\n");
	http_quit();
	return 0;
}
