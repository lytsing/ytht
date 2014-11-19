#include <sys/file.h>
#include "ythtbbs.h"
#include "checkreg.c"

int
getregforms(char *filename, int num, const char *userid)
{
	int lockfd, t, numline = 0, count = 0;
	FILE *fpr, *fpw;
	char buf[256];
	lockfd =
	    openlockfile(MY_BBS_HOME "/.lock_new_register", O_RDONLY, LOCK_EX);
	if (lockfd <= 0)
		return -1;
	mkdir(SCANREGDIR, 0770);
	if (file_exist(GETREGFILE))
		goto ERROR1;
	rename(NEWREGFILE, GETREGFILE);
	fpr = fopen(GETREGFILE, "r");
	if (fpr == NULL)
		goto ERROR1;
	strcpy(filename, SCANREGDIR);
	sprintf(buf, "R.%%d.%s", userid);
	t = trycreatefile(filename, buf, time(NULL), 5);
	if (t < 0)
		goto ERROR2;
	fpw = fopen(filename, "w");
	if (fpw == NULL)
		goto ERROR2;
	while (fgets(buf, sizeof (buf), fpr)) {
		if (buf[0] == '-') {
			if (!numline)
				continue;
			fputs(buf, fpw);
			count++;
			if (count >= num)
				break;
			numline = 0;
			continue;
		}
		numline++;
		fputs(buf, fpw);
	}
	fclose(fpw);
	fpw = fopen(NEWREGFILE, "a");
	if (fpw == NULL)
		goto ERROR2;
	while (fgets(buf, sizeof (buf), fpr)) {
		fputs(buf, fpw);
	}
	fputs("----\n", fpw);
	fclose(fpw);
	fclose(fpr);
	unlink(GETREGFILE);
	close(lockfd);
	return count;
      ERROR2:
	fclose(fpr);
      ERROR1:
	close(lockfd);
	return -1;
}

int
usedEmail(char *email)
{
	FILE *fp;
	char buf[128], *ptr;
	int retv = 0;
	fp = fopen(USEDEMAIL, "r");
	if (!fp)
		return retv;
	while (fgets(buf, sizeof (buf), fp)) {
		ptr = strtok(buf, " \r\n\t");
		if (!ptr)
			continue;
		if (!strcasecmp(ptr, email)) {
			retv = 1;
			break;
		}
	}
	fclose(fp);
	return retv;
}

int
trustEmail(char *email)
{
	int retv = 0;
	FILE *fp;
	char buf[128], *ptr;
	if (!strchr(email, '@') || !strchr(email, '.'))
		return 0;
	if (strcasestr(email, ".bbs@"))
		return 0;
#if 0
	char *url;
	url = strchr(email, '@');
	if (!url)
		return 0;
	url++;
	fp = fopen(TRUSTEMAIL, "r");
	if (!fp)
		return 0;
	while (fgets(buf, sizeof (buf), fp)) {
		if ((ptr = strchr(buf, '\n')))
			*ptr = 0;
		if ((ptr = strchr(buf, '\r')))
			*ptr = 0;
		if (!strcasecmp(buf, url)) {
			retv = 1;
			break;
		}
	}
	fclose(fp);
	if (!retv)
		return retv;
#else
	retv = 1;
#endif
	fp = fopen(UNTRUSTEMAIL, "r");
	if (fp) {
		while (fgets(buf, sizeof (buf), fp)) {
			ptr = strtok(buf, " \r\n\t");
			if (!ptr)
				continue;
			if (strcasestr(email, buf)) {
				retv = 0;
				break;
			}
		}
		fclose(fp);
	}
	if (!retv)
		return 0;

	if (usedEmail(email))
		retv = 0;
	return retv;
}

int
do_send_emailcheck(const struct userec *tmpu, const struct userdata *udata,
		   const char *randstr)
{
	FILE *fin, *fout;
	char buf[256];
	if (NULL == (fin = fopen(EMAILCHECK, "r"))) {
		return -1;
	}
	sprintf(buf, "/usr/lib/sendmail -f SYSOP.bbs@%s %s &>/dev/null",
		MY_BBS_DOMAIN, udata->email);
	if (NULL == (fout = popen(buf, "w"))) {
		fclose(fin);
		return -1;
	}

	/* begin of sending a mail to user to check email-addr */
	fprintf(fout, "Reply-To: SYSOP.bbs@%s\n", MY_BBS_DOMAIN);
	fprintf(fout, "From: SYSOP.bbs@%s\n", MY_BBS_DOMAIN);
	fprintf(fout, "To: %s\n", udata->email);
	fprintf(fout, "Subject: @%s@[-%s-]%s mail check.\n", tmpu->userid,
		randstr, MY_BBS_ID);
	fprintf(fout, "X-Forwarded-By: SYSOP \n");
	fprintf(fout, "X-Disclaimer: %s registration mail.\n", MY_BBS_ID);
	fprintf(fout, "\n");
	fprintf(fout, "BBS LOCATION     : %s (%s)\n", MY_BBS_DOMAIN, MY_BBS_IP);
	fprintf(fout, "YOUR BBS USER ID : %s\n", tmpu->userid);
	fprintf(fout, "APPLICATION DATE : %s", ctime(&(tmpu->firstlogin)));
	//fprintf(fout, "LOGIN HOST       : %s\n", fromhost);
	fprintf(fout, "YOUR NICK NAME   : %s\n", tmpu->username);
	fprintf(fout, "YOUR NAME        : %s\n", udata->realname);
	while (fgets(buf, 255, fin) != NULL) {
		if (buf[0] == '.' && buf[1] == '\n')
			fputs(". \n", fout);
		else if (!strncmp(buf, "#showlink", 9)) {
			fprintf(fout, "        http://%s/%s/emailconfirm?"
				"userid=%s&randstr=%s \n", MY_BBS_DOMAIN,
				BASESMAGIC, tmpu->userid, randstr);
		} else
			fputs(buf, fout);
	}
	fprintf(fout, ".\n");
	pclose(fout);
	fclose(fin);
	return 0;
	/* end of sending a mail to user to check email-addr */
}

