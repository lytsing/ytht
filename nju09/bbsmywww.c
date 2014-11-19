#include "bbslib.h"

int
bbsmywww_main()
{
	char *ptr, buf[256];
	int t_lines = 20, link_mode = 0, def_mode = 0, att_mode = 0, doc_mode =
	    0, edit_mode = 0, type;
	html_header(1);
	check_msg();
	printf("<body>");
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܶ��ƽ���");
	changemode(GMENU);
	if (readuservalue(currentuser->userid, "t_lines", buf, sizeof (buf)) >=
	    0)
		t_lines = atoi(buf);
	if (readuservalue(currentuser->userid, "link_mode", buf, sizeof (buf))
	    >= 0)
		link_mode = atoi(buf);
	if (readuservalue(currentuser->userid, "def_mode", buf, sizeof (buf)) >=
	    0)
		def_mode = atoi(buf);
//      if (readuservalue(currentuser->userid, "att_mode", buf, sizeof(buf)) >= 0) att_mode=atoi(buf);
	att_mode = w_info->att_mode;
	doc_mode = w_info->doc_mode;
	edit_mode = w_info->edit_mode;
	type = atoi(getparm("type"));
	ptr = getparm("t_lines");
	if (ptr[0])
		t_lines = atoi(ptr);
	ptr = getparm("link_mode");
	if (ptr[0])
		link_mode = atoi(ptr);
	ptr = getparm("def_mode");
	if (ptr[0])
		def_mode = atoi(ptr);
	ptr = getparm("att_mode");
	if (ptr[0])
		att_mode = atoi(ptr);
	ptr = getparm("doc_mode");
	if (ptr[0])
		doc_mode = atoi(ptr);
	ptr = getparm("edit_mode");
	if (ptr[0])
		edit_mode = atoi(ptr);
	printf("<center>%s -- WWW���˶��� [ʹ����: %s]<hr>", BBSNAME,
	       currentuser->userid);
//      if (type > 0)
//              return save_set(t_lines, link_mode, def_mode, att_mode);
	if (t_lines < 10 || t_lines > 40)
		t_lines = 20;
	if (link_mode < 0 || link_mode > 1)
		link_mode = 0;
	if (att_mode < 0 || att_mode > 1)
		att_mode = 0;
	if (doc_mode < 0 || doc_mode > 1)
		doc_mode = 0;
	if (edit_mode < 0 || edit_mode > 1)
		edit_mode = 0;

	if (type > 0)
		return save_set(t_lines, link_mode, def_mode, att_mode,
				doc_mode, edit_mode);
	printf("<table><form action=bbsmywww>\n");
	printf("<tr><td>\n");
	printf("<input type=hidden name=type value=1>");
	printf
	    ("һ����ʾ����������(10-40): <input name=t_lines size=8 value=%d><br>\n",
	     t_lines);
	printf
	    ("����ʶ�� (0ʶ��, 1��ʶ��): <input name=link_mode size=8 value=%d><br>\n",
	     link_mode);
	printf
	    ("����ģʽ (0һ��, 1����): &nbsp;&nbsp;<input name=def_mode size=8 value=%d><br>\n",
	     def_mode);
	printf
	    ("�༭ģʽ (0 �򵥣�1 ����������): <input name=edit_mode size=8 value=%d><br>\n",
	     edit_mode);
	printf
	    ("����ģʽ (0��ͨ��1����): &nbsp;&nbsp;<input name=att_mode size=8 value=%d><br><br>\n",
	     att_mode);
	printf
	    ("<font color=red>����ģʽ����Ϊ 0 �����ٶȱȽϿ졣ͼƬ��ʾ�������߿��Խ�����ģʽ����Ϊ 1��</font><br>\n");
	printf
	    ("����ģʽ (0��ͨ��1����): &nbsp;&nbsp;<input name=doc_mode size=8 value=%d><br><br>\n",
	     doc_mode);
	printf
	    ("<font color=red>����ģʽ����Ϊ 0 �����ٶȱȽϿ졣������ʾ�������߿��Խ�����ģʽ����Ϊ 1��</font><br>\n");
	printf
	    ("</td></tr><tr><td align=center><input type=submit value=ȷ��> <input type=reset value=��ԭ>\n");
	printf("</td></tr></form></table></body></html>\n");
	return 0;
}

int
save_set(int t_lines, int link_mode, int def_mode, int att_mode, int doc_mode,
	 int edit_mode)
{
	char buf[20];
	if (t_lines < 10 || t_lines > 40)
		http_fatal("���������");
	if (link_mode < 0 || link_mode > 1)
		http_fatal("���������ʶ�����");
	if (def_mode < 0 || def_mode > 1)
		http_fatal("�����ȱʡģʽ");
	if (att_mode < 0 || att_mode > 1)
		http_fatal("����ĸ���ģʽ");
	if (doc_mode < 0 || doc_mode > 1)
		http_fatal("���������ģʽ");

	sprintf(buf, "%d", t_lines);
	saveuservalue(currentuser->userid, "t_lines", buf);
	sprintf(buf, "%d", link_mode);
	saveuservalue(currentuser->userid, "link_mode", buf);
	sprintf(buf, "%d", def_mode);
	saveuservalue(currentuser->userid, "def_mode", buf);
	sprintf(buf, "%d", att_mode);
	saveuservalue(currentuser->userid, "att_mode", buf);
	sprintf(buf, "%d", doc_mode);
	saveuservalue(currentuser->userid, "doc_mode", buf);
	sprintf(buf, "%d", edit_mode);
	saveuservalue(currentuser->userid, "edit_mode", buf);

	w_info->t_lines = t_lines;
	w_info->def_mode = def_mode;
	w_info->link_mode = link_mode;
	w_info->att_mode = att_mode;
	w_info->doc_mode = doc_mode;
	w_info->edit_mode = edit_mode;
	printf("WWW���Ʋ����趨�ɹ�.<br>\n");
	printf("[<a href='javascript:history.go(-2)'>����</a>]");
	return 0;
}
