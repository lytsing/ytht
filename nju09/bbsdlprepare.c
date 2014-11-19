#include "bbslib.h"

int
bbsdlprepare_main()
{
	char taskfile[512];
	char *action;
	int done;
	html_header(1);
	if (!loginok || isguest)
		http_fatal("����δ��¼, ���ȵ�¼");
	sprintf(taskfile, "dllist/%s.task", currentuser->userid);
	printf("<body>���������ṩ�����ļ�������û��������ء�"
	       "�ļ����� rar ��ʽ������������ rar �ļ��������룬"
	       "ϵͳ��ʹ�ø��������ɸ����ļ���ѹ���ļ� %s.***.Personal.rar��"
	       "Ȼ�����Ϳ������ظ��ļ��������趨�������ѹ�����ļ���"
	       "����ļ����Ǽ�ʱ���ɵģ���Ҫ�Ⱥ� 1 Сʱ���ҡ�<br><br>",
	       currentuser->userid);
	if (!file_exist(taskfile)) {
		printf("<hr>");
		http_fatal("��û�и����ļ�");
	}
	action = getparm("action");
	if (!strcmp(action, "setpass")) {
		trySetDLPass(taskfile);
	}
	done = reportStatus(taskfile);
	printSetDLPass();
	return 0;
}

int
trySetDLPass(char *taskfile)
{
	char *passwd, *p1, *p2;
	char buf[256], path[512];
	passwd = getparm("passwd");
	p1 = getparm("pass");
	p2 = getparm("passverify");
	printf("<hr>");
	if (!checkpasswd(currentuser->passwd, currentuser->salt, passwd)) {
		printf("<font color=red>BBS �û��������벻��ȷ��</font><br>");
		return -1;
	}
	if (strcmp(p1, p2)) {
		printf
		    ("<font color=red>ѹ���ļ����������������������</font><br>");
		return -1;
	}
	savestrvalue(taskfile, "pass", p1);
	savestrvalue(taskfile, "done", "0");
	printf("<font color=red>���ܴ���ļ����������óɹ���</font><br>");
	readstrvalue(taskfile, "rarfile", buf, sizeof (buf));
	if (*buf) {
		sprintf(path, HTMPATH "/download/%s", buf);
		unlink(path);
		savestrvalue(taskfile, "rarfile", "");
	}
	return 0;
}

int
printSetDLPass()
{
	printf("<hr>���ô�����루���д������ɾ��������ļ����������ɣ�<br>");
	printf("<form action=bbsdlprepare?action=setpass method=post>"
	       "BBS �û�������<input type=password name=passwd maxlenght=%d size=%d><br>"
	       "���ܴ���ļ�(*.rar)������<input type=password name=pass maxlength=20 size=20><br>"
	       "������һ�μ�������<input type=password name=passverify maxlength=20 size=20><br>"
	       "<input type=submit value=\"ȷ��\">"
	       "</form>", PASSLEN - 1, PASSLEN - 1);
	return 0;
}

int
reportStatus(char *taskfile)
{
	char buf[512];
	printf("<hr><font color=green>Ŀǰ״̬��<br>");
	if (readstrvalue(taskfile, "done", buf, sizeof (buf)) >= 0
	    && *buf == '1') {
		readstrvalue(taskfile, "rarfile", buf, sizeof (buf));
		if (!*buf) {
			printf("���´�����ִ�����֪ͨ sysop��<br>");
		} else {
			printf("����Ѿ���ɣ������ء�<br>");
			printf
			    ("����ļ����ӣ�<a href=/download/%s>����</a><br>",
			     buf);
			printf("���ʹ�õ��� IE ����������� ID �������֣�"
			       "���ҷ����޷����ص�������Ҫ�� IE �Ĳ˵������ҵ���<br>"
			       "����Tools  =>> Internet Options...  =>>  Advanced =>> Always send URLs as UTF-8<br>"
			       "ȡ�����ѡ����������� IE �������</font><br>");

		}
		return 1;
	}
	if (readstrvalue(taskfile, "pass", buf, sizeof (buf)) >= 0 && *buf) {
		printf("�Ѿ��趨���ܴ���ļ������룬��Ⱥ�����<br>");
		printf("����ļ����ӣ����ޡ�<br>");
	} else {
		printf("��δ�趨���ܴ���ļ������룬�����������롣<br>");
		printf("����ļ����ӣ����ޡ�<br>");
	}
	printf("</font>");
	return 0;
}