int
send_emailcheck(const struct userec *tmpu, const struct userdata *udata)
{
	char buf[256], randstr[30];
	int code;
	FILE *fp;

	code = (time(0) / 2) + (rand() / 10);
	sethomefile(buf, tmpu->userid, "mailcheck");
	if ((fp = fopen(buf, "w")) == NULL) {
		return -1;
	}
	fprintf(fp, "%9.9d\n", code);
	fprintf(fp, "randstr %9.9d\n", code);
	fprintf(fp, "firstlogin %ld\n", (long) tmpu->firstlogin);
	fprintf(fp, "realname %s\n", udata->realname);
	fprintf(fp, "address %s\n", udata->address);
	fprintf(fp, "email %s\n", udata->email);
	fprintf(fp, "lasthost %lu\n", tmpu->lasthost);
	fclose(fp);
	sprintf(randstr, "%9.9d", code);
	if (do_send_emailcheck(tmpu, udata, randstr) < 0) {
		unlink(buf);
		return -1;
	}
	return 0;
}

int
_recordInvitation(int t, int code, const char *inviter, const char *toemail,
		  const char *toname)
{
	char buf[256];
	FILE *fp;
	mkdir(INVITATIONDIR, 0770);
	sprintf(buf, INVITATIONDIR "/%s", inviter);
	mkdir(buf, 0770);
	sprintf(buf, INVITATIONDIR "/%s/%d.%d", inviter, t, code);
	fp = fopen(buf, "w");
	if (fp == NULL)
		return -1;
	fprintf(fp, "toemail %s\n", toemail);
	fprintf(fp, "toname %s\n", toname);
	fclose(fp);
	return 0;
}

int
sendInvitation(const char *inviter, const char *toemail, const char *toname,
	       const char *tmpfn)
{
	int code, t, retv;
	FILE *fout;
	char title[256];

	t = time(NULL);
	code = rand() / 10;
	if (_recordInvitation(t, code, inviter, toemail, toname) < 0)
		return -1;

	snprintf(title, sizeof (title),
		 "%s，%s 邀请您在%s BBS 建立用户\n", toname, inviter,
		 MY_BBS_ID);

	if (NULL == (fout = fopen(tmpfn, "a+"))) {
		return -3;
	}
	fprintf(fout,
		"\n------------------以上为 %s 给您的留言---------------------\n",
		inviter);
	fprintf(fout, "亲爱的%s，您好，\n\n", toname);
	fprintf(fout, "%s.bbs@%s 邀请您在 %s BBS 建立一个用户。"
		"这封邀请信在两周内有效。\n\n",
		inviter, MY_BBS_DOMAIN, MY_BBS_NAME);
	fprintf(fout, "要接受该邀请，您只要点击下面的连接，并选择自己的用户名"
		"和设置密码就可以了，\n");
	fprintf(fout, "        http://%s/%s/inviteconfirm?"
		"inviter=%s&t=%d&code=%d\n\n", MY_BBS_DOMAIN, BASESMAGIC,
		inviter, t, code);
	fprintf(fout,
		"建立用户之后，系统会自动将你们的用户名加到对方的好友名单。\n\n");
	fprintf(fout,
		"如果您想先了解一下%s BBS 的概况，可以访问一下我们的 WWW 页面，\n"
		"        http://%s/\n", MY_BBS_NAME, MY_BBS_DOMAIN);
	fprintf(fout, "或者用 telnet 访问，\n"
		"        telnet %s\n\n\n", MY_BBS_DOMAIN);
	fprintf(fout, "                              %s BBS\n", MY_BBS_NAME);
	fclose(fout);
	retv = bbs_sendmail(tmpfn, title, toemail, inviter, 0);
	if (retv < 0)
		return -4;
	return 0;
}
