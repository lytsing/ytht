#include "bbslib.h"

int
bbscccmail_main()
{
	struct fileheader x;
	struct boardmem *brd;
	char file[80], target[80];
	char dir[80];
	int ent;
	int ret;
	int filetime;
	html_header(1);
	check_msg();
	strsncpy(file, getparm("F"), 30);
	if (!file[0])
		strsncpy(file, getparm("file"), 30);
	filetime = atoi(file + 2);
	strsncpy(target, getparm("target"), 30);
	if (!loginok || isguest)
		http_fatal("匆匆过客不能进行本项操作");
	changemode(POSTING);
	sprintf(dir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]), currentuser->userid);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &ent, 1);
	if (ret == -1)
		http_fatal("错误的文件名");
	printf("<center>%s -- 转载文章 [使用者: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	if (target[0]) {
		brd = getboard(target);
		if (brd == 0)
			http_fatal("错误的讨论区名称或你没有在该版发文的权限");
		if (!has_post_perm(currentuser, brd))
			http_fatal("错误的讨论区名称或你没有在该版发文的权限");
		if (noadm4political(target))
			http_fatal
			    ("对不起,因为没有版面管理人员在线,本版暂时封闭.");
		return do_cccmail(&x, brd);
	}
	printf("<table><tr><td>\n");
	printf("<font color=red>转贴发文注意事项:<br>\n");
	printf
	    ("本站规定同样内容的文章严禁在 6 个或 6 个以上讨论区内重复发表。");
	printf("违者将被封禁在本站发文的权利<br><br></font>\n");
	printf("文章标题: %s<br>\n", nohtml(x.title));
	printf("文章作者: %s<br>\n", fh2owner(&x));
	printf("文章出处: %s 的信箱<br>\n", currentuser->userid);
	printf("<form action=bbscccmail method=post>\n");
	printf("<input type=hidden name=file value=%s>", file);
	printf("转载到 <input name=target size=30 maxlength=30> 讨论区. ");
	printf("<input type=submit value=确定></form>");
	return 0;
}

int
do_cccmail(struct fileheader *x, struct boardmem *brd)
{
	FILE *fp, *fp2;
	char board[80], title[512], buf[512], path[200], path2[200],
	    i;
	char tmpfn[80];
	int retv;
	int mark = 0;
	strcpy(board, brd->header.filename);
	sprintf(path, "mail/%c/%s/%s", mytoupper(currentuser->userid[0]), currentuser->userid, fh2fname(x));
	if (brd->header.flag & IS1984_FLAG)
		http_fatal("该版面禁止转载");
	fp = fopen(path, "r");
	if (fp == 0)
		http_fatal("信件内容已丢失, 无法转载");
	sprintf(path2, "bbstmpfs/tmp/%d.tmp", thispid);
	fp2 = fopen(path2, "w");
	if (fgets(buf, 256, fp) != 0) {
		if (!strncmp(buf, "发信人: ", 8) || !strncmp(buf, "寄信人: ", 8)) {
			for (i = 0; i < 6; i++) {
				if (fgets(buf, 256, fp) == 0)
					break;
				if (buf[0] == '\n')
					break;
			}
		}
		else
			rewind(fp);
	}
	fprintf(fp2,
		"\033[m\033[1m【 以下文字转载自 \033[32m%s \033[m\033[1m的信箱 】\n",
		currentuser->userid);
	fprintf(fp2,
		"\033[m\033[1m【 原文由 \033[32m%s \033[m\033[1m于 \033[0m%s\033[1m 发表 】\033[m\n",
		fh2owner(x), Ctime(x->filetime));
	while (1) {
		retv = fread(buf, 1, sizeof (buf), fp);
		if (retv <= 0)
			break;
		fwrite(buf, 1, retv, fp2);
	}
	fclose(fp);
	fclose(fp2);
	if (!strncmp(x->title, "[转载]", 6)) {
		strsncpy(title, x->title, sizeof (title));
	} else {
		sprintf(title, "[转载] %.55s", x->title);
	}
	sprintf(tmpfn, "bbstmpfs/tmp/filter.%s.%d", currentuser->userid, thispid);
	copyfile(path2, tmpfn);
	filter_attach(tmpfn);
	if (dofilter(title, tmpfn, 2)) {
	#ifdef POST_WARNING
		char mtitle[256];
		sprintf(mtitle, "[转载报警] %s %.60s", board, title);
		mail_file(path2, "delete", mtitle, currentuser->userid);
		updatelastpost("deleterequest");
	#endif
		mark |= FH_DANGEROUS;
	}
	unlink(tmpfn);
	post_article(board, title, path2, currentuser->userid,
		     currentuser->username, fromhost, -1, mark, 0,
		     currentuser->userid, -1);
	unlink(path2);
	printf("'%s' 已转贴到 %s 版.<br>\n", nohtml(title), board);
	printf("[<a href='javascript:history.go(-2)'>返回</a>]");
	return 0;
}

