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
	    ("document.regform.g.value=\"看不到数字请点我\"; document.regform.g.disabled=false}\n");
	printf("//--></script>\n");
	printf("<body>");
	if (!strcmp(MY_BBS_ID, "YTHT"))
		http_fatal("暂不开放注册");
	printf("<nobr><center>%s -- 新用户注册<hr>\n", BBSNAME);
	printf
	    ("<font color=green>欢迎加入本站. 以下资料请如实填写. 带*号的项目为必填内容.</font>");
	printf
	    ("</center><br>1、请勿以党和国家领导人或其他名人的真实姓名、字号、艺名、笔名注册； <br>");
	printf("2、请勿以国家机构或其他机构的名称注册； <br>");
	printf("3、请勿注册和其他网友之名相近、相仿的名字； <br>");
	printf("4、请勿注册不文明、不健康之代号； <br>");
	printf("5、请勿注册易产生歧义、引起他人误解之代号； <br><center>");

	printf("<form name=regform method=post action=bbsdoreg>\n");
	printf("<table width=100%%>\n");
	printf("<tr><td align=right>*请输入代号:<td align=left>"
	       "<input name=userid size=%d maxlength=%d "
	       "onBlur=show(document.regform.userid.value);return false> "
	       "(2-12字符, 必须全为英文字母或标准汉字)\n", IDLEN, IDLEN);
	printf("<tr><td align=right>*请输入密码:<td align=left>"
	       "<input type=password name=pass1 size=%d maxlength=%d> (有效长度31位)\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td align=right>*请确认密码:<td align=left>"
	       "<input type=password name=pass2 size=%d maxlength=%d> (有效长度31位)\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td align=right>*请输入昵称:<td align=left>"
	       "<input name=username size=20 maxlength=%d> (2-30字符, 中英文不限)\n",
	       NAMELEN - 1);
	printf("<tr><td align=right>*请输入您的真实姓名:</td><td align=left>"
	       "<input name=realname size=20> (请用中文, 至少2个汉字)</td></tr>\n");
	printf("<tr><td align=right>*目前详细住址:</td><td align=left>"
	       "<input name=address size=40>  (至少8个汉字)</td></tr>\n");
	printf("<tr><td></td><td align=left><font color=red>"
	       "(住址请具体到宿舍或者门牌号码)</font></td></tr>");
	printf("<tr><td align=right>*学校系级或者公司单位:</td><td align=left>"
	       "<input name=dept size=40> (至少7个汉字)</td></tr>\n");
	printf("<tr><td></td><td align=left><font color=red>"
	       "(学校请具体到院系年级，工作单位请具体到部门)</font></td></tr>");
	printf("<tr><td align=right>email地址(可选):</td><td align=left>"
	       "<input name=email size=40></td></tr>\n");
	printf("<tr><td align=right>联络电话(可选):</td><td align=left>"
	       "<input name=phone size=40></td></tr>\n");
	printf
	    ("<tr><td align=right>校友会或者毕业学校(可选):</td><td align=left>"
	     "<input name=assoc size=40></td></tr>\n");
	printf("<tr><td align=right>上站留言(可选):</td><td align=left>");
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
	printf("</form></center>");
	http_quit();
	return 0;
}
