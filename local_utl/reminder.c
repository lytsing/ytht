//by ecnegrevid 2001.9.29
//提醒信息后面可以附加内容, 附加的内容写到etc/reminder.extra中
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
		")的用户您好，您在本站注册的帐号 %s 现在\n"
		"生命力已经降低到 10 以下，需要您用该帐号登陆一次才能"
		"使生命力恢复。\n如果该帐户并不是您注册的，忽略这封信就可以了。\n\n"
		"    登录本站的方法，可以通过我们的 WWW 页面，\n"
		"         http://" MY_BBS_DOMAIN "\n"
		"也可以使用 telnet 或者 ssh 方式登录本站，登录时使用"
		"同样\n的域名，" MY_BBS_DOMAIN "\n\n关于生命力的说明:\n"
		"    在 BBS 系统上，每个帐号都有一个生命力，在用户不\n"
		"登录的情况下，生命力每天减少 1，等生命力减少到 0 的时\n"
		"候，帐号就会自动消失。帐号每次登录后生命力就恢复到\n"
		"一个正常值，对于通过注册而且已经登录 4 次的用户，这个\n"
		"固定值至少是 120；对于通过注册但登录少于 4 次的用户，\n"
		"这个固定值是 30；对于未通过注册的用户，这个固定值是 15。\n\n",
		urec->userid);
	if (mf.size)
		fwrite(mf.ptr, 1, mf.size, fp);
	fputs("\n", fp);
	fclose(fp);

	bbs_sendmail(tmpfn, "系统提醒（" MY_BBS_NAME "）", udata.email,
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
