#include "bbslib.h"

int
bbsdelmsg_main()
{
	html_header(1);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܴ���ѶϢ, ���ȵ�¼");
	changemode(LOOKMSGS);
	clear_msg(currentuser->userid);
	u_info->unreadmsg = 0;
	printf("��ɾ������ѶϢ����");
	return 0;
}
