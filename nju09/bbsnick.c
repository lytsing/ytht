#include "bbslib.h"

int
bbsnick_main()
{
	int i;
	unsigned char nick[80];
	html_header(1);
	check_msg();
	printf("<body>");
	if (!loginok || isguest)
		http_fatal("�Ҵҹ����޷��ı��ǳ�");
	changemode(GMENU);
	strsncpy(nick, getparm("nick"), 30);
	if (nick[0] == 0) {
		printf("%s -- ��ʱ�ı��ǳ�(�����ķ���Ч) [ʹ����: %s]<hr>\n",
		       BBSNAME, currentuser->userid);
		printf
		    ("<form action=bbsnick>���ǳ�<input name=nick size=24 maxlength=24 type=text value='%s'> \n",
		     void1(nohtml(u_info->username)));
		printf("<input type=submit value=ȷ��>");
		printf("</form></body>");
		http_quit();
	}
	for (i = 0; nick[i]; i++)
		if (nick[i] < 32 || nick[i] == 255)
			nick[i] = ' ';
	strsncpy(u_info->username, nick, 32);
	printf("��ʱ����ǳƳɹ�");
	printf("</body></html>");
	return 0;
}
