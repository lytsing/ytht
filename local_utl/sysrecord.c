#include "bbs.h"
#include "ythtbbs.h"

char *
setbfile(buf, boardname, filename)
char *buf, *boardname, *filename;
{
	sprintf(buf, MY_BBS_HOME "/boards/%s/%s", boardname, filename);
	return buf;
}

char *
setbdir(buf, boardname)
char *buf, *boardname;
{
	char dir[STRLEN];

	strncpy(dir, DOT_DIR, STRLEN);
	dir[STRLEN - 1] = '\0';
	sprintf(buf, MY_BBS_HOME "/boards/%s/%s", boardname, dir);
	return buf;
}

void
getcross(filepath, filepath2, nboard, posttitle)
char *filepath, *filepath2, *nboard, *posttitle;
{
	FILE *inf, *of;
	char buf[256];
	time_t now;

	now = time(0);
	inf = fopen(filepath2, "r");
	if (inf == NULL)
		return;
	of = fopen(filepath, "w");
	if (of == NULL) {
		fclose(inf);
		return;
	}

	fprintf(of, "������: deliver (�Զ�����ϵͳ), ����: %s\n", nboard);
	fprintf(of, "��  ��: %s\n", posttitle);
	fprintf(of, "����վ: �Զ�����ϵͳ (%24.24s)\n\n", ctime(&now));
	fprintf(of, "����ƪ���������Զ�����ϵͳ��������\n\n");
	while (fgets(buf, 256, inf) != NULL)
		fprintf(of, "%s", buf);
	fclose(inf);
	fclose(of);
}

int
post_cross(filename, nboard, posttitle, owner)
char *filename, *nboard, *posttitle, *owner;
{
	struct fileheader postfile;
	char filepath[STRLEN];
	char buf[256];
	time_t now;

	memset(&postfile, 0, sizeof (postfile));

	now = time(0);
	setbfile(filepath, nboard, "");
	now = trycreatefile(filepath, "M.%d.A", now, 100);
	if (now < 0)
		return -1;
	postfile.filetime = now;
	postfile.thread = now;
	fh_setowner(&postfile, owner, 0);
	strsncpy(postfile.title, posttitle, sizeof (postfile.title));

	getcross(filepath, filename, nboard, posttitle);

	setbdir(buf, nboard);
	if (append_record(buf, &postfile, sizeof (postfile)) == -1) {
		printf("post recored fail!\n");
		return 0;
	}
	printf("post sucessful\n");
	return 1;
}

int
cmpbnames(bname, brec)
char *bname;
struct boardheader *brec;
{
	if (!strncmp(bname, brec->filename, sizeof (brec->filename)))
		return 1;
	else
		return 0;
}

int
postfile(filename, owner, nboard, posttitle)
char *filename, *owner, *nboard, *posttitle;
{
	struct boardheader fh;

	if (search_record
	    (MY_BBS_HOME "/" BOARDS, &fh, sizeof (fh), cmpbnames, nboard) <= 0) {
		printf("%s �������Ҳ���", nboard);
		return -1;
	}
	post_cross(filename, nboard, posttitle, owner);
	return 0;
}

int
securityreport(owner, str, title)
char *owner;
char *str;
char *title;
{
	FILE *se;
	char fname[STRLEN];

	sprintf(fname, "tmp/security.%s", owner);
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "ϵͳ��ȫ��¼ϵͳ\n\033[1mԭ��%s\033[m\n", str);
		fclose(se);
		postfile(fname, owner, "syssecurity", title);
		unlink(fname);
	}
	return 0;
}

int
deliverreport(board, title, str)
char *board;
char *title;
char *str;
{
	FILE *se;
	char fname[STRLEN];

	sprintf(fname, "tmp/deliver.%s.%ld", board, time(NULL));
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "%s", str);
		fclose(se);
		postfile(fname, "deliver", board, title);
		unlink(fname);
	}
	return 0;
}
