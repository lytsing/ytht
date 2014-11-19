#include "bbslib.h"

int
bbsccc_main()
{
	struct fileheader x;
	struct boardmem *brd, *brd1;
	char board[80], file[80], target[80];
	char dir[80];
	int ret;
	int filetime;
	int ent;
	html_header(1);
	check_msg();
	getparmboard(board, sizeof (board));

	strsncpy(file, getparm2("F", "file"), 30);
	filetime = atoi(file + 2);
	strsncpy(target, getparm("target"), 30);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܽ��б������");
	if (!(brd1 = getboard(board)))
		http_fatal("�����������");
	changemode(POSTING);
	sprintf(dir, "boards/%s/.DIR", board);
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
		return do_ccc(&x, brd1, brd);
	}
	printf("<table><tr><td>\n");
	printf("<font color=red>ת������ע������:<br>\n");
	printf
	    ("��վ�涨ͬ�����ݵ������Ͻ��� 6 ���� 6 ���������������ظ�����");
	printf("Υ�߽�������ڱ�վ���ĵ�Ȩ��<br><br></font>\n");
	printf("���±���: %s<br>\n", nohtml(x.title));
	printf("��������: %s<br>\n", fh2owner(&x));
	printf("ԭ������: %s<br>\n", board);
	printf("<form action=bbsccc method=post>\n");
	printf("<input type=hidden name=board value=%s>", board);
	printf("<input type=hidden name=file value=%s>", file);
	printf("ת�ص� <input name=target size=30 maxlength=30> ������. ");
	printf("<input type=submit value=ȷ��></form>");
	return 0;
}

int
do_ccc(struct fileheader *x, struct boardmem *brd1, struct boardmem *brd)
{
	FILE *fp, *fp2;
	char board2[80], board[80], title[512], buf[512], path[200], path2[200],
	    i;
	char tmpfn[80];
	int hide1, hide2, retv;
	int mark = 0;
	strcpy(board2, brd->header.filename);
	strcpy(board, brd1->header.filename);
	sprintf(path, "boards/%s/%s", board, fh2fname(x));
	if (brd->header.flag & IS1984_FLAG)
		http_fatal("�ð����ֹת��");
	hide1 = hideboard_x(brd1);
	hide2 = hideboard_x(brd);
	if (hide1 && !hide2)
		http_fatal("�Ƿ�ת��");
	fp = fopen(path, "r");
	if (fp == 0)
		http_fatal("�ļ������Ѷ�ʧ, �޷�ת��");
	sprintf(path2, "bbstmpfs/tmp/%d.tmp", thispid);
	fp2 = fopen(path2, "w");
	for (i = 0; i < 3; i++)
		if (fgets(buf, 256, fp) == 0)
			break;
	fprintf(fp2,
		"\033[m\033[1m�� ��������ת���� \033[32m%s \033[m\033[1m������ ��\n",
		hide1 ? "����" : board);
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
	sprintf(tmpfn, "bbstmpfs/tmp/filter.%s.%d", currentuser->userid,
		thispid);
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
	post_article(board2, title, path2, currentuser->userid,
		     currentuser->username, fromhost, -1, mark, 0,
		     currentuser->userid, -1);
	unlink(path2);
	printf("'%s' ��ת���� %s ��.<br>\n", nohtml(title), board2);
	printf("[<a href='javascript:history.go(-2)'>����</a>]");
	return 0;
}
