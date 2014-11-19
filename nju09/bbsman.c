#include "bbslib.h"

int
bbsman_main()
{
	int i, total = 0, mode;
	char board[80], tbuf[256], *cbuf;
	char dir[80];
	struct boardmem *brd;
	char *data = NULL;
	int size, last;
	int fd;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("���ȵ�¼");
	changemode(READING);
	getparmboard(board, sizeof(board));
	mode = atoi(getparm("mode"));
	brd = getboard(board);
	if (brd == 0)
		http_fatal("�����������");
	if (!has_BM_perm(currentuser, brd))
		http_fatal("����Ȩ���ʱ�ҳ");
	if (mode <= 0 || mode > 5)
		http_fatal("����Ĳ���");
	printf("<table>");
	cbuf = "none_op";

	sprintf(dir, "boards/%s/.DIR", board);
	size = file_size(dir);
	if (!size)
		http_fatal("��������");
	fd = open(dir, O_RDWR);
	if (fd < 0)
		http_fatal("��������");
	MMAP_TRY {
		data =
		    mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		close(fd);
		if (data == (void *) -1) {
			MMAP_UNTRY;
			http_fatal("�޷���ȡ�����б�");
		}
		last = 0;
		for (i = 0; i < parm_num && i < 40; i++) {
			if (!strncmp(parm_name[i], "box", 3)) {
				total++;
				if (mode == 1) {
#ifndef WWW_BM_DO_DEL
					do_set(data, size, parm_name[i] + 3,
					       FH_DEL, board);
#else
					do_del(board, parm_name[i] + 3);
#endif
					cbuf = "delete";
				} else if (mode == 2) {
					do_set(data, size, parm_name[i] + 3,
					       FH_MARKED, board);
					cbuf = "mark";
				} else if (mode == 3) {
					do_set(data, size, parm_name[i] + 3,
					       FH_DIGEST, board);
					cbuf = "digest";
				} else if (mode == 5) {
					do_set(data, size, parm_name[i] + 3, 0,
					       board);
					cbuf = "clear_flag";
				}
			}
		}
		printf("</table>");
	}
	MMAP_CATCH {
		close(fd);
	}
	MMAP_END {
		munmap(data, size);
	}
	if (total <= 0)
		printf("����ѡ������<br>\n");
	snprintf(tbuf, sizeof (tbuf), "WWW batch %s on board %s,total %d",
		 cbuf, board, total);
	securityreport(tbuf, tbuf);
	printf("<br><a href=bbsmdoc?B=%d>���ع���ģʽ</a>", getbnumx(brd));
	http_quit();
	return 0;
}

int
do_del(char *board, char *file)
{
	FILE *fp;
	int num = 0, filetime;
	char path[256], dir[256], *id = currentuser->userid;
	struct fileheader f;
	filetime = atoi(file + 2);
	sprintf(dir, "boards/%s/.DIR", board);
	sprintf(path, "boards/%s/%s", board, file);
	fp = fopen(dir, "r");
	if (fp == 0)
		http_fatal("����Ĳ���");
	while (1) {
		if (fread(&f, sizeof (struct fileheader), 1, fp) <= 0)
			break;
		if (f.filetime == filetime) {
			delete_file(dir, sizeof (struct fileheader), num + 1, NULL);
			cancelpost(board, id, &f, !strcmp(id, f.owner));
			updatelastpost(board);
			fclose(fp);
			printf("<tr><td>%s  <td>����:%s <td>ɾ���ɹ�.\n",
			       fh2owner(&f), nohtml(f.title));
			tracelog("%s del %s %s %s", currentuser->userid, board,
				 fh2owner(&f), f.title);
			return 0;
		}
		num++;
	}
	fclose(fp);
	printf("<tr><td><td>%s<td>�ļ�������.\n", file);
	return -1;
}

int
do_set(char *dirptr, int size, char *file, int flag, char *board)
{
	char path[256];
	struct fileheader *f;
	int om, og, nm, ng, filetime;
	int start;
	int total = size / sizeof (struct fileheader);
	filetime = atoi(file + 2);
	sprintf(path, "boards/%s/%s", board, file);

	start = Search_Bin((struct fileheader *)dirptr, filetime, 0, total - 1);
	if (start >= 0) {
		f = (struct fileheader *) (dirptr +
					   start * sizeof (struct fileheader));
		om = f->accessed & FH_MARKED;
		og = f->accessed & FH_DIGEST;
		f->accessed |= flag;
		if (flag == 0)
			f->accessed = 0;
		nm = f->accessed & FH_MARKED;
		ng = f->accessed & FH_DIGEST;
		printf("<tr><td>%s<td>����:%s<td>��ǳɹ�.\n",
		       fh2owner(f), nohtml(f->title));
		if ((!om) && (nm))
			tracelog("%s mark %s %s %s", currentuser->userid, board,
				 fh2owner(f), f->title);
		else if ((om) && (!nm))
			tracelog("%s unmark %s %s %s", currentuser->userid,
				 board, fh2owner(f), f->title);
		else if ((!og) && (ng))
			tracelog("%s digest %s %s %s", currentuser->userid,
				 board, fh2owner(f), f->title);
		else if ((og) && (!ng))
			tracelog("%s undigest %s %s %s", currentuser->userid,
				 board, fh2owner(f), f->title);
		return 0;
	}
	printf("<td><td><td>%s<td>�ļ�������.\n", file);
	return -1;
}
