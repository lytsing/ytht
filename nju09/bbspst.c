#include "bbslib.h"

int
bbspst_main()
{
	int local_article, num, fullquote = 0, guestre = 0, thread = -1;
	char *ptr, userid[80], buf[512], path[512], file[512], board[512],
	    title[80] = "";
	struct fileheader dirinfo;
	struct boardmem *x;
	int filetime;
	int ret, ent;
	extern int cssv;
	html_header(1);
	printf("<script src=" CSSPATH "richtext.js></script>");
	check_msg();
	if (!loginok) {
		printf("匆匆过客不能发表文章，请先登录!<br>");
		printf
		    ("如果您确实已经登录却看到这个提示，请尝试了解登录时的 IP验证范围 选项<br>");
		printf("<script>openlog();</script>");
		http_quit();
	}
	local_article = atoi(getparm("la"));
	getparmboard(board, sizeof (board));
	if (*(ptr = getparm("M"))) {
		int mode = atoi(ptr) ? 1 : 0;
		if (mode != w_info->edit_mode) {
			w_info->edit_mode = mode;
			sprintf(buf, "%d", mode);
			saveuservalue(currentuser->userid, "edit_mode", buf);
		}
	}
	strsncpy(file, getparm("F"), 20);
	if (!file[0])
		strsncpy(file, getparm("file"), 20);
	num = atoi(getparm("num")) - 1;
	fullquote = atoi(getparm("fullquote"));
	if (file[0] != 'M' && file[0])
		http_fatal("错误的文件名");
	filetime = atoi(file + 2);
	if (!(x = getboard(board)))
		http_fatal("错误的讨论区或者您无权在此讨论区发表文章");
	if (njuinn_board(board) && !innd_board(board))
		local_article = 1;
	if (!has_post_perm(currentuser, x) && !isguest)
		http_fatal("错误的讨论区或者您无权在此讨论区发表文章");
	if (noadm4political(board))
		http_fatal("对不起,因为没有版面管理人员在线,本版暂时封闭.");
	if (x->ban == 2)
		http_fatal("对不起, 因为版面文章超限, 本版暂时关闭.");

	if (file[0]) {
		sprintf(path, "boards/%s/.DIR", board);
		ret =
		    get_fileheader_records(path, filetime, num, 1, &dirinfo,
					   &ent, 1);
		if (ret == -1)
			http_fatal("错误的文件名");
		num = ent;
		if (dirinfo.accessed & FH_NOREPLY)
			http_fatal("本文被设为不可Re模式");
		thread = dirinfo.thread;
		if (dirinfo.accessed & FH_ALLREPLY)
			guestre = 1;
		strsncpy(userid, fh2owner(&dirinfo), 20);
		if (strncmp(dirinfo.title, "Re: ", 4))
			snprintf(title, 60, "Re: %s", dirinfo.title);
		else
			strsncpy(title, dirinfo.title, 60);

	}
	if (isguest && !guestre) {
		printf("匆匆过客不能发表文章，请先登录!<br><br>");
		printf("<script>openlog();</script>");
		http_quit();
	}
	changemode(POSTING);
	printf("<script>\nfunction submitForm() {updateRTE('rte1');"
	       "document.form1.text.value=html2ansi(document.form1.rte1.value);"
	       "return false;}\ninitRTE('/images/', '/', '%s?%d');</script>\n",
	       currstyle->cssfile, cssv);
	printf("<script>function setSFEvents(rtename) {");
	if (!strcmp(currentuser->userid, "ylsdd")&&0) {
		printf
		    ("var o=document.getElementById(rtename).contentWindow.document;"
		     "if(!o.addEventListener){"
		     "o.onkeypress=respondKeypress;o.onkeydown=respondKeydown;o.onfocus=respondFocusin;} else {"
		     "o.addEventListener(\"keypress\", respondKeypress, false);"
		     "o.addEventListener(\"keydown\",respondKeydown, false);"
		     "o.addEventListener(\"focus\",respondFocusin, false);}");
	}
	printf("}</script>");
	printf("</head>");
	if (w_info->edit_mode == 0) {
		printf("<body>");
	} else {
		printf
		    ("<body onLoad='enableDesignMode(\"rte1\", ansi2html(document.form1.text.value), false);setSFEvents(\"rte1\");'>\n");
	}
	printf("<div class=swidth style=\"height:99%%\">");
	printf("<table height=100%% width=100%% border=0><tr height=20><td>");	//大表
	printf("%s -- 发表文章于 %s 讨论区\n", BBSNAME, board);
	strsncpy(buf, getsenv("QUERY_STRING"), sizeof (buf));
	if ((ptr = strstr(buf, "&M=")))
		*ptr = 0;
	if (w_info->edit_mode == 0) {
		printf
		    ("<script>if(isRichText) document.write(' [<a href=bbspst?%s&M=1 class=red>切换到所见即所得编辑器</a>');</script>]",
		     buf);
	} else {
		printf(" [<a href=bbspst?%s&M=0>切换到简单编辑器</a>]", buf);
		printf
		    ("<script>if(isRichText!=true||!testReplace()) location.replace('bbspst?%s&M=0')</script>",
		     buf);
	}
	if (x->header.flag & IS1984_FLAG)
		printf
		    ("<br><font color=red>请注意，本文发表后需通过审查</font>");
	printf("</td></tr><tr><td valign=top>");	//大表
	printf("<table border=1 width=100%% height=100%%>");	//中表
	if (file[0])
		snprintf(buf, sizeof (buf), "&ref=%s&rid=%d", file, num + 1);
	//form 放在 table 跟 tr 之间，否则显示不好
	printf("<form name=form1 method=post action=bbssnd?B=%d&th=%d%s>\n",
	       getbnumx(x), thread, file[0] ? buf : "");
	printf("<tr height=40><td>");	//中表
	printf
	    ("使用标题: <input type=text name=title size=50 maxlength=100 value='%s'>",
	     void1(nohtml(title)));
	if (innd_board(board) || njuinn_board(board))
		printf
		    (" <font class=red>转信</font><input type=checkbox name=outgoing %s>\n",
		     local_article ? "" : "checked");
	if (anony_board(board))
		printf("匿名<input type=checkbox name=anony %s>\n",
		       strcmp(board, DEFAULTANONYMOUS) ? "" : "checked");

	printf("<table width=100%%><tr><td>");
	printselsignature();
	printf("</td><td align=right>");
	printuploadattach();
	printf("<br>");
	printusemath(0);
	printf("</td></tr></table>");

	if (!w_info->edit_mode) {
		printubb("form1", "text");
		printf("<br>");
	}
	if (file[0]) {
		printf("引文模式: %s ", fullquote ? "完全" : "精简");
		printf
		    ("[<a target=_self href=bbspst?inframe=1&B=%d&file=%s&num=%d&la=%d",
		     getbnumx(x), file, num + 1, local_article);
		printf("&fullquote=%d>使用%s引文(将丢弃所更改内容)</a>]<br>",
		       !fullquote, (!fullquote) ? "完全" : "精简");
	}
	printf("</td></tr><tr><td>");	//中表
	if (w_info->edit_mode)
		printf
		    ("<div style='position:absolute;visibility:hidden;display:none'>"
		     "<textarea name=text>");
	else
		printf
		    ("<br>\n<textarea "
		     "onkeydown='if(event.keyCode==87 && event.ctrlKey) {document.form1.submit(); return false;}' "
		     "onkeypress='if(event.keyCode==10) return document.form1.submit()' "
		     "name=text rows=20 cols=76 wrap=virtual class=f2 >\n\n");
	if (file[0]) {
		sprintf(path, "boards/%s/%s", board, file);
		printquote(path, board, userid, fullquote);
	}
	printf("</textarea>");
	if (w_info->edit_mode)
		printf("</div>");
	if (w_info->edit_mode) {
		if (!strcmp(currentuser->userid, "ylsdd")&&0) {
			printf
			    ("<script>writeRichText('rte1','<'+'script id=\"imeScript\" type=\"text/javascript\" src=\"/sf_ime/script/sf_ime_py.js\"></script'+'>',0,'100%%',true,false);</script>");
		} else {
			printf
			    ("<script>writeRichText('rte1','',0,'100%%',true,false);</script>");
		}
	}
	printf("</td></tr><tr height=30><td class=post align=center>");	//中表
	if (w_info->edit_mode)
		printf
		    ("<input type=submit value=发表 onclick=\"this.value='文章提交中，请稍候...';"
		     "this.disabled=true;submitForm();form1.submit();\">");
	else
		printf
		    ("<table width=100%%><tr><td align=center>"
		     "<input type=submit value=发表 onclick=\"this.value='文章提交中，请稍候...';"
		     "this.disabled=true;form1.submit();\"> </td><td align=center>"
		     "<input type=reset value=清除 onclick='return confirm(\"确定要全部清除吗?\")'>"
		     "</td></tr></table>");
	printf("</td></tr></form></table>"	//中表
	       "</td></tr></table></div>");	//大表
	if (!strcmp(currentuser->userid, "ylsdd")&&0) {
		printf
		    ("<script id=\"imeScript\" type=\"text/javascript\" src=\"/sf_ime/script/sf_ime_py.js\"></script>");
	}
	printf("</body>");
	http_quit();
	return 0;
}

