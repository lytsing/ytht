//by ecnegrevid 2001.9.29
//������Ϣ������Ը�������, ���ӵ�����д��etc/reminder.extra��
#include "../include/bbs.h"
#include "ythtlib.h"
#include "ythtbbs.h"

int
sendreminder(struct userec *urec)
{
	char *ptr;
	time_t t;
	FILE *fp;
	struct userdata udata;
      static struct mmapfile mf = { ptr:NULL };
	char tmpfn[256];

	if (!mf.ptr)
		mmapfile(MY_BBS_HOME "/etc/reminder.extra", &mf);

	loaduserdata(urec->userid, &udata);
	if (strchr(udata.email, '@') == NULL)
		return 0;
	ptr = udata.email;
	while (*ptr) {
		if (!isalnum(*ptr) && !strchr(".@", *ptr))
			return 0;
		ptr++;
	}
	if (strcasestr(udata.email, ".bbs@" MY_BBS_DOMAIN) != NULL)
		return 0;

	sprintf(tmpfn, MY_BBS_HOME "/bbstmpfs/tmp/reminder.%d", getpid());
	fp = fopen(tmpfn, "w");
	if (fp == NULL)
		return 0;
	fprintf(fp,
		"    " MY_BBS_NAME "(" MY_BBS_DOMAIN
		")���û����ã����ڱ�վע����ʺ� %s ����\n"
		"�������Ѿ����͵� 10 ���£���Ҫ���ø��ʺŵ�½һ�β���"
		"ʹ�������ָ���\n������ʻ���������ע��ģ���������žͿ����ˡ�\n\n"
		"    ��¼��վ�ķ���������ͨ�����ǵ� WWW ҳ�棬\n"
		"         http://" MY_BBS_DOMAIN "\n"
		"Ҳ����ʹ�� telnet ���� ssh ��ʽ��¼��վ����¼ʱʹ��"
		"ͬ��\n��������" MY_BBS_DOMAIN "\n\n������������˵��:\n"
		"    �� BBS ϵͳ�ϣ�ÿ���ʺŶ���һ�������������û���\n"
		"��¼������£�������ÿ����� 1�������������ٵ� 0 ��ʱ\n"
		"���ʺžͻ��Զ���ʧ���ʺ�ÿ�ε�¼���������ͻָ���\n"
		"һ������ֵ������ͨ��ע������Ѿ���¼ 4 �ε��û������\n"
		"�̶�ֵ������ 120������ͨ��ע�ᵫ��¼���� 4 �ε��û���\n"
		"����̶�ֵ�� 30������δͨ��ע����û�������̶�ֵ�� 15��\n\n",
		urec->userid);
	if (mf.size)
		fwrite(mf.ptr, 1, mf.size, fp);
	fputs("\n", fp);
	fclose(fp);

	bbs_sendmail(tmpfn, "ϵͳ���ѣ�" MY_BBS_NAME "��", udata.email,
		     "SYSOP", 0);
	unlink(tmpfn);

	if ((fp = fopen(MY_BBS_HOME "/reminder.log", "a")) != NULL) {
		t = time(NULL);
		ptr = ctime(&t);
		ptr[strlen(ptr) - 1] = 0;
		fprintf(fp, "%s %s %s\n", ptr, urec->userid, udata.email);
		fclose(fp);
	}
	return 0;
}

int
send_emailcheck_again(struct userec *rec, struct userdata *udata)
{
	char mcfn[256];
	char randstr[256];
	char email[256];
	sethomefile(mcfn, rec->userid, "mailcheck");
	if (!file_exist(mcfn))
		return -1;
	if (readstrvalue(mcfn, "email", email, sizeof (email)) >= 0) {
		if (strcasecmp(email, udata->email)) {
			savestrvalue(mcfn, "email", udata->email);
		}
	}
	if (!trustEmail(udata->email))
		return -1;
	if (readstrvalue(mcfn, "randstr", randstr, sizeof (randstr)) < 0)
		return -1;
	do_send_emailcheck(rec, udata, randstr);
	return 0;
}

int
main(int argc, char *argv[])
{
	int fd1;
	struct userec rec;
	struct userdata udata;
	int size1 = sizeof (rec);
	chdir(MY_BBS_HOME);

	if ((fd1 = open(PASSFILE, O_RDONLY)) == -1) {
		perror("open PASSWDFILE");
		return -1;
	}

	while (read(fd1, &rec, size1) == size1) {
		if (!rec.userid[0] || rec.kickout)
			continue;
		if (countlife(&rec) != 10)
			continue;
		printf("%s\n", rec.userid);
		sleep(1);
#ifdef ENABLE_EMAILREG
		if ((rec.userlevel & PERM_DEFAULT) != PERM_DEFAULT) {
			if (loaduserdata(rec.userid, &udata) < 0)
				continue;
			send_emailcheck_again(&rec, &udata);
		} else {
			sendreminder(&rec);
		}
#else
		sendreminder(&rec);
#endif
	}
	close(fd1);
	return 0;
}
