#include "bbslib.h"
FILE *fp;

int
bbssig_main()
{
	struct mmapfile mf = { ptr:NULL };
	char path[256];
	html_header(1);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ�������ǩ���������ȵ�¼");
	changemode(EDITUFILE);
	printf("<body><center>%s -- ����ǩ���� [ʹ����: %s]<hr>\n",
	       BBSNAME, currentuser->userid);
	sprintf(path, "home/%c/%s/signatures",
		mytoupper(currentuser->userid[0]), currentuser->userid);
	if (!strcasecmp(getparm("type"), "1"))
		save_sig(path);
	printf("<form name=form1 method=post action=bbssig?type=1>\n");
	printf("ǩ����ÿ6��Ϊһ����λ, ��1�е���6��Ϊ��һ��, ��7�е���12�п�ʼΪ��2��, ��������.�س�Ϊ���б��<br>"
	       "(<a href=bbscon?B=Announce&F=M.1047666649.A>"
	       "[��ʱ����]����ͼƬǩ�����Ĵ�С����</a>)<br>");
	printubb("form1", "text");
	printf
	    ("<textarea  onkeydown='if(event.keyCode==87 && event.ctrlKey) {document.form1.submit(); return false;}'  onkeypress='if(event.keyCode==10) return document.form1.submit()' name=text rows=20 cols=80>\n");
	MMAP_TRY {
		if (mmapfile(path, &mf) >= 0) 
			ansi2ubb(mf.ptr, mf.size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	printf("</textarea><br>\n");
	printf("<input type=submit value=����> ");
	printf("<input type=reset value=��ԭ>\n");
	printf("</form><hr></body>\n");
	http_quit();
	return 0;
}

void
save_sig(char *path)
{
	char *buf;
	int use_ubb = strlen(getparm("useubb"));
	buf = getparm("text");
	if (use_ubb)
		ubb2ansi(buf, path);
	else
		f_write(path, buf);
	printf("ǩ�����޸ĳɹ���");
	http_quit();
}
