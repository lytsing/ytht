//�Զ��������޷��/������ĳ���
//��������޿����ڴ���������޸İ������Եĵط��趨
//crontab ��д��
//2 2,8,14,20 * * * /home/bbs/bin/autoBanBoards 2 &>/dev/null
//5 0,1,3,4,5,6,7,9,10,11,12,13,15,16,17,18,19,21,22,23 * * *  /home/bbs/bin/autoBanBoards 1 &>/dev/null

#include "bbs.h"
int level = 0;			//0: �����н��; 1: �����н��;���; 2: ���/����/���

struct bbsinfo bbsinfo;

int
getlimit(struct boardheader *bh)
{
	if (!bh->limitchar)
		return 3000;
	return bh->limitchar * 100;
}

int
warnmsg(struct boardheader *bh, int action, int limit, int limit_new)
{
	char buf[512];
	int i, uid;
	struct msghead head;
	struct user_info *uinfo;
	int pos;
	head.time = time(0);
	head.sent = 0;
	head.mode = 0;
	strcpy(head.id, "ϵ ͳ");
	head.frompid = 0;
	head.topid = 0;
	snprintf(buf, sizeof (buf),
		 "�����ΰ����� %s �����������%s����, ���ް���ᱻ�Զ����, �뼴ʱ����\n"
		 "  3 ��ǰ������������ %d, 12 Сʱǰ������������ %d, \n"
		 "  ��������������Ҫ���� %d.\n", bh->filename,
		 (action == 1) ? "����" : "�Ѿ�", limit, limit_new,
		 limit_new + 500);
	for (i = 0; i < BMNUM; i++) {
		if (!bh->bm[i][0])
			continue;
		if (!strcasecmp(bh->bm[i], "SYSOP"))
			continue;
		uid = getuser(bh->bm[i], NULL);
		if (uid <= 0)
			continue;
		save_msgtext(bh->bm[i], &head, buf);
		if (!(uinfo = queryUIndex(uid, NULL, 0, &pos)))
			continue;
		if (uinfo->pid > 1)
			kill(uinfo->pid, SIGUSR2);
		else
			(uinfo->unreadmsg)++;
	}
	return 0;
}

int
checklimit(int fd, int count, int limit, int flex, time_t t)
{
	int retv = 0;
	struct fileheader fh;
	if (count < limit + 2 * flex / 3)
		return 0;

	lseek(fd, sizeof (fh) * limit, SEEK_SET);
	read(fd, &fh, sizeof (fh));
	if (fh.filetime <= t + 12 * 3600)
		retv = 1;
	if (fh.filetime <= t) {
		if (count < limit + flex)
			retv = 1;
		else
			retv = 2;
	}
	return retv;
}

int
shouldBan(struct boardheader *bh, int oldvalue)
{
	int limit, limit_new, count, fd = -1, ban = 0;
	char buf[256];
	time_t t = time(NULL);
	if (!oldvalue && !level)
		return oldvalue;
	limit = getlimit(bh);
	limit_new = limit + limit / 3;
	sprintf(buf, MY_BBS_HOME "/boards/%s/.DIR", bh->filename);
	count = file_size(buf) / sizeof (struct fileheader);

	if (count <= limit)
		return 0;
	if (count > limit_new + 500)
		goto SHOULDBAN;

	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return 0;
	ban = max(ban, checklimit(fd, count, limit, 500, t - 3 * 24 * 3600));
	ban = max(ban, checklimit(fd, count, limit_new, 500, t - 12 * 3600));
	close(fd);

	if (ban == 2)
		goto SHOULDBAN;
	if (ban == 1)
		goto WOULDBAN;
	return 0;
      WOULDBAN:
	if (oldvalue != 1) {
		if (!level)
			return 0;
		warnmsg(bh, 1, limit, limit_new);
	}
	return 1;
      SHOULDBAN:
	if (oldvalue != 2) {
		if (level < 2)
			goto WOULDBAN;
		warnmsg(bh, 2, limit, limit_new);
	}
	return 2;
}

int
genfiles()
{
	FILE *fp;
	if ((fp = fopen("bbstmpfs/dynamic/banAlert.www.new", "w")) == NULL)
		return -1;
	fprintf(fp, "<b>�������³��ޱ���</b><br>"
		"<font class=red>!!!</font> �������½�Ҫ����, ���潫�� 5 Сʱ�ڱ���ֹ����.<br>"
		"<font class=red><b>!!!</b></font> �������³���, �����Ѿ�����ֹ����.<br>"
		"<br><br>��ͨ����"
		"<li><u>3 ��ǰ����������</u>���� 3000, "
		"<li><u>5 Сʱǰ����������</u>���� 4000, "
		"<li><u>������������</u>���ó��� 4000 + 500."
		"<br>����������������涨��"
		"<li><u>3 ��ǰ����������</u>������Ϊ x,"
		"<li><u>5 Сʱǰ����������</u>���� x + x/3,"
		"<li><u>������������</u>���ó��� x + x/3 + 500.");
	fprintf(fp, "<br><br>�ϴμ��ʱ�䣺%s<br>", Ctime(time(NULL)));
	fclose(fp);
	rename("bbstmpfs/dynamic/banAlert.www.new",
	       "bbstmpfs/dynamic/banAlert.www");
	return 0;
}

//argv[1]: -1: �������з��/������; 0: ֻ���н��; 
//1: ���;���; 2: ���/���;���
int
main(int argn, char **argv)
{
	int i;
	int oldvalue;
	struct boardheader *bh;
	char *(valuestr[]) = { "���", "����", "���" };

	printf("�Զ����/������\n����ʱ��: %s\n", Ctime(time(NULL)));

	if (argn >= 2)
		level = atoi(argv[1]);
	if (level > 2 || level < -1)
		level = 0;
	if (initbbsinfo(&bbsinfo) < 0) {
		printf("Failed to attach shm.\n");
		return -1;
	}
	if (uhash_uptime() == 0) {
		printf("Failed to access uhash.\n");
		return -1;
	}

	chdir(MY_BBS_HOME);
	for (i = 0; i < MAXBOARD; i++) {
		bh = &bbsinfo.bcache[i].header;
		if (!bh->filename[0])
			continue;
		if (!strcasecmp(bh->filename, "syssecurity")) {
			bbsinfo.bcache[i].ban = 0;
			continue;
		}
		if (level == -1) {
			bbsinfo.bcache[i].ban = 0;
			continue;
		}
		oldvalue = bbsinfo.bcache[i].ban;
		bbsinfo.bcache[i].ban = shouldBan(bh, bbsinfo.bcache[i].ban);
		if (bbsinfo.bcache[i].ban || oldvalue)
			printf("%-20.20s %s ==> %s\n", bh->filename,
			       valuestr[oldvalue],
			       valuestr[(int) bbsinfo.bcache[i].ban]);
	}
	genfiles();
	return 0;
}
