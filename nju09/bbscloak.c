#include "bbslib.h"

int
bbscloak_main()
{
	html_header(1);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܽ��д˲���, ���ȵ�¼");
	if (!(currentuser->userlevel & PERM_CLOAK))
		http_fatal("����Ĳ���");
	changemode(GMENU);
	if (u_info->invisible) {
		u_info->invisible = 0;
		printf("����״̬�Ѿ�ֹͣ��.");
	} else {
		u_info->invisible = 1;
		printf("����״̬�Ѿ���ʼ��.");
	}
	return 0;
}
