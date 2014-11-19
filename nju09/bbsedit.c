#include "bbslib.h"

int
bbsedit_main()
{
	FILE *fp;
	int type = 0;
	char buf[512], path[512], file[512], board[512], title[80];
	int base64, isa = 0, len;
	char *fn = NULL;
	struct boardmem *brd;
	struct fileheader x;
	int filetime, ret, ent;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("匆匆过客不能修改文章，请先登录");
	changemode(EDIT);
	getparmboard(board, sizeof(board));
	strsncpy(title, getparm("title"), 60);
	type = atoi(getparm("type"));
	brd = getboard(board);
	if (brd == 0)
		http_fatal("错误的讨论区");
	strsncpy(file, getparm("F"), 30);
	if (!file[0])
		strsncpy(file, getparm("file"), 30);
	filetime = atoi(file + 2);
	if (strstr(file, "..") || strstr(file, "/"))
		http_fatal("错误的参数");
	if (!has_post_perm(currentuser, brd))
		http_fatal("错误的讨论区或者您无权在此讨论区发表文章");
	if (noadm4political(board))
		http_fatal("对不起,因为没有版面管理人员在线,本版暂时封闭.");
	sprintf(path, "boards/%s/.DIR", board);
	ret = get_fileheader_records(path, filetime, NA_INDEX, 1, &x, &ent, 1);
	if (ret == -1)
		http_fatal("错误的参数");
	if (strcmp(x.owner, currentuser->userid)
	    && (!has_BM_perm(currentuser, brd)))
		http_fatal("你无权修改此文章");
	if (!strcmp(board, "syssecurity"))
		http_fatal("你无权修改系统记录");
	if (brd->header.flag & IS1984_FLAG)
		http_fatal("本讨论区文章禁止修改");
	printf("<center>%s -- 修改文章 [使用者: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	if (type != 0)
		return update_form(board, file, title);
	printf("<table border=1>\n");
	printf("<tr><td>");
	printf("<tr><td><form name=form1 method=post action=bbsedit>\n");
	printf
	    ("使用标题: <input type=text name=title size=40 maxlength=100 value='%s'> 讨论区: %s<br>\n",
	     void1(nohtml(x.title)), board);
	printf("本文作者：%s<br>\n", fh2owner(&x));
	printusemath(x.accessed & FH_MATH);
	printuploadattach();
	printf("&nbsp;&nbsp;");
	printubb("form1", "text");
	sprintf(path, "boards/%s/%s", board, file);
	fp = fopen(path, "r");
	if (fp == 0)
		http_fatal("文件丢失");
	snprintf(path, sizeof (path), PATHUSERATTACH "/%s",
		 currentuser->userid);
	clearpath(path);
	keepoldheader(FCGI_ToFILE(fp), SKIPHEADER);
	printf
	    ("<br>\n<textarea  onkeydown='if(event.keyCode==87 && event.ctrlKey) {document.form1.submit(); return false;}'  onkeypress='if(event.keyCode==10) return document.form1.submit()' name=text rows=20 cols=76 wrap=virtual class=f2>\n");
	underline = 0;
	highlight = 0;
	lastcolor = 37;
	useubb = 1;
	while (1) {
		if (fgets(buf, 500, fp) == 0)
			break;
		if (isa && (!strcmp(buf, "\r\n") || !strcmp(buf, "\n")))	//附件之后吞一个空行
			continue;
		base64 = isa = 0;
		if (!strncmp(buf, "begin 644", 10)) {
			isa = 1;
			base64 = 1;
			len = 0;
			fn = buf + 10;
		} else if (checkbinaryattach(buf, FCGI_ToFILE(fp), &len)) {
			isa = 1;
			base64 = 0;
			fn = buf + 18;
		}
		if (isa) {
			if (!getattach
			    (FCGI_ToFILE(fp), buf, fn, path, base64, len, 0)) {
				printf("#attach %s\n", fn);
			}
		} else
			ansi2ubb(buf, strlen(buf), stdout);
	}
	fclose(fp);
	printf("</textarea>\n");
	printf("<tr><td class=post align=center>\n");
	printf("<input type=hidden name=type value=1>\n");
	printf("<input type=hidden name=board value=%s>\n", board);
	printf("<input type=hidden name=file value=%s>\n", file);
	printf("<input type=submit value=存盘> \n");
	printf("<input type=reset value=重置></form>\n");
	printf("</table>");
	http_quit();
	return 0;
}

int
Origin2(text)
char text[256];
{
	char tmp[STRLEN];

	sprintf(tmp, ":．%s %s．[FROM:", BBSNAME, BBSHOST);
	if (strstr(text, tmp))
		return 1;
	sprintf(tmp, ":．%s %s [FROM:", BBSNAME, "http://" MY_BBS_DOMAIN);
	if (strstr(text, tmp))
		return 1;
	else
		return 0;
}

int
update_form(char *board, char *file, char *title)
{
	FILE *fp;
	char *buf = getparm("text"), path[80];
	int num = 0, filetime;
	int usemath, useattach, use_ubb;
	char dir[STRLEN];
	char filename[STRLEN], filename2[STRLEN];
	struct fileheader x;
	struct mmapfile mf = { ptr:NULL };
	int dangerous = 0;
	int i, ret;
	filetime = atoi(file + 2);
	usemath = strlen(getparm("usemath"));
	use_ubb = strlen(getparm("useubb"));
	if (usemath)
		usemath = testmath(buf);

	for (i = 0; i < strlen(title); i++)
		if (title[i] <= 27 && title[i] >= -1)
			title[i] = ' ';
	i = strlen(title) - 1;
	while (i >= 0 && isspace(title[i])) {
		title[i] = 0;
		i--;
	}
	if (title[0] == 0)
		http_fatal("标题不能为空");
	if (!hideboard(board)) {
		dangerous = dofilter_edit(title, buf, political_board(board));
		if (dangerous == 1) {
			mail_buf(buf, strlen(buf), currentuser->userid, title, currentuser->userid);
			http_fatal(BAD_WORD_NOTICE);
		} else if (dangerous == 2) {
		#ifdef POST_WARNING
			char mtitle[256];
			sprintf(mtitle, "[修改报警] %s %.60s", board, title);
			mail_buf(buf, strlen(buf), "delete", mtitle, currentuser->userid);
			updatelastpost("deleterequest");
		#endif
		}
	}
	sprintf(filename, "bbstmpfs/tmp/%d.tmp", thispid);
	sprintf(filename2, "bbstmpfs/tmp/%d.tmp2", thispid);
	if (use_ubb)
		ubb2ansi(buf, filename2);
	else
		f_write(filename2, buf);
	useattach =
	    (insertattachments_byfile
	     (filename, filename2, currentuser->userid));
	unlink(filename2);
	sprintf(path, "boards/%s/%s", board, file);
	fp = fopen(path, "r+");
	if (fp == 0)
		http_fatal("无法存盘");
	keepoldheader(FCGI_ToFILE(fp), SKIPHEADER);
	/*
	   i = 0;
	   while (buf[i]) {
	   if (buf[i] == '\r') {
	   fprintf(fp, "\n");
	   if (buf[i + 1] == '\n')
	   i++;
	   } else
	   fwrite(buf + i, 1, 1, fp);
	   i++;
	   } */
	mmapfile(filename, &mf);
	fwrite(mf.ptr, mf.size, 1, fp);
	mmapfile(NULL, &mf);
	unlink(filename);
	ftruncate(fileno(fp), ftell(fp));
	fclose(fp);
	add_edit_mark(path, currentuser->userid, now_t, fromhost);
	sprintf(dir, "boards/%s/.DIR", board);
	fp = fopen(dir, "r");
	if (fp == 0)
		http_fatal("错误的参数");
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &num, 1);
	if (ret == 0) {
		x.edittime = now_t;
		x.sizebyte = numbyte(eff_size(path));
		strsncpy(x.title, title, sizeof (x.title));
		if (usemath)
			x.accessed |= FH_MATH;
		else
			x.accessed &= ~FH_MATH;
		if (useattach)
			x.accessed |= FH_ATTACHED;
		else
			x.accessed &= ~FH_ATTACHED;
		if (dangerous)
			x.accessed |= FH_DANGEROUS;
		change_dir(dir, &x, (void *) DIR_do_substitution, num + 1, 0 , 1, NULL);
		updatelastpost(board);
	}
	if (innd_board(board))
		outgo_post(&x, board, currentuser->userid,
			   currentuser->username);
	printf("修改文章成功.<br><a href=bbsdoc?B=%d>返回本讨论区</a>",
	       getbnum(board));
	return 0;
}

int
getpathsize(char *path)
{
	DIR *pdir;
	struct dirent *pdent;
	char fname[1024];
	int totalsize = 0, size;
	pdir = opendir(path);
	if (!pdir)
		return -1;
	while ((pdent = readdir(pdir))) {
		if (!strcmp(pdent->d_name, "..") || !strcmp(pdent->d_name, "."))
			continue;
		if (strlen(pdent->d_name) + strlen(path) >= sizeof (fname)) {
			totalsize = -1;
			break;
		}
		sprintf(fname, "%s/%s", path, pdent->d_name);
		size = file_size(fname);
		if (size < 0) {
			totalsize = -1;
			break;
		}
		totalsize += size;
	}
	closedir(pdir);
	return totalsize;
}
