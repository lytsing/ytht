#include "bbslib.h"

int
bbsemailreg_main()
{
#ifndef ENABLE_EMAILREG
	//if ENABLE_EMAILREG is not defined, bbsemailreg will just be an
	//alias of bbsreg. See bbsmain.c. The following line is put here
	//just for reference. It will never be excuted.
	return bbsreg_main();
#else
	html_header(1);
	printf("<script language=\"JavaScript\"><!--\n"
	       "function show(userid) {"
	       "document.images['myImage'].src = \"/cgi-bin/regimage?userid=\"+userid;\n"
	       "document.regform.g.value=\"���������������\";"
	       "document.regform.g.disabled=false;}\n" "//--></script>\n");
	printf("<body>");
	printf("<center>%s -- ���û�ע��<hr>\n", BBSNAME);
	printf("��ӭ���뱾վ��������������ʵ��д���� * �ŵ���ĿΪ�������ݡ�<br>"
	       "�ύע�ᵥ֮��ϵͳ��������� email ��ַ����һ��ȷ���š��ظ�<br>"
	       "��ȷ����֮�����Ϳ����ڱ�վ�������¡����������ˡ�<br>");
	printf
	    ("<i>Ϊ�������������벻Ҫע�᲻������������֮���š�����������ʺţ�<br>"
	     "�����ر���Ҫ��Ҳ�����ʹ��֪����ʿ��������<br><br></i>");
	printf("<b><font color=red>����޷��ṩ email����"
	       "<a href=bbsreg>��дע�ᵥ</a>���ֹ�ע��</font></b><br><br>");
	printf("<form name=regform method=post action=bbsdoreg?useemail=on>\n");
	printf("<table>\n");
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
	printf("<tr><td align=right>*��������������:</td><td align=left>"
	       "<input name=realname size=20> (��������, ����2������)</td></tr>\n");
	printf("<tr><td align=right>*Email��ַ:</td><td align=left>"
	       "<input name=email size=40> (ÿ�� email �����֤һ�� ID)</td></tr>\n");
	printf("<tr><td align=right>*��ȷ��Email��ַ:</td><td align=left>"
	       "<input name=email2 size=40> (Ҫ�� email ��ַ��֤ѽ���������)</td></tr>\n");
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
#endif
}
