#include "bbslib.h"

int
bbsbadlogins_main()
{
	char file[STRLEN];
	html_header(1);
	printf("<body>");
	if (!loginok || isguest) {
		printf("Ŷ���㲢û�е�½�����ü�������¼��</body></html>");
		return 0;
	}

	sethomefile(file, currentuser->userid, BADLOGINFILE);
	if (!file_exist(file)) {
		printf("û���κ�������������¼</body></html>");
		return 0;
	}
	if (*getparm("del") == '1') {
		unlink(file);
		printf("������������¼�ѱ�ɾ��<br>");
		printf
		    ("<a href='#' onClick='javascript:window.close()'>�رմ���</a>");
	} else {
		printf("��������������������¼<br><pre>");
		showfile(file);
		printf("</pre>");
		printf("<a href=bbsbadlogins?del=1>ɾ��������������¼</a>");
	}
	printf("<br><br><b>Ϊ��֤�ʻ��İ�ȫ�������� telnet ��ʽ��½" MY_BBS_NAME
	       "���������˵�--&gt;�������䡱--&gt;���ޱ���˵������������趨��ֹ��½�� IP��</b>");
	printf("</body></html>");
	return 0;
}
