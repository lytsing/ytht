#include "bbslib.h"

int
bbspwd_main()
{
	int type;
	char pw1[PASSLEN], pw2[PASSLEN], pw3[PASSLEN];
	char md5pass[MD5LEN];
	struct userec tmp;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("您尚未登录, 请先登录");
	changemode(GMENU);
	printf("<body>");
	type = atoi(getparm("type"));
	if (type == 0) {
		printf("%s -- 修改密码 [用户: %s]<hr>\n",
		       BBSNAME, currentuser->userid);
		printf("<form action=bbspwd?type=1 method=post>\n");
		printf
		    ("你的旧密码: <input maxlength=%d size=%d type=password name=pw1><br>\n",
		     PASSLEN - 1, PASSLEN - 1);
		printf
		    ("你的新密码: <input maxlength=%d size=%d type=password name=pw2><br>\n",
		     PASSLEN - 1, PASSLEN - 1);
		printf
		    ("再输入一次: <input maxlength=%d size=%d type=password name=pw3><br><br>\n",
		     PASSLEN - 1, PASSLEN - 1);
		printf("<input type=submit value=确定修改>\n");
		printf("</body>");
		http_quit();
	}
	strsncpy(pw1, getparm("pw1"), PASSLEN);
	strsncpy(pw2, getparm("pw2"), PASSLEN);
	strsncpy(pw3, getparm("pw3"), PASSLEN);
	if (strcmp(pw2, pw3))
		http_fatal("两次密码不相同");
	if (strlen(pw2) < 2)
		http_fatal("新密码太短");
	if (!checkpasswd(currentuser->passwd, currentuser->salt, pw1))
		http_fatal("密码不正确");
	memcpy(&tmp, currentuser, sizeof (tmp));
	tmp.salt = getsalt_md5();
	genpasswd(md5pass, tmp.salt, pw2);
	memcpy(tmp.passwd, md5pass, MD5LEN);
	updateuserec(&tmp, 0);
	printf("[%s] 密码修改成功.", currentuser->userid);
	return 0;
}
