#include "bbslib.h"

int
regreq_main()
{
	html_header(1);
	printf("<nobr><center>%s -- ע����ʾ<hr>\n", BBSNAME);
	printf("<table width=100%%>\n");
	printf("�װ���" MY_BBS_NAME "�û������Ѿ��ڱ�վ�����%ld������<br>",
	       (now_t - (w_info->login_start_time)) / 60);
	printf("���<a href=bbsreg>����</a>����ע��<br><br>");
	printf("�� һ��Ҫע����<br>");
	printf("Ϊ�˸���λ�����ṩ�������ʵķ��񣬸��õ����ҽ�����ͨ��");
	printf(MY_BBS_NAME "���齨��������û�ע�ᡣ");
	printf
	    ("ע���Ϊ���ǵ���ͨע���û���ʮ�ַ����ݵġ����������ڻ���һ����ʱ�䣬");
	printf("��Ϊ" MY_BBS_NAME
	       "��Ϊע��ɹ�����ͨע���û��ṩ�������ɫ����");
	printf("��Ȼ����ʹû�н���ע��Ҳ���������" MY_BBS_NAME "����Ϣ��");
	printf("������һЩ��ɫ�����ֻ��������̾�ˡ�");
	printf
	    ("Ϊ�����Ժ���õ�����" MY_BBS_NAME
	     "���������ܵ����񣬸Ͽ�ȥע��ɣ�<br><br>");

	printf("�� �ȵ�¼�������ʲô�ô���<br>");
	printf("���ǽ���ϰ����ʹ��www��ʽ����" MY_BBS_NAME
	       "ʱ�Ƚ����û���¼��");
	printf("��Ϊ" MY_BBS_NAME
	       "����������ۺ��������ܶ���������û���¼����ʹ�á�");
	printf
	    ("��Ϊ��ͨ�û�������һ��������¼�ĺ�ϰ�ߣ������Եؽ�ʡ������������");
	printf("���ۺͲ�ѯ��ʱ�䣬�����������ױȵĸ��Ի�����");
	printf("</table><br><hr>\n");
	printf("</center>");
	http_quit();
	return 0;
}
