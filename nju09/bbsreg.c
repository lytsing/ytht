#include "bbslib.h"

int
bbsreg_main()
{
	html_header(1);
	printf("<script language=\"JavaScript\"><!--\n");
//      printf("function show(userid) {if (document.images) document.images['myImage'].src = \"http://"MY_BBS_DOMAIN"/cgi-bin/regimage?userid=\"+userid;}\n");
	printf
	    ("function show(userid) {document.images['myImage'].src = \"/cgi-bin/regimage?userid=\"+userid;\n");
	printf
	    ("document.regform.g.value=\"���������������\"; document.regform.g.disabled=false}\n");
	printf("//--></script>\n");
	printf("<body>");
	if (!strcmp(MY_BBS_ID, "YTHT"))
		http_fatal("�ݲ�����ע��");
	printf("<nobr><center>%s -- ���û�ע��<hr>\n", BBSNAME);
	printf
	    ("<font color=green>��ӭ���뱾վ. ������������ʵ��д. ��*�ŵ���ĿΪ��������.</font>");
	printf
	    ("</center><br>1�������Ե��͹����쵼�˻��������˵���ʵ�������ֺš�����������ע�᣻ <br>");
	printf("2�������Թ��һ�������������������ע�᣻ <br>");
	printf("3������ע�����������֮���������µ����֣� <br>");
	printf("4������ע�᲻������������֮���ţ� <br>");
	printf("5������ע���ײ������塢�����������֮���ţ� <br><center>");

	printf("<form name=regform method=post action=bbsdoreg>\n");
	printf("<table width=100%%>\n");
	printf("<tr><td align=right>*���������:<td align=left>"
	       "<input name=userid size=%d maxlength=%d "
	       "onBlur=show(document.regform.userid.value);return false> "
	       "(2-12�ַ�, ����ȫΪӢ����ĸ���׼����)\n", IDLEN, IDLEN);
	printf("<tr><td align=right>*����������:<td align=left>"
	       "<input type=password name=pass1 size=%d maxlength=%d> (��Ч����31λ)\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td align=right>*��ȷ������:<td align=left>"
	       "<input type=password name=pass2 size=%d maxlength=%d> (��Ч����31λ)\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td align=right>*�������ǳ�:<td align=left>"
	       "<input name=username size=20 maxlength=%d> (2-30�ַ�, ��Ӣ�Ĳ���)\n",
	       NAMELEN - 1);
	printf("<tr><td align=right>*������������ʵ����:</td><td align=left>"
	       "<input name=realname size=20> (��������, ����2������)</td></tr>\n");
	printf("<tr><td align=right>*Ŀǰ��ϸסַ:</td><td align=left>"
	       "<input name=address size=40>  (����8������)</td></tr>\n");
	printf("<tr><td></td><td align=left><font color=red>"
	       "(סַ����嵽����������ƺ���)</font></td></tr>");
	printf("<tr><td align=right>*ѧУϵ�����߹�˾��λ:</td><td align=left>"
	       "<input name=dept size=40> (����7������)</td></tr>\n");
	printf("<tr><td></td><td align=left><font color=red>"
	       "(ѧУ����嵽Ժϵ�꼶��������λ����嵽����)</font></td></tr>");
	printf("<tr><td align=right>email��ַ(��ѡ):</td><td align=left>"
	       "<input name=email size=40></td></tr>\n");
	printf("<tr><td align=right>����绰(��ѡ):</td><td align=left>"
	       "<input name=phone size=40></td></tr>\n");
	printf
	    ("<tr><td align=right>У�ѻ���߱�ҵѧУ(��ѡ):</td><td align=left>"
	     "<input name=assoc size=40></td></tr>\n");
	printf("<tr><td align=right>��վ����(��ѡ):</td><td align=left>");
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
	printf("</form></center>");
	http_quit();
	return 0;
}
