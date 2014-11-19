#include "bbslib.h"

int
bbsmailcon_main()
{
	FILE *fp = NULL;
	char dir[80], file[80], path[80], *id;
	struct fileheader x;
	int num, total = 0;
	if ((!loginok || isguest) && (!tempuser))
		http_fatal("���ȵ�¼%d", tempuser);
	//if (tempuser) http_fatal("user %d", tempuser);
	changemode(RMAIL);
	strsncpy(file, getparm("file"), 32);
	num = atoi(getparm("num")) - 1;
	id = currentuser->userid;
	if (strncmp(file, "M.", 2))
		http_fatal("����Ĳ���1");
	if (strstr(file, "..") || strstr(file, "/"))
		http_fatal("����Ĳ���2");
	sprintf(path, "mail/%c/%s/%s", mytoupper(id[0]), id, file);
	if (*getparm("attachname") == '/') {
		showbinaryattach(path);
		return 0;
	}
	if (!tempuser && num != -1) {
		sprintf(dir, "mail/%c/%s/.DIR", mytoupper(id[0]), id);
		total = file_size(dir) / sizeof (x);
		if (total <= 0)
			http_fatal("����Ĳ���3 %s", dir);
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
	printf("%s -- �Ķ��ż� [ʹ����: %s]<hr>\n", BBSNAME, id);
	if (!tempuser) {
		printf("</center>����: %s<br>", void1(titlestr(x.title)));
		printf("������: %s<br>", titlestr(x.owner));
	}
	if (loginok && !isguest && !(currentuser->userlevel & PERM_LOGINOK) &&
	    !tempuser && !strncmp(x.title, "<ע��ʧ��>", 10)
	    && !has_fill_form(currentuser->userid))
		printf
		    ("--&gt;<a href=bbsform><font color=RED size=+1>������дע�ᵥ</font></a>&lt;--\n<br>  <input type=button value=�鿴���� onclick=\"javascript:{open('/reghelp.html','winreghelp','width=600,height=460,resizeable=yes,scrollbars=yes');}\"><br>");
	printf("<center>");
	sprintf(path, "mail/%c/%s/%s", mytoupper(id[0]), id, file);
	showcon(path);
	if (!tempuser && num != -1) {
		printf
		    ("[<a onclick='return confirm(\"�����Ҫɾ���������?\")' href=bbsdelmail?file=%s>ɾ��</a>]",
		     file);
		printf
		    ("[<a onclick='return confirm(\"�������Ϊ������������ʼ���? ������ǣ���ʹ��ɾ���ż�����\")' href=bbsdelmail?spam=1&file=%s>�����ʼ�</a>]",
		     file);
		printf("[<a href=bbsfwdmail?F=%s>ת��</a>]", fh2fname(&x));
		printf("[<a href=bbscccmail?F=%s>ת��</a>]", fh2fname(&x));
		if (num > 0) {
			fseek(fp, sizeof (x) * (num - 1), SEEK_SET);
			fread(&x, sizeof (x), 1, fp);
			printf("[<a href=bbsmailcon?file=%s&num=%d>��һƪ</a>]",
			       fh2fname(&x), num);
		}
		printf("[<a href=bbsmail>�����ż��б�</a>]");
		if (num < total - 1) {
			fseek(fp, sizeof (x) * (num + 1), SEEK_SET);
			fread(&x, sizeof (x), 1, fp);
			printf("[<a href=bbsmailcon?file=%s&num=%d>��һƪ</a>]",
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
		printf("[<a href='bbspstmail?file=%s&num=%d&reply=1'>����</a>]",
		       fh2fname(&x), num + 1);
		fclose(fp);
	}
	printf("</center></body>\n");
	http_quit();
	return 0;
}
