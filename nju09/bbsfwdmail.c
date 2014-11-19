#include "bbslib.h"

int
bbsfwdmail_main()
{
	struct fileheader x;
	char file[80], target[80], dir[80], path[80], buf[256];
	char bbsfwdmail_tmpfn[80];
	char *ptr;
	struct userdata currentdata;
	struct userec *up;
	FILE *fp, *fp1;
	int ent, ret, filetime;
	int i, retv;
	html_header(1);
	check_msg();
	strsncpy(file, getparm("F"), 30);
	if (!file[0])
		strsncpy(file, getparm("file"), 30);
	filetime = atoi(file + 2);
	strsncpy(target, getparm("target"), 30);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܽ��б������");
	changemode(SMAIL);
	sprintf(dir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]),
		currentuser->userid);
	if ((ptr=check_mailperm(currentuser)))
		http_fatal(ptr);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &ent, 1);
	if (ret == -1)
		http_fatal("������ż���");
	printf("<center>%s -- ת��/�Ƽ������� [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);

	if (target[0]) {
		if (!strstr(target, "@")) {
			if (getuser(target, &up) <= 0)
				http_fatal("�����ʹ�����ʺ�");
			strcpy(target, up->userid);
		}
		snprintf(bbsfwdmail_tmpfn, 80, "bbstmpfs/tmp/bbsfwdmail.%s.%d",
			 currentuser->userid, thispid);
		snprintf(path, 80, "mail/%c/%s/%s",
			 mytoupper(currentuser->userid[0]), currentuser->userid,
			 fh2fname(&x));
		fp = fopen(bbsfwdmail_tmpfn, "w");
		fp1 = fopen(path, "r");
		if (!fp || !fp1) {
			if(fp)
				fclose(fp);
			if(fp1)
				fclose(fp1);
			http_fatal
			    ("�ż������Ѷ�ʧ�������������ڲ�������ת��ʧ��");
		}
		if (fgets(buf, 256, fp1) != 0) {
			if (!strncmp(buf, "������: ", 8)
			    || !strncmp(buf, "������: ", 8)) {
				for (i = 0; i < 6; i++) {
					if (fgets(buf, 256, fp1) == 0)
						break;
					if (buf[0] == '\n')
						break;
				}
			} else
				rewind(fp1);
		}
		fprintf(fp,
			"\033[m\033[1m�� ��������ת���� \033[32m%s \033[m\033[1m������ ��\n",
			currentuser->userid);
		fprintf(fp,
			"\033[m\033[1m�� ԭ���� \033[32m%s \033[m\033[1m�� \033[0m%s\033[1m ���� ��\033[m\n",
			fh2owner(&x), Ctime(x.filetime));
		while (1) {
			retv = fread(buf, 1, sizeof (buf), fp1);
			if (retv <= 0)
				break;
			fwrite(buf, 1, retv, fp);
		}
		fprintf(fp,
		"--\n\033[m\033[1;32m�� ת��:��%s %s��[FROM: %-.20s]\033[m\n",
		MY_BBS_NAME, MY_BBS_DOMAIN, fromhost);
		fclose(fp1);
		fclose(fp);
		do_fwdmail(bbsfwdmail_tmpfn, &x, target);
		unlink(bbsfwdmail_tmpfn);
		return 0;
	}
	loaduserdata(currentuser->userid, &currentdata);
	printf("<table><tr><td>\n");
	printf("���±���: %s<br>\n", nohtml(x.title));
	printf("��������: %s<br>\n", fh2owner(&x));
	printf("�ż�����: %s ������<br>\n", currentuser->userid);
	printf("<form action=bbsfwdmail method=post>\n");
	printf("<input type=hidden name=file value=%s>", file);
	printf
	    ("������ת�ĸ� <input name=target size=30 maxlength=30 value='%s'> (������Է���id��email��ַ). <br>\n",
	     void1(nohtml(currentdata.email)));
	printf("<input type=submit value=ȷ��ת��></form>");
	return 0;
}

int
do_fwdmail(char *fn, struct fileheader *x, char *target)
{
	char title[512];
	if (!file_exist(fn))
		http_fatal("�ż������Ѷ�ʧ, �޷�ת��");
	sprintf(title, "[ת��] %s", x->title);
	title[60] = 0;
	post_mail(target, title, fn, currentuser->userid,
		  currentuser->username, fromhost, -1, 0);
	printf("������ת�ĸ�'%s'<br>\n", nohtml(target));
	printf("[<a href='javascript:history.go(-2)'>����</a>]");
	return 0;
}

