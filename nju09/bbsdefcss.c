#include "bbslib.h"


int
bbsdefcss_main()
{
	char *ptr, buf[256];
	int type;
	html_header(1);
	check_msg();
	printf("<body>");
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܶ��ƽ���");
	changemode(GMENU);
	type = atoi(getparm("type"));
	if (type > 0){
		sethomefile(buf, currentuser->userid, "ubbs.css");
		ptr=getparm("ucss");
		f_write(buf,ptr);
		sethomefile(buf, currentuser->userid, "uleft.css");
		ptr=getparm("uleftcss");
		f_write(buf,ptr);
		//�����ļ���css,�Ȱ��Ǳ�Ū�ú��ٴ򿪡�
#if 0
		sethomefile(buf, currentuser->userid, "upercor.css");
		ptr=getparm("upercorcss");
		f_write(buf,ptr);
#endif
		printf("WWW�û�������ʽ�趨�ɹ�.<br>\n");
		printf("[<a href='javascript:history.go(-2)'>����</a>]");
		return 0;
	}
	printf("<table align=center><form action=bbsdefcss method=post>\n");
	printf("<tr><td>\n");
	printf("<input type=hidden name=type value=1>");
	printf("<font color=red> ����CSSģʽ�����û��Զ���WWW�ĸ�����ʾЧ���� ע�⣬���������ڵ�ǰ�Ľ���[%s]�������css��ȷ���󽫸���ԭ�е������Զ���css��</font><br><br>\n",currstyle->name);
	printf
	     ("��ǰ�������ڵ�CSS�������£�<br><textarea name=ucss rows=10 cols=76>");
	if(strstr(currstyle->cssfile,"ubbs.css"))
		sethomefile(buf, currentuser->userid, "ubbs.css");
	else
		sprintf(buf,HTMPATH "%s",currstyle->cssfile);
	showfile(buf);
	printf
	     ("</textarea><br><br>��ǰ�����ѡ����CSS��������(body.foot�ǵ��е�css)��<br><textarea name=uleftcss rows=5 cols=76>");
	if(strstr(currstyle->leftcssfile,"uleft.css"))
		sethomefile(buf, currentuser->userid, "uleft.css");
	else
		sprintf(buf,HTMPATH "%s",currstyle->leftcssfile);
	showfile(buf);
	printf("</textarea><br><br>");
#if 0
	printf("��ǰ�ĸ����ļ���CSS�������£�<br><textarea name=upercorcss rows=5 cols=76>");
	sethomefile(buf, currentuser->userid, "upercor.css");
	if(!file_exist(buf))
		sprintf(buf,HTMPATH "%s",currstyle->cssfile);
	showfile(buf);
#endif
	printf
	    ("</textarea></td></tr><tr><td align=center><input type=submit value=ȷ��> <input type=reset value=��ԭ>\n");
	printf("</td></tr></form></table></body></html>\n");
	return 0;
}

