#include "bbslib.h"

int
bbsbkndoc_main()
{
	FILE *fp;
	char board[80], bkn[80], dir[160], *ptr, genbuf[STRLEN];
	struct boardmem *x1;
	struct fileheader x;
	int i, start, total;
	html_header(1);
	printf("</head>");
	changemode(BACKNUMBER);
	check_msg();
	getparmboard(board, sizeof(board));
	strsncpy(bkn, getparm("bkn"), 32);
	ptr = bkn;
	while (*ptr) {
		if (*ptr != 'B' && *ptr != '.' && !isdigit(*ptr))
			http_fatal("����Ĺ�����");
		ptr++;
	}
	if (strlen(bkn) < 3)
		http_fatal("����Ĺ�����");

	x1 = getboard(board);
	if (x1 == 0)
		nosuchboard(board, "bbsbkndoc");
	else {
		updateinboard(x1);
		strcpy(board, x1->header.filename);
		sprintf(dir, "boards/.backnumbers/%s/%s", board, bkn);
		if (!file_exist(dir))
			http_fatal("û��������");
		sprintf(dir, "boards/.backnumbers/%s/%s/.DIR", board, bkn);
		fp = fopen(dir, "r");
		total = file_size(dir) / sizeof (struct fileheader);
		start = getdocstart(total, w_info->t_lines);
		printf("<body topmargin=0>");
		printf("<nobr><center>\n");
		sprintf(genbuf, "�������� ������[%d]", total);
		printboardtop(x1, 5, genbuf);
		if (total <= 0)
			http_fatal("�������Ŀǰû������");
		printf("<table cellSpacing=0 cellPadding=2>\n");
		printf
		    ("<tr class=docbgcolor><td>���<td>״̬<td>����<td>����<td>����<td>�Ǽ�<td>����</td>\n");
		if (fp) {
			fseek(fp, (start - 1) * sizeof (struct fileheader),
			      SEEK_SET);
			for (i = 0; i < w_info->t_lines; i++) {
				char filename[255];
				char *ptr;
				if (fread(&x, sizeof (x), 1, fp) <= 0)
					break;
				if (x.accessed & FH_HIDE) {
					printf
					    ("<tr><td>%d<td>&nbsp;<td>����<td>����<td>-�����Ѿ���ɾ��-<td>\n",
					     start + i);
					continue;
				}
				ptr = flag_str2(x.accessed, 1);
				sprintf(filename,
					"boards/.backnumbers/%s/%s/%s", board,
					bkn, fh2fname(&x));
				printf("<tr><td>%d<td>%s<td>%s", start + i, ptr,
				       userid_str(x.owner));
				printf("<td>%12.12s", Ctime(x.filetime) + 4);
				x.title[40] = 0;
				printf
				    ("<td><a href=bbsbkncon?B=%d&bkn=%s&file=%s&num=%d>%s%s </a>%s",
				     getbnumx(x1), bkn, fh2fname(&x), start + i - 1,
				     strncmp(x.title, "Re: ", 4) ? "�� " : "",
				     void1(titlestr(x.title)),
				     size_str(eff_size(filename)));
				printf("<td><font color=%s>%d</font>\n",
				       x.staravg50 ? "red" : "black",
				       x.staravg50 / 50);
				printf("<td><font color=%s>%d��</font>\n",
				       x.hasvoted ? "red" : "black",
				       x.hasvoted);
			}
			printf("</table>");
			printhr();
			fclose(fp);
		}
		sprintf(genbuf, "bbsbkndoc?B=%d&bkn=%s", getbnumx(x1), bkn);
		bbsdoc_helper(genbuf, start, total, w_info->t_lines);
		printf("<table><tr><td><form name=form1 action=bbsbkndoc>\n");
		printf("<input type=hidden name=B value=%d>", getbnumx(x1));
		printf("<input type=hidden name=bkn value=%s>", bkn);
		printf
		    ("<input type=submit value=��ת��> �� <input type=text name=start size=4> ƪ");
		printf("</form>\n");
		printf("</td><td><form name=form2 action=bbsdoc>\n");
		printf
		    ("<input type=submit value=��ת��><input type=text name=board size=5> ������");
		printf("</form></td></tr></table>\n");
		printf("</body>");
		http_quit();
	}
	return 0;
}
