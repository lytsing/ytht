#include "bbslib.h"

//secstr == NULL: all boards
//secstr == "": boards that doesnn't belong to any group
static void
showlist_alphabetical(const char *secstr)
{
	struct boardmem *(data[MAXBOARD]);
	int i, len = 0, total = 0;
	if (secstr) {
		len = strlen(secstr);
		if (len == 0)
			len = 1;
	}
	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
		if (secstr
		    && strncmp(shm_bcache->bcache[i].header.sec1, secstr, len)
		    && strncmp(shm_bcache->bcache[i].header.sec2, secstr, len)
		    )
			continue;
		if (has_view_perm_x(currentuser, &(shm_bcache->bcache[i]))) {
			data[total] = &(shm_bcache->bcache[i]);
			total++;
		}
	}
	if (!total)
		return;
	qsort(data, total, sizeof (struct boardmem *), (void *) cmpboard);
	printf("<table>\n");
	for (i = 0; i < total; i++) {
		char *buf3 = "";
		if (ismybrd(data[i]->header.filename))
			buf3 = " checked";
		if (i && i % 3 == 0)
			printf("</tr>");
		if (i % 3 == 0)
			printf("\n<tr>");
		if (secstr && strncmp(data[i]->header.sec1, secstr, len)) {
			char s[2] = { 0, 0 };
			s[0] = data[i]->header.sec1[0];
			printf("<td><a href=bbsdoc?B=%d>%s(%s)</a>",
			       getbnumx(data[i]),
			       data[i]->header.filename,
			       nohtml(data[i]->header.title));
			printf("(��<i>%s</i>��)</td>",
			       nohtml(getsectree(s)->title));
		} else {
			printf
			    ("<td><input type=checkbox name=%s %s><a href=bbsdoc?B=%d>%s(%s)</a></td>",
			     data[i]->header.filename, buf3,
			     getbnumx(data[i]), data[i]->header.filename,
			     nohtml(data[i]->header.title));
		}
	}
	printf("</table><hr>\n");
}

static void
showlist_grouped()
{
	int i;
	for (i = 0; i < sectree.nsubsec; i++) {
		printf("<b>%s</b>", nohtml(sectree.subsec[i]->title));
		showlist_alphabetical(sectree.subsec[i]->basestr);
	}
}

int
bbsmybrd_main()
{
	int type, mode, mybrdmode;
	char buf[80];
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("��δ��¼���߳�ʱ");
	changemode(ZAP);
	type = atoi(getparm("type"));
	if (type != 0) {
		read_submit();
		http_quit();
	}
	readmybrd(currentuser->userid);
	printf("<style type=text/css>A {color: 000080} </style>\n");
	printf("<body><center>\n");
	printf
	    ("����Ԥ������������(��ĿǰԤ����%d��������������Ԥ��%d��)<br>\n",
	     mybrdnum, GOOD_BRC_NUM);
	printf("<a href=bbsmybrd?mode=0>����ĸ˳������</a> &nbsp; "
	       "<a href=bbsmybrd?mode=1>����������</a><hr>");
	printf("<form action=bbsmybrd?type=1&confirm1=1 method=post>\n");
	printf("<input type=hidden name=confirm1 value=1>\n");
	mode = atoi(getparm("mode"));
	if (mode == 0)
		showlist_alphabetical(NULL);
	else
		showlist_grouped();
	readuservalue(currentuser->userid, "mybrdmode", buf, sizeof (buf));
	mybrdmode = atoi(buf);
	printf
	    ("<br><input type=radio name=mybrdmode value='0' %s>Ԥ����������ʾ�������� "
	     "<input type=radio name=mybrdmode value='1' %s>Ԥ����������ʾӢ������<br>",
	     mybrdmode ? "" : "checked", mybrdmode ? "checked" : "");
	printf
	    ("<input type=submit value=ȷ��Ԥ��> <input type=reset value=��ԭ>\n");
	printf("</form></body>\n");
	http_quit();
	return 0;
}

int
readmybrd(char *userid)
{
	char file[200];
	FILE *fp;
	int l;
	mybrdnum = 0;
	sethomefile(file, currentuser->userid, ".goodbrd");
	fp = fopen(file, "r");
	if (fp) {
		while (fgets(mybrd[mybrdnum], sizeof (mybrd[0]), fp) != NULL) {
			l = strlen(mybrd[mybrdnum]);
			if (mybrd[mybrdnum][l - 1] == '\n')
				mybrd[mybrdnum][l - 1] = 0;
			mybrdnum++;
			if (mybrdnum >= GOOD_BRC_NUM)
				break;
		}
		fclose(fp);
	}
	if (mybrdnum == 0) {
		strcpy(mybrd[mybrdnum], DEFAULTBOARD);
		mybrdnum++;
	}
	return 0;
}

int
ismybrd(char *board)
{
	int i;

	for (i = 0; i < mybrdnum; i++)
		if (!strcasecmp(board, mybrd[i]))
			return 1;
	return 0;
}

int
read_submit()
{
	int i;
	char buf1[200];
	FILE *fp;
	struct boardmem *x;
	mybrdnum = 0;
	if (!strcmp(getparm("confirm1"), ""))
		http_fatal("��������");
	for (i = 0; i < parm_num; i++) {
		if (!strcasecmp(parm_val[i], "on")) {
			if (ismybrd(parm_name[i]))
				continue;
			if (mybrdnum >= GOOD_BRC_NUM)
				http_fatal("����ͼԤ������%d��������",
					   GOOD_BRC_NUM);
			if (!(x = getboard2(parm_name[i]))) {
				printf("����: �޷�Ԥ��'%s'������<br>\n",
				       nohtml(parm_name[i]));
				continue;
			}
			strsncpy(mybrd[mybrdnum], parm_name[i],
				 sizeof (mybrd[0]));
			mybrdnum++;
		}
	}
	sethomefile(buf1, currentuser->userid, ".goodbrd");
	fp = fopen(buf1, "w");
	for (i = 0; i < mybrdnum; i++)
		fprintf(fp, "%s\n", mybrd[i]);
	fclose(fp);
	saveuservalue(currentuser->userid, "mybrdmode", getparm("mybrdmode"));
	printf
	    ("<script>top.f2.location='bbsleft?t=%ld'</script>�޸�Ԥ���������ɹ���������һ��Ԥ����%d��������:<hr>\n",
	     now_t, mybrdnum);
	for (i = 0; i < mybrdnum; i++) {
		x = getboard2(mybrd[i]);
		if (!x)
			continue;
		printf("<a href=home?B=%d>%s(%s)</a><br>", getbnumx(x), mybrd[i],
		       void1(nohtml(x->header.title)));
	}
	//printf("[<a href='javascript:history.go(-2)'>����</a>]");
	return 0;
}
