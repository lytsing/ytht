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
	       "document.regform.g.value=\"看不到数字请点我\"; document.regform.g.disabled=false}\n"
	       "//--></script>\n");

	printf("<body>");
	printf("<center>%s -- 新用户注册<hr>\n", BBSNAME);

	close(open(invitefn, O_RDWR));	//touch new

	if (!file_exist(invitefn) ||
	    readstrvalue(invitefn, "toemail", email, sizeof (email)) < 0 ||
	    readstrvalue(invitefn, "toname", name, sizeof (name)) < 0) {
		printf
		    ("这个邀请已经过期了，但是您可以进行<a href=bbsemailreg>email 自动注册</a>。");
		return 0;
	}

	if (!trustEmail(email)) {
		printf("所使用的 email 已经注册了别的用户，或者不在认证范"
		       "围。但是您可以进行<a href=bbsemailreg>email 自动"
		       "注册</a>。");
		return 0;
	}
	printf
	    ("<div style='width:500' align=left><b>请选择自己的用户名和密码</b>"
	     "<br><i>为免予纠纷起见，请不要注册不文明、不健康之代号、引起歧义的帐号；<br>"
	     "如无特别需要，也请避免使用知名人士的姓名。<br><br></i></div>");
	printf("<form name=regform method=post action=bbsdoreg?invited=1>\n");
	printf("<input type=hidden name=inviter value='%s'>", inviter);
	printf("<input type=hidden name=code value='%s'>", code);
	printf("<input type=hidden name=t value='%s'>", t);
	printf("<input type=hidden name=email value='%s'>", email);
	printf("<input type=hidden name=name value='%s'>", urlencode(name));
	printf("<table>\n");
	printf("<tr><td align=right>请输入用户名：</td><td align=left>"
	       "<input type=text name=userid size=%d maxlength=%d "
	       "onBlur='show(document.regform.userid.value);return false'> "
	       "(2-12字符, 必须全为英文字母或标准汉字)\n", IDLEN, IDLEN);
	printf("<tr><td align=right>请输入密码：</td><td align=left>"
	       "<input type=password name=pass1 size=%d maxlength=%d> (有效长度31位)\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td align=right>请确认密码：</td><td align=left>"
	       "<input type=password name=pass2 size=%d maxlength=%d> (有效长度31位)\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td align=right>请输入昵称：</td><td align=left>"
	       "<input name=username size=20 maxlength=%d> (2-30字符, 中英文不限)\n",
	       NAMELEN - 1);
	printf("<tr><td align=right>请输入您的姓名：</td><td align=left>"
	       "<input name=realname size=30 value='%s'> (请用中文, 至少2个汉字)</td></tr>\n",
	       urlencode(name));
	printf("<tr><td align=right>上站留言(可选)：</td><td align=left>");
	printf
	    ("<textarea name=words rows=3 cols=40 wrap=virutal></textarea></td></tr>");
	printf("<tr><td align=right>"
	       "<input type=\"button\" name=g value=\"请先填写代号\" disabled=true "
	       "onClick=show(document.regform.userid.value);return false><br>");
	printf
	    ("<td align=left><img src=\"/cgi-bin/regimage\" name=\"myImage\">");
	printf("<tr><td align=right>*请输入上面的数字:<td align=left>"
	       "<input name=regpass size=10 maxlength=10>\n");
	printf("</table><br><hr>\n");
	printf
	    ("<input type=button value=提交表格 "
	     "onclick=\"this.value='注册单提交中，请稍候...';this.disabled=true;regform.submit();\" >"
	     " <input type=reset value=重新填写>"
	     " <input type=button value=查看帮助 onclick=\"javascript:{open('/reghelp.html','winreghelp','width=600,height=460,resizeable=yes,scrollbars=yes');return false;}\"\n");
	printf("</form></center></body></html>");

	return 0;
}
