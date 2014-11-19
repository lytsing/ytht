#include "bbslib.h"

char bbsfwd_tmpfn[80];

int
bbsfwd_main()
{
	struct fileheader x;
	char board[80], file[80], maildir[80], target[80], dir[80], path[80], buf[256];
	char *ptr;
	struct userdata currentdata;
	struct userec *up;
	FILE *fp, *fp1;
	int i, retv, filetime;
	int ret, ent;
	html_header(1);
	check_msg();
	getparmboard(board, sizeof(board));
	strsncpy(file, getparm("F"), 30);
	if (!file[0])
		strsncpy(file, getparm("file"), 30);
	filetime = atoi(file + 2);
	strsncpy(target, getparm("target"), 30);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܽ��б������");
	changemode(SMAIL);
	sprintf(maildir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]),
		currentuser->userid);
	if ((ptr=check_mailperm(currentuser)))
		http_fatal(ptr);
	if (!getboard(board))
		http_fatal("�����������");
	sprintf(dir, "boards/%s/.DIR", board);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &ent, 1);
	if (ret == -1)
		http_fatal("������ļ���");
	printf("<center>%s -- ת��/�Ƽ������� [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	if (target[0]) {
		if (!strstr(target, "@")) {
			if (getuser(target, &up) <= 0)
				http_fatal("�����ʹ�����ʺ�");
			strcpy(target, up->userid);
		}
		snprintf(bbsfwd_tmpfn, 80, "bbstmpfs/tmp/bbsfwd.%s.%d",
			 currentuser->userid, thispid);
		sprintf(path, "boards/%s/%s", board, fh2fname(&x));
		fp = fopen(bbsfwd_tmpfn, "w");
		fp1 = fopen(path, "r");
		if (!fp || !fp1) {
			if (fp)
				fclose(fp);
			if (fp1)
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
			hideboard(board) ? "����" : board);
		fprintf(fp,
			"\033[m\033[1m�� ԭ���� \033[32m%s \033[m\033[1m�� \033[0m%s\033[1m ���� ��\033[m\n",
			fh2owner(&x), Ctime(x.filetime));
		while (1) {
			retv = fread(buf, 1, sizeof (buf), fp1);
			if (retv <= 0)
				break;
			fwrite(buf, 1, retv, fp);
		}
		fclose(fp1);
		fclose(fp);
		do_fwd(&x, board, target);
		unlink(bbsfwd_tmpfn);
		return 0;

	}
	loaduserdata(currentuser->userid, &currentdata);
	printf("<table><tr><td>\n");
	printf("���±���: %s<br>\n", nohtml(x.title));
	printf("��������: %s<br>\n", fh2owner(&x));
	printf("ԭ������: %s<br>\n", board);
	printf("<form action=bbsfwd method=post>\n");
	printf("<input type=hidden name=board value=%s>", board);
	printf("<input type=hidden name=file value=%s>", file);
	printf
	    ("������ת�ĸ� <input name=target size=30 maxlength=30 value='%s'> (������Է���id��email��ַ). <br>\n",
	     void1(nohtml(currentdata.email)));
	printf("<input type=submit value=ȷ��ת��></form>");
	return 0;
}

int
do_fwd(struct fileheader *x, char *board, char *target)
{
	char title[512];
	if (!file_exist(bbsfwd_tmpfn))
		http_fatal("�ļ������Ѷ�ʧ, �޷�ת��");
	sprintf(title, "[ת��] %s", x->title);
	title[60] = 0;
	post_mail(target, title, bbsfwd_tmpfn, currentuser->userid,
		  currentuser->username, fromhost, -1, 0);
	printf("������ת�ĸ�'%s'<br>\n", nohtml(target));
	printf("[<a href='javascript:history.go(-2)'>����</a>]");
	return 0;
}
