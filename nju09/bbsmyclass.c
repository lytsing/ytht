#include "bbslib.h"

void
showmyclasssetting()
{
	struct brcinfo *brcinfo;
	char *myclass, *myclasstitle;
	char *(titlelist[]) = {
	"版面标题", "我的班级", "我的院系", "我的学校", NULL};
	int i;
	printf
	    ("<script>function search(){\n"
	     "newwindow=open('bbssearchboard?element=sel.myclass.value&match='+document.sel.myclass.value,\n"
	     "'', 'width=600,height=460,resizable=yes,scrollbars=yes');\n"
	     "if(newwindow.opener==null) newwindow.opener=self;"
	     "}\n"
	     "function settitle(v) {document.sel.myclasstitle.value=v;}</script>");
	printf("<center>选择在 WWW 底栏显示的版面<br><br>");
	myclass = getparm("myclass");
	myclasstitle = getparm("myclasstitle");
	if (!myclass[0]) {
		brcinfo = brc_readinfo(currentuser->userid);
		myclass = brcinfo->myclass;
		myclasstitle = brcinfo->myclasstitle;
		if (!myclasstitle[0])
			myclasstitle = "版面标题";
	}
	printf
	    ("<table><tr><td><form name='sel' action=bbsmyclass method=post>"
	     "版面名称：<input type=text name=myclass value='%s'> "
	     "<a href='javascript:search()'>搜索</a><br>",
	     void1(nohtml(myclass)));
	printf
	    ("显示为：<input type=text name=myclasstitle value='%s'>&nbsp;&nbsp;",
	     void1(nohtml(myclasstitle)));
	for (i = 0; titlelist[i]; i++) {
		printf("<a href='javascript:settitle(\"%s\")'>%s</a> ",
		       titlelist[i], titlelist[i]);
	}
	printf("<input type=hidden name=submittype value=1>");
	printf
	    ("<br><center><input type=submit value='确定'></form></td></tr></table>");
#if 1
	printf
	    ("<br>没找到自己的学校？请到<a href=home?B=8admin>兄弟院校区区务管理</a>申请开版！"
	     "<br><br>没找到自己的班级？马上到<a href=home?B=L_admin>同学录区区务管理版</a>"
	     "或 (北京大学的系级) <a href=home?B=1admin>北京大学区区务管理版</a>申请开版！");
#endif
}

void
savemyclass()
{
	struct brcinfo *brcinfo;
	struct boardmem *bx;
	char *ptr = getparm("myclass");
	bx = getboard2(ptr);
	if (!bx) {
		printf("<b>没有找到 %s 版，请用“搜索”功能搜索版面</b><br>\n",
		       ptr);
		return;
	}
	if (!has_read_perm_x(currentuser, bx)) {
		printf
		    ("<b>%s 版是一个封闭版面，请先向版务申请加入，或者用“搜索”功能另选其他版面</b><br>\n",
		     ptr);
		return;
	}
	brcinfo = brc_readinfo(currentuser->userid);
	strsncpy(brcinfo->myclass, bx->header.filename,
		 sizeof (brcinfo->myclass));
	ptr = strtrim(getparm("myclasstitle"));
	if (!strcmp(ptr, "版面标题"))
		brcinfo->myclasstitle[0] = 0;
	else
		strsncpy(brcinfo->myclasstitle, ptr,
			 sizeof (brcinfo->myclasstitle));
	brc_saveinfo(currentuser->userid, brcinfo);
	printf
	    ("<b>设置成功</b>（以后可以到菜单“个人工具箱”-->“设定底栏显示的版面”进行修改）<br>");
	printf("<script>top.f4.location.reload();</script>");
}

int
bbsmyclass_main()
{
	int type;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("尚未登录或者超时");
	changemode(ZAP);
	type = atoi(getparm("submittype"));
	if (type == 1) {
		savemyclass();
		printf("<hr>");
	}
	showmyclasssetting();
	return 0;
}