int
printquote(char *filename, char *board, char *userid, int fullquote)
{
	int lines = 0, i;
	char buf[512];
      struct mmapfile mf = { ptr:NULL };
	struct memline ml;
	underline = 0;
	highlight = 0;
	lastcolor = 37;
	useubb = 1;
	if (mmapfile(filename, &mf) < 0)
		return -1;
	printf("\n\n【 在 %s 的大作中提到: 】\n", userid);
	MMAP_TRY {
		memlineinit(&ml, mf.ptr, mf.size);
		for (i = 0; i < 6; i++) {
			if (!memlinenext(&ml))
				break;
			if (ml.text[0] == '\r' || ml.text[0] == '\n')
				break;
		}
		while (memlinenext(&ml)) {
			if (ml.text[0] == '\n' || ml.text[0] == '\r')
				continue;
			if (ml.len > 4 && (!strncmp(ml.text, ": 【", 4)
					   || !strncmp(ml.text, ": : ", 4)))
				continue;
			if (ml.len == 3 && !strncmp(ml.text, "--\n", 3))
				break;
			if (ml.len > 10 && !strncmp(ml.text, "begin 644 ", 10))
				break;
			if (ml.len > 18
			    && !strncmp(ml.text, "beginbinaryattach ", 18))
				break;
			//用ml.len而不是ml.len+1来丢弃最后一个\n，而长行的最后反正是要丢弃的，没问题
			strsncpy(buf, ml.text, min(sizeof (buf), ml.len));
			filteransi2(buf);
			strrtrim(buf);
			if (!*buf)
				continue;
			if (!fullquote && strlen(buf) > min(7 - lines, 6) * 50)
				buf[min(7 - lines, 6) * 50] = 0;
			void1(buf);
			printf(": %s\n", nohtml_textarea(buf));
			lines += 1 + strlen(buf) / 50;
			if (!fullquote && lines >= 7) {
				printf(": (以下引言省略...)\n");
				break;
			}
		}
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	return 0;
}

void
printselsignature()
{
	int i, sigln, numofsig;
	char path[200];
	sprintf(path, "home/%c/%s/signatures",
		mytoupper(currentuser->userid[0]), currentuser->userid);
	sigln = countln(path);
	numofsig = (sigln + MAXSIGLINES - 1) / MAXSIGLINES;
	printf
	    ("<a target=_blank href=bbssig>签名档</a> <select name=\"signature\">\n");
	if (u_info->signature == 0 || u_info->signature > 11)
		printf("<option value=\"0\" selected>不用签名档</option>\n");
	else
		printf("<option value=\"0\">不用签名档</option>\n");
	for (i = 1; i <= numofsig && i <= 11; i++) {
		if (u_info->signature == i)
			printf
			    ("<option value=\"%d\" selected>第 %d 个</option>\n",
			     i, i);
		else
			printf("<option value=\"%d\">第 %d 个</option>\n", i,
			       i);
	}
	if (u_info->signature == -1)
		printf
		    ("<option value=\"-%d\" selected>随机签名档</option>\n",
		     numofsig);
	else
		printf("<option value=\"-%d\">随机签名档</option>\n", numofsig);
	printf("</select>\n");
}

void
printuploadattach()
{
	printf(" [<a href=bbsupload target=uploadytht>添加/删除附件</a>]");
}

void
printusemath(int checked)
{
	printf("<input type=checkbox name=usemath %s> "
	       "<a href=home/boards/BBSHelp/html/itex/itexintro.html target=_blank>"
	       "使用Tex数学公式</a>", checked ? " checked" : "");
}
