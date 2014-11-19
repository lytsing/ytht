#include "bbslib.h"

int
bbsinviteconfirm_main()
{
	char inviter[30], code[30], t[30];
	char email[128], name[128], invitefn[256];
	strsncpy(inviter, getparm("inviter"), sizeof (inviter));
	strsncpy(code, getparm("code"), sizeof (code));
	strsncpy(t, getparm("t"), sizeof (t));
	sprintf(invitefn, INVITATIONDIR "/%s/%s.%s", inviter, t, code);

	html_header(1);
	printf("<script language=\"JavaScript\"><!--\n"
	       "function show(userid) {document.images['myImage'].src = \"/cgi-bin/regimage?userid=\"+userid;\n"
	       "document.regform.g.value=\"���������������\"; document.regform.g.disabled=false}\n"
	       "//--></script>\n");

	printf("<body>");
	printf("<center>%s -- ���û�ע��<hr>\n", BBSNAME);

	close(open(invitefn, O_RDWR));	//touch new

	if (!file_exist(invitefn) ||
	    readstrvalue(invitefn, "toemail", email, sizeof (email)) < 0 ||
	    readstrvalue(invitefn, "toname", name, sizeof (name)) < 0) {
		printf
		    ("��������Ѿ������ˣ����������Խ���<a href=bbsemailreg>email �Զ�ע��</a>��");
		return 0;
	}

	if (!trustEmail(email)) {
		printf("��ʹ�õ� email �Ѿ�ע���˱���û������߲�����֤��"
		       "Χ�����������Խ���<a href=bbsemailreg>email �Զ�"
		       "ע��</a>��");
		return 0;
	}
	printf
	    ("<div style='width:500' align=left><b>��ѡ���Լ����û���������</b>"
	     "<br><i>Ϊ�������������벻Ҫע�᲻������������֮���š�����������ʺţ�<br>"
	     "�����ر���Ҫ��Ҳ�����ʹ��֪����ʿ��������<br><br></i></div>");
	printf("<form name=regform method=post action=bbsdoreg?invited=1>\n");
	printf("<input type=hidden name=inviter value='%s'>", inviter);
	printf("<input type=hidden name=code value='%s'>", code);
	printf("<input type=hidden name=t value='%s'>", t);
	printf("<input type=hidden name=email value='%s'>", email);
	printf("<input type=hidden name=name value='%s'>", urlencode(name));
	printf("<table>\n");
	printf("<tr><td align=right>�������û�����</td><td align=left>"
	       "<input type=text name=userid size=%d maxlength=%d "
	       "onBlur='show(document.regform.userid.value);return false'> "
	       "(2-12�ַ�, ����ȫΪӢ����ĸ���׼����)\n", IDLEN, IDLEN);
	printf("<tr><td align=right>���������룺</td><td align=left>"
	       "<input type=password name=pass1 size=%d maxlength=%d> (��Ч����31λ)\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td align=right>��ȷ�����룺</td><td align=left>"
	       "<input type=password name=pass2 size=%d maxlength=%d> (��Ч����31λ)\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td align=right>�������ǳƣ�</td><td align=left>"
	       "<input name=username size=20 maxlength=%d> (2-30�ַ�, ��Ӣ�Ĳ���)\n",
	       NAMELEN - 1);
	printf("<tr><td align=right>����������������</td><td align=left>"
	       "<input name=realname size=30 value='%s'> (��������, ����2������)</td></tr>\n",
	       urlencode(name));
	printf("<tr><td align=right>��վ����(��ѡ)��</td><td align=left>");
	printf
	    ("<textarea name=words rows=3 cols=40 wrap=virutal></textarea></td></tr>");
	printf("<tr><td align=right>"
	       "<input type=\"button\" name=g value=\"������д����\" disabled=true "
	       "onClick=show(document.regform.userid.value);return false><br>");
	printf
	    ("<td align=left><img src=\"/cgi-bin/regimage\" name=\"myImage\">");
	printf("<tr><td align=right>*���������������:<td align=left>"
	       "<input name=regpass size=10 maxlength=10>\n");
	printf("</table><br><hr>\n");
	printf
	    ("<input type=button value=�ύ��� "
	     "onclick=\"this.value='ע�ᵥ�ύ�У����Ժ�...';this.disabled=true;regform.submit();\" >"
	     " <input type=reset value=������д>"
	     " <input type=button value=�鿴���� onclick=\"javascript:{open('/reghelp.html','winreghelp','width=600,height=460,resizeable=yes,scrollbars=yes');return false;}\"\n");
	printf("</form></center></body></html>");

	return 0;
}
