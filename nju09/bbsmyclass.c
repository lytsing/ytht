#include "bbslib.h"

void
showmyclasssetting()
{
	struct brcinfo *brcinfo;
	char *myclass, *myclasstitle;
	char *(titlelist[]) = {
	"�������", "�ҵİ༶", "�ҵ�Ժϵ", "�ҵ�ѧУ", NULL};
	int i;
	printf
	    ("<script>function search(){\n"
	     "newwindow=open('bbssearchboard?element=sel.myclass.value&match='+document.sel.myclass.value,\n"
	     "'', 'width=600,height=460,resizable=yes,scrollbars=yes');\n"
	     "if(newwindow.opener==null) newwindow.opener=self;"
	     "}\n"
	     "function settitle(v) {document.sel.myclasstitle.value=v;}</script>");
	printf("<center>ѡ���� WWW ������ʾ�İ���<br><br>");
	myclass = getparm("myclass");
	myclasstitle = getparm("myclasstitle");
	if (!myclass[0]) {
		brcinfo = brc_readinfo(currentuser->userid);
		myclass = brcinfo->myclass;
		myclasstitle = brcinfo->myclasstitle;
		if (!myclasstitle[0])
			myclasstitle = "�������";
	}
	printf
	    ("<table><tr><td><form name='sel' action=bbsmyclass method=post>"
	     "�������ƣ�<input type=text name=myclass value='%s'> "
	     "<a href='javascript:search()'>����</a><br>",
	     void1(nohtml(myclass)));
	printf
	    ("��ʾΪ��<input type=text name=myclasstitle value='%s'>&nbsp;&nbsp;",
	     void1(nohtml(myclasstitle)));
	for (i = 0; titlelist[i]; i++) {
		printf("<a href='javascript:settitle(\"%s\")'>%s</a> ",
		       titlelist[i], titlelist[i]);
	}
	printf("<input type=hidden name=submittype value=1>");
	printf
	    ("<br><center><input type=submit value='ȷ��'></form></td></tr></table>");
#if 1
	printf
	    ("<br>û�ҵ��Լ���ѧУ���뵽<a href=home?B=8admin>�ֵ�ԺУ���������</a>���뿪�棡"
	     "<br><br>û�ҵ��Լ��İ༶�����ϵ�<a href=home?B=L_admin>ͬѧ¼����������</a>"
	     "�� (������ѧ��ϵ��) <a href=home?B=1admin>������ѧ����������</a>���뿪�棡");
#endif
}

void
savemyclass()
{
	struct brcinfo *brcinfo;
	struct boardmem *bx;
	char *ptr = getparm("myclass");
	bx = getboard2(ptr);
	if (!bx) {
		printf("<b>û���ҵ� %s �棬���á�������������������</b><br>\n",
		       ptr);
		return;
	}
	if (!has_read_perm_x(currentuser, bx)) {
		printf
		    ("<b>%s ����һ����հ��棬���������������룬�����á�������������ѡ��������</b><br>\n",
		     ptr);
		return;
	}
	brcinfo = brc_readinfo(currentuser->userid);
	strsncpy(brcinfo->myclass, bx->header.filename,
		 sizeof (brcinfo->myclass));
	ptr = strtrim(getparm("myclasstitle"));
	if (!strcmp(ptr, "�������"))
		brcinfo->myclasstitle[0] = 0;
	else
		strsncpy(brcinfo->myclasstitle, ptr,
			 sizeof (brcinfo->myclasstitle));
	brc_saveinfo(currentuser->userid, brcinfo);
	printf
	    ("<b>���óɹ�</b>���Ժ���Ե��˵������˹����䡱-->���趨������ʾ�İ��桱�����޸ģ�<br>");
	printf("<script>top.f4.location.reload();</script>");
}

int
bbsmyclass_main()
{
	int type;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("��δ��¼���߳�ʱ");
	changemode(ZAP);
	type = atoi(getparm("submittype"));
	if (type == 1) {
		savemyclass();
		printf("<hr>");
	}
	showmyclasssetting();
	return 0;
}
