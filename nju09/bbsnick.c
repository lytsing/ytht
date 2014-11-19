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
		http_fatal("匆匆过客无法改变昵称");
	changemode(GMENU);
	strsncpy(nick, getparm("nick"), 30);
	if (nick[0] == 0) {
		printf("%s -- 临时改变昵称(环顾四方有效) [使用者: %s]<hr>\n",
		       BBSNAME, currentuser->userid);
		printf
		    ("<form action=bbsnick>新昵称<input name=nick size=24 maxlength=24 type=text value='%s'> \n",
		     void1(nohtml(u_info->username)));
		printf("<input type=submit value=确定>");
		printf("</form></body>");
		http_quit();
	}
	for (i = 0; nick[i]; i++)
		if (nick[i] < 32 || nick[i] == 255)
			nick[i] = ' ';
	strsncpy(u_info->username, nick, 32);
	printf("临时变更昵称成功");
	printf("</body></html>");
	return 0;
}
