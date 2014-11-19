#include "bbslib.h"

int
bbsform_main()
{
	int type;
	struct userdata currentdata;
	html_header(1);
	type = atoi(getparm("type"));
	if (!loginok || isguest)
		http_fatal("您尚未登录, 请重新登录。");
	changemode(NEW);
	printf("%s -- 填写注册单<hr>\n", BBSNAME);
	check_if_ok();
	if (type == 1) {
		check_submit_form();
		http_quit();
	}
	loaduserdata(currentuser->userid, &currentdata);
	printf
	    ("您好, %s, 注册单通过后即可获得注册用户的权限, 下面各项务必请认真填写<br><hr>\n",
	     currentuser->userid);
	printf("<form method=post action=bbsform?type=1>\n");
	printf
	    ("真实姓名: <input name=realname type=text maxlength=8 size=8 value='%s'><br>\n",
	     void1(nohtml(currentdata.realname)));
	printf
	    ("居住地址: <input name=address type=text maxlength=32 size=32 value='%s'><br>\n",
	     void1(nohtml(currentdata.address)));
	printf
	    ("联络电话(可选): <input name=phone type=text maxlength=32 size=32><br>\n");
	printf
	    ("学校系级/公司单位: <input name=dept maxlength=60 size=40><br>\n");
	printf
	    ("校友会/毕业学校(可选): <input name=assoc maxlength=60 size=42><br><hr><br>\n");
	printf
	    ("<input type=submit value=注册>  <input type=reset value=取消>  <input type=button value=查看帮助 onclick=\"javascript:{open('/reghelp.html','winreghelp','width=600,height=460,resizeable=yes,scrollbars=yes');}\">\n");
	http_quit();
	return 0;
}

void
check_if_ok()
{
	if (user_perm(currentuser, PERM_LOGINOK))
		http_fatal("您已经通过本站的身份认证, 无需再次填写注册单.");
	if (has_fill_form(currentuser->userid))
		http_fatal("目前站长尚未处理您的注册单，请耐心等待.");
}

void
check_submit_form()
{
	FILE *fp;
	char dept[80], phone[80], assoc[80];
	struct userdata currentdata;
	if (strlen(getparm("realname")) < 4)
		http_fatal("请输入真实姓名(请用中文, 至少2个字)");
	if (strlen(getparm("address")) < 16)
		http_fatal("居住地址长度至少要16个字符(或8个汉字)");
	if (strlen(getparm("dept")) < 14)
		http_fatal
		    ("工作单位或学校系级的名称长度至少要14个字符(或7个汉字)");
	loaduserdata(currentuser->userid, &currentdata);
	fp = fopen("new_register", "a");
	if (fp == 0)
		http_fatal("注册文件错误，请通知SYSOP");
	strsncpy(currentdata.realname, getparm("realname"),
		 sizeof (currentdata.realname));
	strsncpy(dept, getparm("dept"), 60);
	strsncpy(currentdata.address, getparm("address"),
		 sizeof (currentdata.address));
	strsncpy(phone, getparm("phone"), 60);
	strsncpy(assoc, getparm("assoc"), 60);
	fprintf(fp, "usernum: %d, %s\n", getuser(currentuser->userid, NULL),
		Ctime(now_t));
	fprintf(fp, "userid: %s\n", currentuser->userid);
	fprintf(fp, "realname: %s\n", currentdata.realname);
	fprintf(fp, "dept: %s\n", dept);
	fprintf(fp, "addr: %s\n", currentdata.address);
	fprintf(fp, "phone: %s\n", phone);
	fprintf(fp, "assoc: %s\n", assoc);
	fprintf(fp, "rereg: 1\n");
	fprintf(fp, "----\n");
	fclose(fp);
	printf
	    ("您的注册单已成功提交. 站长检验过后会给您发信, 请留意您的信箱.<br>"
	     "<a href=bbsboa>浏览" MY_BBS_NAME "</a>");
}
