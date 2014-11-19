#include "bbs.h"

#define DEFAULT_ADD 600
#define CONT MY_BBS_HOME "/help/watchmanhelp"

struct bbsinfo bbsinfo;

void
usage(char *name)
{
	printf("%s userid userpasswd [minutes_to_delay]\n", name);
}

int
main(int argc, char *argv[])
{
	struct UTMPFILE *shm_utmp;
	int time_add = 0;
	int i;
	FILE *fp, *fr;
	time_t now_t;
	char buf[256];
	if (argc < 3) {
		usage(argv[0]);
		return 0;
	}
	//errlog("argc:%d",argc);
	if (argc > 3){
		time_add = atoi(argv[3]) * 60;
	//	errlog("argv[3]: %s",argv[3]);
	}
	if (!time_add)
		time_add = DEFAULT_ADD;
	for (i = 0; i < 10; i++) {
		initbbsinfo(&bbsinfo);
		shm_utmp = bbsinfo.utmpshm;
		if (shm_utmp)
			break;
		sleep(10);
	}
	if (!shm_utmp) {
		errlog("can't attach ushm!");
		return -1;
	}
	now_t = time(NULL);
	if (shm_utmp->watchman) {
		strcpy(buf,
		       "�����ﾯ�������������԰�����ס�ˡ�����");
	} else {
		shm_utmp->watchman = now_t + time_add;
		getrandomint(&(shm_utmp->unlock));
		snprintf(buf, sizeof (buf), "������,��î��!������ %s",
			 Ctime(now_t));
	}

	fp = popen("/usr/bin/mail bbs", "w");
	if (!fp) {
		errlog("can't open mail!");
		return -2;
	}
	fr = fopen(CONT, "r");
	fprintf(fp, "#name: %s\n", argv[1]);
	fprintf(fp, "#pass: %s\n", argv[2]);
	fprintf(fp, "#board: deleterequest\n");
	fprintf(fp, "#title: %s\n", buf);
	fprintf(fp, "#localpost:\n\n");
	if (!strncmp(buf, "��", 2)) {
		fprintf(fp,
			"����%d�����ڽ��н�������,����BBS���к�������صİ��潫�����\n"
			"ֱ�����˽����˽�������.\n" "��������: %u\n",
			time_add / 60, shm_utmp->unlock % 10000);
	}
	fprintf(fp, "�����ǹ��ڽ���������˵��:\n");
	fr = fopen(CONT, "r");
	if (fr) {
		while (fgets(buf, sizeof (buf), fr))
			fputs(buf, fp);
		fclose(fr);
	}
	pclose(fp);
	return 0;
}
