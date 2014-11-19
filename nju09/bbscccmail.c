#include "bbslib.h"

int
bbscccmail_main()
{
	struct fileheader x;
	struct boardmem *brd;
	char file[80], target[80];
	char dir[80];
	int ent;
	int ret;
	int filetime;
	html_header(1);
	check_msg();
	strsncpy(file, getparm("F"), 30);
	if (!file[0])
		strsncpy(file, getparm("file"), 30);
	filetime = atoi(file + 2);
	strsncpy(target, getparm("target"), 30);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܽ��б������");
	changemode(POSTING);
	sprintf(dir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]), currentuser->userid);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &ent, 1);
	if (ret == -1)
		http_fatal("������ļ���");
	printf("<center>%s -- ת������ [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	if (target[0]) {
		brd = getboard(target);
		if (brd == 0)
			http_fatal("��������������ƻ���û���ڸð淢�ĵ�Ȩ��");
		if (!has_post_perm(currentuser, brd))
			http_fatal("��������������ƻ���û���ڸð淢�ĵ�Ȩ��");
		if (noadm4political(target))
			http_fatal
			    ("�Բ���,��Ϊû�а��������Ա����,������ʱ���.");
		return do_cccmail(&x, brd);
	}
	printf("<table><tr><td>\n");
	printf("<font color=red>ת������ע������:<br>\n");
	printf
	    ("��վ�涨ͬ�����ݵ������Ͻ��� 6 ���� 6 ���������������ظ�����");
	printf("Υ�߽�������ڱ�վ���ĵ�Ȩ��<br><br></font>\n");
	printf("���±���: %s<br>\n", nohtml(x.title));
	printf("��������: %s<br>\n", fh2owner(&x));
	printf("���³���: %s ������<br>\n", currentuser->userid);
	printf("<form action=bbscccmail method=post>\n");
	printf("<input type=hidden name=file value=%s>", file);
	printf("ת�ص� <input name=target size=30 maxlength=30> ������. ");
	printf("<input type=submit value=ȷ��></form>");
	return 0;
}

int
do_cccmail(struct fileheader *x, struct boardmem *brd)
{
	FILE *fp, *fp2;
	char board[80], title[512], buf[512], path[200], path2[200],
	    i;
	char tmpfn[80];
	int retv;
	int mark = 0;
	strcpy(board, brd->header.filename);
	sprintf(path, "mail/%c/%s/%s", mytoupper(currentuser->userid[0]), currentuser->userid, fh2fname(x));
	if (brd->header.flag & IS1984_FLAG)
		http_fatal("�ð����ֹת��");
	fp = fopen(path, "r");
	if (fp == 0)
		http_fatal("�ż������Ѷ�ʧ, �޷�ת��");
	sprintf(path2, "bbstmpfs/tmp/%d.tmp", thispid);
	fp2 = fopen(path2, "w");
	if (fgets(buf, 256, fp) != 0) {
		if (!strncmp(buf, "������: ", 8) || !strncmp(buf, "������: ", 8)) {
			for (i = 0; i < 6; i++) {
				if (fgets(buf, 256, fp) == 0)
					break;
				if (buf[0] == '\n')
					break;
			}
		}
		else
			rewind(fp);
	}
	fprintf(fp2,
		"\033[m\033[1m�� ��������ת���� \033[32m%s \033[m\033[1m������ ��\n",
		currentuser->userid);
	fprintf(fp2,
		"\033[m\033[1m�� ԭ���� \033[32m%s \033[m\033[1m�� \033[0m%s\033[1m ���� ��\033[m\n",
		fh2owner(x), Ctime(x->filetime));
	while (1) {
		retv = fread(buf, 1, sizeof (buf), fp);
		if (retv <= 0)
			break;
		fwrite(buf, 1, retv, fp2);
	}
	fclose(fp);
	fclose(fp2);
	if (!strncmp(x->title, "[ת��]", 6)) {
		strsncpy(title, x->title, sizeof (title));
	} else {
		sprintf(title, "[ת��] %.55s", x->title);
	}
	sprintf(tmpfn, "bbstmpfs/tmp/filter.%s.%d", currentuser->userid, thispid);
	copyfile(path2, tmpfn);
	filter_attach(tmpfn);
	if (dofilter(title, tmpfn, 2)) {
	#ifdef POST_WARNING
		char mtitle[256];
		sprintf(mtitle, "[ת�ر���] %s %.60s", board, title);
		mail_file(path2, "delete", mtitle, currentuser->userid);
		updatelastpost("deleterequest");
	#endif
		mark |= FH_DANGEROUS;
	}
	unlink(tmpfn);
	post_article(board, title, path2, currentuser->userid,
		     currentuser->username, fromhost, -1, mark, 0,
		     currentuser->userid, -1);
	unlink(path2);
	printf("'%s' ��ת���� %s ��.<br>\n", nohtml(title), board);
	printf("[<a href='javascript:history.go(-2)'>����</a>]");
	return 0;
}

