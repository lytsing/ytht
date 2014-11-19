#include "bbslib.h"

int
bbspstmail_main()
{
	int num, fullquote = 0, reply = 0, lines = 0, rowlen, offset;
	char mymaildir[80], userid[80], buf[512], path[512], file[512],
	    board[40], title[80] = "";
	char *ptr;
	struct fileheader dirinfo;
	struct mmapfile mf={ptr:NULL};
	int ent, ret, filetime;
	html_header(1);
	check_msg();
	if (!loginok)
		http_fatal("�Ҵҹ��Ͳ���д�ţ����ȵ�¼");
	sprintf(mymaildir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]),
		currentuser->userid);
	if ((ptr = check_mailperm(currentuser)))
		http_fatal(ptr);
	changemode(SMAIL);
	getparmboard(board, sizeof (board));
	if (board[0] && !getboard(board))
		http_fatal("�����������");
	strsncpy(file, getparm("F"), 20);
	if (!file[0])
		strsncpy(file, getparm("file"), 20);
	if (file[0] != 'M' && file[0])
		http_fatal("������ļ���");
	filetime = atoi(file + 2);
	num = atoi(getparm("num")) - 1;
	if (file[0]) {
		fullquote = atoi(getparm("fullquote"));
		reply = atoi(getparm("reply"));
		buf[0] = '\0';
		if (board[0])
			sprintf(buf, "boards/%s/.DIR", board);
		else if (loginok && !isguest)
			sprintf(buf, "mail/%c/%s/.DIR",
				mytoupper(currentuser->userid[0]),
				currentuser->userid);
		if ('\0' == buf[0])
			http_fatal("������ļ���");
		ret = get_fileheader_records(buf, filetime, num, 1, &dirinfo, &ent, 1);
		if (ret == -1)
			http_fatal("������ļ���");
		num = ent;
		strsncpy(userid, dirinfo.owner, sizeof (userid));
		if (strchr(userid, '.')) {
			if (board[0])
				sprintf(buf, "boards/%s/%s", board,
					fh2fname(&dirinfo));
			else
				setmailfile(buf, currentuser->userid,
					    fh2fname(&dirinfo));
			getdocauthor(buf, userid, sizeof (userid));
		}
		if (strncmp(dirinfo.title, "Re: ", 4))
			snprintf(title, 55, "Re: %s", dirinfo.title);
		else
			strsncpy(title, dirinfo.title, 55);
	} else
		strsncpy(userid, getparm("userid"), 20);
	if (isguest && strcmp(userid, "SYSOP"))
		http_fatal("�Ҵҹ��Ͳ���д�ţ����ȵ�¼");
	if (!USERPERM(currentuser, PERM_LOGINOK) && strcmp(userid, "SYSOP"))
		http_fatal("δͨ��ע���û�ֻ�ܸ�SYSOPд��!");
	printf("<script language=javascript>function chguserid(){"
	       "if(document.form1.allfriend.checked==true){"
	       "document.form1.userid.value='���к���';"
	       "document.form1.userid.disabled=true;"
	       "} else {document.form1.userid.value='';"
	       "document.form1.userid.disabled=false;"
	       "document.form1.userid.focus();}}</script>");
	printf("<body><center>\n");
	printf("%s -- �����Ÿ� [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	printf("<table border=1><tr><td>\n");
	printf("<form name=form1 method=post action=bbssndmail>\n");
	printf("<input type=hidden name=reply value=%d>", reply);
	printf("<input type=hidden name=num value=%d>", num + 1);
	printf
	    ("�ż�����: <input type=text name=title size=40 maxlength=100 value='%s'><br>",
	     void1(nohtml(title)));
	printf("������: %s<br>\n", currentuser->userid);
	printf
	    ("������: <input type=text name=userid value='%s'> ���͸����к���<input type=checkbox name=allfriend onclick=chguserid();>[<a href=bbsfall target=_blank>�鿴��������</a>]<br>",
	     nohtml(userid));
	printselsignature();
	printuploadattach();
	printf(" ����<input type=checkbox name=backup>&nbsp;\n");
	printubb("form1", "text");
	if (file[0]) {
		printf("<br>����ģʽ: %s ", fullquote ? "��ȫ" : "����");
		printf
		    ("[<a target=_self href=bbspstmail?inframe=1&B=%d&file=%s&num=%d",
		     getbnum(board), file, num + 1);
		printf("&fullquote=%d>�л�Ϊ%sģʽ</a> (����������������)]",
		       !fullquote, (!fullquote) ? "��ȫ" : "����");
	}
	printf("<br>\n");
	printf
	    ("<textarea  onkeydown='if(event.keyCode==87 && event.ctrlKey) {document.form1.submit(); return false;}'  onkeypress='if(event.keyCode==10) return document.form1.submit()' name=text rows=20 cols=76 wrap=virtual>\n\n");
	if (file[0]) {
		underline = 0;
		highlight = 0;
		lastcolor = 37;
		useubb = 1;
		if (board[0]) {
			printf("�� �� %s �Ĵ������ᵽ: ��\n", userid);
			sprintf(path, "boards/%s/%s", board, file);
		} else {
			printf("�� �� %s ���������ᵽ: ��\n", userid);
			sprintf(path, "mail/%c/%s/%s",
				mytoupper(currentuser->userid[0]),
				currentuser->userid, file);
		}
		MMAP_TRY {
			if (mmapfile(path, &mf) < 0) {
				MMAP_UNTRY;
				printf("</textarea>");
				http_fatal("�ļ�����");
			}
			for (offset = 0, rowlen = 999; rowlen > 1;
			     offset += rowlen)
				rowlen =
				    mmap_getline(mf.ptr + offset,
						 mf.size - offset);
			while (1) {
				if ((rowlen =
				     mmap_getline(mf.ptr + offset,
						  mf.size - offset)) == 0)
					break;
				if ((!strncmp
				     (mf.ptr + offset, ": ��", min(rowlen, 4))
				     && (rowlen >= 4))
				    ||
				    (!strncmp
				     (mf.ptr + offset, ": : ", min(rowlen, 4))
				     && (rowlen >= 4))
				    || (mf.ptr[offset] == '\r')) {
					offset += rowlen;
					continue;
				}
				if ((!strncmp
				     (mf.ptr + offset, "--\n", min(rowlen, 3))
				     && (rowlen >= 3))
				    ||
				    (!strncmp
				     (mf.ptr + offset, "begin 644 ",
				      min(rowlen, 10)) && (rowlen >= 10))
				    ||
				    (!strncmp
				     (mf.ptr + offset, "beginbinaryattach ",
				      min(rowlen, 18)) && (rowlen >= 18)))
					break;
				if (!fullquote && lines >= 7) {
					printf(": (��������ʡ��...)\n");
					break;
				}
				lines++;
				printf(": ");
				ansi2ubb(mf.ptr + offset, rowlen, stdout);
				offset += rowlen;
			}
		}
		MMAP_CATCH {
		}
		MMAP_END mmapfile(NULL, &mf);
	}
	printf("</textarea><br>");
	printf
	    ("<tr><td class=post align=center><table width=100%%><tr><td align=center>"
	     "<input type=submit value=���� onclick=\"this.value='�ż��ύ�У����Ժ�...';"
	     "this.disabled=true;form1.submit();\"> "
	     "</td><td align=center>"
	     "<input type=reset value=��� onclick='return confirm(\"ȷ��Ҫȫ�������?\")'>"
	     "</td></tr></table></form></td></tr></table></body>");
	http_quit();
	return 0;
}
