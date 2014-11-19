#include "bbslib.h"

void
printLoginFormUTF8()
{
#ifdef HTTPS_DOMAIN
	printf("<form name=l action=https://" HTTPS_DOMAIN
	       "/" SMAGIC "/bbslogin method=post target=_top>");
	printf("<input type=hidden name=usehost value='http%c://%s:%s'",
	       !strcasecmp(getsenv("HTTPS"), "ON"),
	       getsenv("HTTP_HOST"), getsenv("SERVER_PORT"));
#else
	printf
	    ("<form name=l accept-charset=\"gb2312\" action=bbslogin method=post target=_top>");
#endif
	printf("<input type=hidden name=lastip1 value=''>"
	       "<input type=hidden name=lastip2 value=''>"
	       "帐号：<input type=text name=id maxlength=%d size=11><br>"
	       "密码：<input type=password name=pw maxlength=%d size=11><br>"
	       "<a href=/ipmask.html target=_blank>验证</a>：<select name=ipmask>\n"
	       "<option value=0 selected>单IP</option>\n"
	       "<option value=6>64 IP</option>\n"
	       "<option value=8>256 IP</option>\n"
	       "<option value=15>32768 IP</option></select><br>"
	       "<input type=submit value=登录>"
	       "<input type=submit value=注册 onclick=\"{top.location.href='/"
	       SMAGIC "/bbsemailreg';return false}\">\n"
	       "</form>\n", IDLEN, PASSLEN - 1);
}

static void
printBlogSetupForm()
{
	printf("<br><b>建立 BLOG</b><br>");
	printf("<form action=blogsetup method=post>");
	printf("<input type=hidden name=doit value=1 />");
	printf("BLOG 的标题：<input type=text name=title "
		"value=\"尚未指定标题\" />");
	printf("<input type=submit value=\"确定\">");
	printf("</form>");
}

static void
logCreation(struct Blog *blog)
{
	char buf[256];
	snprintf(buf, sizeof (buf), "%s\n", blog->userid);
	f_append("blog/blog.create", buf);
}

int
blogsetup_main()
{
	struct Blog blog;
	struct userec tmp;

	printXBlogHeader();
	check_msg();
	changemode(READING);
	if (!loginok || isguest) {
		printf
		    ("请先登录，只有注册通过的用户才能才能建立 blog。");
		printLoginFormUTF8();
		printXBlogEnd();
		return 0;
	}
	if (openBlog(&blog, currentuser->userid)>=0) {
		char buf[256];
		snprintf(buf, sizeof(buf), "blog?U=%s", blog.config->useridEN);
		closeBlog(&blog);
		redirect(buf);
		printXBlogEnd();
		return 0;
	}
	if (!user_perm(currentuser, PERM_POST)) {
		printf
		    ("只有注册通过的用户才能建立 blog，请先填写<a href=bbsform>注册单</a>或回复注册信件。");
		printXBlogEnd();
		return 0;
	}
	if (!atoi(getparm("doit"))) {
		printBlogSetupForm();
		printXBlogEnd();
		return 0;
	}
	createBlog(currentuser->userid);
	if (openBlogW(&blog, currentuser->userid) < 0) {
		printf("建立 blog 失败，请通知系统管理员。");
	} else {
		modifyConfig(&blog, 0);
		printf("已经建立了您的 blog，点<a href=\"blog?U=%s\">这里</a>继续。",
		     blog.config->useridEN);
		logCreation(&blog);
		closeBlog(&blog);
		memcpy(&tmp, currentuser, sizeof (tmp));
		tmp.hasblog = 1;
		updateuserec(&tmp, 0);
	}
	printXBlogEnd();
	return 0;
}
