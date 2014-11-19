#include "bbslib.h"

int
bbsinfo_main()
{
	int type;
	struct userdata currentdata;
	html_header(1);
	check_msg();
	printf("<body>");
	if (!loginok || isguest)
		http_fatal("您尚未登录");
	changemode(EDITUFILE);
	type = atoi(getparm("type"));
	printf("%s -- 用户个人资料<hr>\n", BBSNAME);
	if (type != 0) {
		check_info();
		http_quit();
	}
	loaduserdata(currentuser->userid, &currentdata);
	if (currentuser->mypic) {
		printf("<table align=right><tr><td><center>");
		printmypic(currentuser->userid);
		printf("<br>当前头像</center></td></tr></table>");
	}
	printf
	    ("<form action=bbsinfo?type=1 method=post enctype='multipart/form-data'>");
	printf("您的帐号: %s<br>\n", currentuser->userid);
	printf
	    ("您的昵称: <input type=text name=nick value='%s' size=24 maxlength=%d>%d个汉字或%d个英文字母以内<br>",
	     void1(nohtml(currentuser->username)), NAMELEN, NAMELEN / 2,
	     NAMELEN);
	printf("发表大作: %d 篇<br>\n", currentuser->numposts);
//      printf("信件数量: %d 封<br>\n", currentuser.nummails);
	printf("上站次数: %d 次<br>\n", currentuser->numlogins);
	printf("上站时间: %ld 分钟<br>\n", currentuser->stay / 60);
	printf
	    ("真实姓名: <input type=text name=realname value='%s' size=16 maxlength=%d>%d个汉字或%d个英文字母以内<br>\n",
	     void1(nohtml(currentdata.realname)), NAMELEN, NAMELEN / 2,
	     NAMELEN);
	printf
	    ("居住地址: <input type=text name=address value='%s' size=40 maxlength=%d>%d个汉字或%d个英文字母以内<br>\n",
	     void1(nohtml(currentdata.address)), STRLEN, STRLEN / 2, STRLEN);
	printf("帐号建立: %s<br>", Ctime(currentuser->firstlogin));
	printf("最近光临: %s<br>", Ctime(currentuser->lastlogin));
	printf("来源地址: %s<br>", inet_ntoa(from_addr));
	printf
	    ("电子邮件: <input type=text name=email value='%s' size=32 maxlength=%d>%d 个英文字母以内<br>\n",
	     void1(nohtml(currentdata.email)), sizeof (currentdata.email) - 1,
	     sizeof (currentdata.email) - 1);
	printf("上载头像: <input type=file name=mypic> 12K 字节以内<br>");
#if 0
	printf
	    ("出生日期: <input type=text name=year value=%d size=4 maxlength=4>年",
	     currentuser.birthyear + 1900);
	printf("<input type=text name=month value=%d size=2 maxlength=2>月",
	       currentuser.birthmonth);
	printf("<input type=text name=day value=%d size=2 maxlength=2>日<br>\n",
	       currentuser.birthday);
	printf("用户性别: ");
	printf("男<input type=radio value=M name=gender %s>",
	       currentuser.gender == 'M' ? "checked" : "");
	printf("女<input type=radio value=F name=gender %s><br>",
	       currentuser.gender == 'F' ? "checked" : "");
#endif
	printf
	    ("<input type=submit value=确定> <input type=reset value=复原>\n");
	printf("</form>");
	printf("<hr>");
	printf("</body></html>");
	return 0;
}

int
check_info()
{
	int m;
	char buf[256];
	struct userdata currentdata;
	struct userec tmp;
	struct parm_file *parmFile;

	loaduserdata(currentuser->userid, &currentdata);
	memcpy(&tmp, currentuser, sizeof (tmp));
	strsncpy(buf, getparm("nick"), 30);
	for (m = 0; m < strlen(buf); m++)
		if ((buf[m] < 32 && buf[m] > 0) || buf[m] == -1)
			buf[m] = ' ';
	if (strlen(buf) > 1) {
		strcpy(tmp.username, buf);
	} else {
		printf("警告: 昵称太短!<br>\n");
	}
	strsncpy(buf, getparm("realname"), 9);
	if (strlen(buf) > 1) {
		strcpy(currentdata.realname, buf);
	} else {
		printf("警告: 真实姓名太短!<br>\n");
	}
	strsncpy(buf, getparm("address"), 40);
	if (strlen(buf) > 8) {
		strcpy(currentdata.address, buf);
	} else {
		printf("警告: 居住地址太短!<br>\n");
	}
	strsncpy(buf, getparm("email"), 32);
	if (strlen(buf) > 8 && strchr(buf, '@')) {
		strcpy(currentdata.email, buf);
	} else {
		printf("警告: email地址不合法!<br>\n");
	}
	if ((parmFile = getparmfile("mypic"))) {
		if (parmFile->len > 12 * 1024) {
			printf("警告: 图片尺寸最大 12K 字节，请压缩图片后重新上载。<br>");
		} else {
			char picfile[256];
			int fd;
			sethomefile(picfile, currentuser->userid, "mypic");
			fd = open(picfile, O_CREAT | O_WRONLY, 0660);
			if (fd > 0) {
				write(fd, parmFile->content, parmFile->len);
				close(fd);
			}
			tmp.mypic = 1;
		}
	}
	updateuserec(&tmp, 0);
	saveuserdata(currentuser->userid, &currentdata);
	printf("[%s] 个人资料修改成功.", currentuser->userid);
	return 0;
}
