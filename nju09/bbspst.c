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
		printf("�Ҵҹ��Ͳ��ܷ������£����ȵ�¼!<br>");
		printf
		    ("�����ȷʵ�Ѿ���¼ȴ���������ʾ���볢���˽��¼ʱ�� IP��֤��Χ ѡ��<br>");
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
		http_fatal("������ļ���");
	filetime = atoi(file + 2);
	if (!(x = getboard(board)))
		http_fatal("�������������������Ȩ�ڴ���������������");
	if (njuinn_board(board) && !innd_board(board))
		local_article = 1;
	if (!has_post_perm(currentuser, x) && !isguest)
		http_fatal("�������������������Ȩ�ڴ���������������");
	if (noadm4political(board))
		http_fatal("�Բ���,��Ϊû�а��������Ա����,������ʱ���.");
	if (x->ban == 2)
		http_fatal("�Բ���, ��Ϊ�������³���, ������ʱ�ر�.");

	if (file[0]) {
		sprintf(path, "boards/%s/.DIR", board);
		ret =
		    get_fileheader_records(path, filetime, num, 1, &dirinfo,
					   &ent, 1);
		if (ret == -1)
			http_fatal("������ļ���");
		num = ent;
		if (dirinfo.accessed & FH_NOREPLY)
			http_fatal("���ı���Ϊ����Reģʽ");
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
		printf("�Ҵҹ��Ͳ��ܷ������£����ȵ�¼!<br><br>");
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
	printf("<table height=100%% width=100%% border=0><tr height=20><td>");	//���
	printf("%s -- ���������� %s ������\n", BBSNAME, board);
	strsncpy(buf, getsenv("QUERY_STRING"), sizeof (buf));
	if ((ptr = strstr(buf, "&M=")))
		*ptr = 0;
	if (w_info->edit_mode == 0) {
		printf
		    ("<script>if(isRichText) document.write(' [<a href=bbspst?%s&M=1 class=red>�л������������ñ༭��</a>');</script>]",
		     buf);
	} else {
		printf(" [<a href=bbspst?%s&M=0>�л����򵥱༭��</a>]", buf);
		printf
		    ("<script>if(isRichText!=true||!testReplace()) location.replace('bbspst?%s&M=0')</script>",
		     buf);
	}
	if (x->header.flag & IS1984_FLAG)
		printf
		    ("<br><font color=red>��ע�⣬���ķ������ͨ�����</font>");
	printf("</td></tr><tr><td valign=top>");	//���
	printf("<table border=1 width=100%% height=100%%>");	//�б�
	if (file[0])
		snprintf(buf, sizeof (buf), "&ref=%s&rid=%d", file, num + 1);
	//form ���� table �� tr ֮�䣬������ʾ����
	printf("<form name=form1 method=post action=bbssnd?B=%d&th=%d%s>\n",
	       getbnumx(x), thread, file[0] ? buf : "");
	printf("<tr height=40><td>");	//�б�
	printf
	    ("ʹ�ñ���: <input type=text name=title size=50 maxlength=100 value='%s'>",
	     void1(nohtml(title)));
	if (innd_board(board) || njuinn_board(board))
		printf
		    (" <font class=red>ת��</font><input type=checkbox name=outgoing %s>\n",
		     local_article ? "" : "checked");
	if (anony_board(board))
		printf("����<input type=checkbox name=anony %s>\n",
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
		printf("����ģʽ: %s ", fullquote ? "��ȫ" : "����");
		printf
		    ("[<a target=_self href=bbspst?inframe=1&B=%d&file=%s&num=%d&la=%d",
		     getbnumx(x), file, num + 1, local_article);
		printf("&fullquote=%d>ʹ��%s����(����������������)</a>]<br>",
		       !fullquote, (!fullquote) ? "��ȫ" : "����");
	}
	printf("</td></tr><tr><td>");	//�б�
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
	printf("</td></tr><tr height=30><td class=post align=center>");	//�б�
	if (w_info->edit_mode)
		printf
		    ("<input type=submit value=���� onclick=\"this.value='�����ύ�У����Ժ�...';"
		     "this.disabled=true;submitForm();form1.submit();\">");
	else
		printf
		    ("<table width=100%%><tr><td align=center>"
		     "<input type=submit value=���� onclick=\"this.value='�����ύ�У����Ժ�...';"
		     "this.disabled=true;form1.submit();\"> </td><td align=center>"
		     "<input type=reset value=��� onclick='return confirm(\"ȷ��Ҫȫ�������?\")'>"
		     "</td></tr></table>");
	printf("</td></tr></form></table>"	//�б�
	       "</td></tr></table></div>");	//���
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
	printf("\n\n�� �� %s �Ĵ������ᵽ: ��\n", userid);
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
			if (ml.len > 4 && (!strncmp(ml.text, ": ��", 4)
					   || !strncmp(ml.text, ": : ", 4)))
				continue;
			if (ml.len == 3 && !strncmp(ml.text, "--\n", 3))
				break;
			if (ml.len > 10 && !strncmp(ml.text, "begin 644 ", 10))
				break;
			if (ml.len > 18
			    && !strncmp(ml.text, "beginbinaryattach ", 18))
				break;
			//��ml.len������ml.len+1���������һ��\n�������е��������Ҫ�����ģ�û����
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
				printf(": (��������ʡ��...)\n");
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
	    ("<a target=_blank href=bbssig>ǩ����</a> <select name=\"signature\">\n");
	if (u_info->signature == 0 || u_info->signature > 11)
		printf("<option value=\"0\" selected>����ǩ����</option>\n");
	else
		printf("<option value=\"0\">����ǩ����</option>\n");
	for (i = 1; i <= numofsig && i <= 11; i++) {
		if (u_info->signature == i)
			printf
			    ("<option value=\"%d\" selected>�� %d ��</option>\n",
			     i, i);
		else
			printf("<option value=\"%d\">�� %d ��</option>\n", i,
			       i);
	}
	if (u_info->signature == -1)
		printf
		    ("<option value=\"-%d\" selected>���ǩ����</option>\n",
		     numofsig);
	else
		printf("<option value=\"-%d\">���ǩ����</option>\n", numofsig);
	printf("</select>\n");
}

void
printuploadattach()
{
	printf(" [<a href=bbsupload target=uploadytht>���/ɾ������</a>]");
}

void
printusemath(int checked)
{
	printf("<input type=checkbox name=usemath %s> "
	       "<a href=home/boards/BBSHelp/html/itex/itexintro.html target=_blank>"
	       "ʹ��Tex��ѧ��ʽ</a>", checked ? " checked" : "");
}
