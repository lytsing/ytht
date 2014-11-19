#include "ythtbbs.h"

static const char *const mailsize_nolimit[] = {
	"SYSOP",
	"delete",
	"arbitration",
	NULL
};

/* 传入的fh里面包含大小
 * 判断 userid 的信箱加上大小后是否超容
 * 超过返回 -1
 * 否则返回 0，并且给相应的用户加上大小
 */
int
update_mailsize_up(struct fileheader *fh, char *userid)
{
	struct userec *lookupuser;
	struct userec tmpu;
	int i, maxsize, size, uid;
	int currsize;
	int sysmail;
	uid = getuser(userid, &lookupuser);
	if (uid <= 0)
		return -1;
	for (i = 0; mailsize_nolimit[i] != NULL; i++) {
		if (!strcmp(lookupuser->userid, mailsize_nolimit[i]))
			return 0;
	}
	currsize = get_mailsize(lookupuser);
	sysmail = fh->accessed & FH_SYSMAIL;
	if (currsize == -1 && !sysmail)
		return -1;
	maxsize = max_mailsize(lookupuser);
	size = bytenum(fh->sizebyte);
	size = (size / 1024);	// kilobyte
	if (size == 0)
		size = 1;
	memcpy(&tmpu, lookupuser, sizeof (struct userec));
	if (currsize == -1)
		tmpu.mailsize = -1;
	else if (size + currsize > SHRT_MAX) {
		if (!sysmail)
			return -1;
		tmpu.mailsize = -1;
	} else {
		if (size + currsize > maxsize) {
			if (!sysmail)
				return -1;
		}
		tmpu.mailsize += size;
	}
	updateuserec(&tmpu, uid);
	return 0;
}

int
update_mailsize_down(struct fileheader *fh, char *userid)
{
	struct userec *lookupuser;
	struct userec tmpu;
	int i, size, uid;
	int currsize;
	uid = getuser(userid, &lookupuser);
	if (uid <= 0)
		return -1;
	for (i = 0; mailsize_nolimit[i] != NULL; i++) {
		if (!strcmp(lookupuser->userid, mailsize_nolimit[i]))
			return 0;
	}
	currsize = get_mailsize(lookupuser);
	if (currsize == -1)
		currsize = calc_mailsize(lookupuser, 0);
	size = bytenum(fh->sizebyte);
	size = size / 1024;	// kilobyte
	if (size == 0)
		size = 1;
	memcpy(&tmpu, lookupuser, sizeof (struct userec));
	if (currsize < size)
		tmpu.mailsize = 0;
	else {
		if (currsize - size > SHRT_MAX)
			tmpu.mailsize = -1;
		else
			tmpu.mailsize -= size;
	}
	updateuserec(&tmpu, uid);
	return 0;
}

int
max_mailsize(struct userec *lookupuser)
{
	int maxsize;
	maxsize = (USERPERM(lookupuser, PERM_SYSOP)
		   || USERPERM(lookupuser, PERM_SPECIAL1)) ?
	    MAX_SYSOPMAIL_HOLD : (USERPERM(lookupuser, PERM_ARBITRATE)
				  || USERPERM(lookupuser, PERM_BOARDS)) ?
	    MAX_MAIL_HOLD * 2 : MAX_MAIL_HOLD;
	maxsize = maxsize * 10;
	return maxsize;
}

int
DIR_do_editmail(struct fileheader *fileinfo, struct fileheader *newfileinfo,
		char *userid)
{
	int uid;
	struct userec *lookupuser;
	struct userec tmpu;
	int size;
	int currsize;
	uid = getuser(userid, &lookupuser);
	size =
	    (bytenum(newfileinfo->sizebyte) -
	     bytenum(fileinfo->sizebyte)) / 1024;
	fileinfo->sizebyte = newfileinfo->sizebyte;
	fileinfo->edittime = newfileinfo->edittime;
	if (uid > 0) {
		currsize = get_mailsize(lookupuser);
		memcpy(&tmpu, lookupuser, sizeof (struct userec));
		if (currsize + size > SHRT_MAX)
			tmpu.mailsize = SHRT_MAX;
		else if (currsize + size < 0)
			tmpu.mailsize = 0;
		else
			tmpu.mailsize += size;
		updateuserec(&tmpu, uid);
	}
	return 0;
}

/* in kilobyte */
int
get_mailsize(struct userec *lookupuser)
{
	if (lookupuser->mailsize == -2)
		calc_mailsize(lookupuser, 1);
	return lookupuser->mailsize;
}

static inline int
_mail_buf(char *buf, int size, char *userid, char *title, char *sender,
	  int sysmail)
{
	struct fileheader newmessage;
	struct stat st;
	char fname[STRLEN], filepath[STRLEN], maildir[STRLEN];
	int now;
	FILE *fp;
	struct userec *lookupuser;

	if (getuser(userid, &lookupuser) <= 0)
		return -1;

	memset(&newmessage, 0, sizeof (newmessage));
	fh_setowner(&newmessage, sender, 0);
	strsncpy(newmessage.title, title, sizeof (newmessage.title));

	setmailfile(filepath, lookupuser->userid, "");
	if (stat(filepath, &st) == -1) {
		if (mkdir(filepath, 0775) == -1)
			return -1;
	} else {
		if (!(st.st_mode & S_IFDIR))
			return -1;
	}
	if (-2 == get_mailsize(lookupuser)) {
		errlog("strange user? %s", lookupuser->userid);
		return -1;
	}
	newmessage.sizebyte = numbyte(size);
	if (sysmail)
		newmessage.accessed |= FH_SYSMAIL;
	else
		newmessage.accessed &= ~FH_SYSMAIL;
	setmailfile(maildir, lookupuser->userid, ".DIR");
	now =
	    append_dir_callback(maildir, &newmessage, 0,
				(void *) update_mailsize_up, userid);
	if (now < 0)
		return -1;
	sprintf(fname, "M.%d.A", now);
	setmailfile(filepath, lookupuser->userid, fname);
	fp = fopen(filepath, "w");
	if (!fp) {
		return -1;
	}
	fprintf(fp, "寄信人: %s\n", sender);
	fprintf(fp, "标  题: %s\n", title);
	fprintf(fp, "发信站: %s (%s)\n\n", MY_BBS_NAME, Ctime(now));
	fwrite(buf, size, 1, fp);
	fclose(fp);
//      tracelog("%s mail %s", sender, lookupuser->userid);
	return now;
}

static inline int
_mail_file(char *filename, char *userid, char *title, char *sender, int sysmail,
	   int link)
{
      struct mmapfile mf = { ptr:NULL };
	int ret = -1;
	struct userec *lookupuser;
	struct fileheader newmessage;
	struct stat st;
	char fname[STRLEN], filepath[STRLEN], maildir[STRLEN], linkpath[STRLEN];
	int now;
	if (link) {
		if (getuser(userid, &lookupuser) <= 0)
			return -1;

		memset(&newmessage, 0, sizeof (newmessage));
		fh_setowner(&newmessage, sender, 0);
		strsncpy(newmessage.title, title, sizeof (newmessage.title));

		setmailfile(filepath, lookupuser->userid, "");
		if (stat(filepath, &st) == -1) {
			if (mkdir(filepath, 0775) == -1)
				return -1;
		} else {
			if (!(st.st_mode & S_IFDIR))
				return -1;
		}
		if (-2 == get_mailsize(lookupuser)) {
			errlog("strange user? %s", lookupuser->userid);
			return -1;
		}
		newmessage.sizebyte = numbyte(1024);
		if (sysmail)
			newmessage.accessed |= FH_SYSMAIL;
		else
			newmessage.accessed &= ~FH_SYSMAIL;
		setmailfile(maildir, lookupuser->userid, ".DIR");
		now =
		    append_dir_callback(maildir, &newmessage, 0,
					(void *) update_mailsize_up, userid);
		if (now < 0)
			return -1;
		sprintf(fname, "M.%d.A", now);
		setmailfile(filepath, lookupuser->userid, fname);
		sprintf(fname, "M.%d.A.link", now);
		setmailfile(linkpath, lookupuser->userid, fname);
		if (symlink(filename, linkpath) < 0)
			return -1;
		if (rename(linkpath, filepath) < 0) {
			unlink(linkpath);
			return -1;
		}
		return now;
	}
	MMAP_TRY {
		if (mmapfile(filename, &mf) < 0)
			MMAP_RETURN(-1);
		ret =
		    _mail_buf(mf.ptr, mf.size, userid, title, sender, sysmail);
	}
	MMAP_CATCH {
		ret = -1;
	}
	MMAP_END mmapfile(NULL, &mf);
	return ret;
}

int
mail_buf(char *buf, int size, char *userid, char *title, char *sender)
{
	int sysmail = 0;
	if (!strcmp(sender, "SYSOP") || !strcmp(sender, "deliver"))
		sysmail = 1;
	return _mail_buf(buf, size, userid, title, sender, sysmail);
}

int
mail_file(char *filename, char *userid, char *title, char *sender)
{
	int sysmail = 0;
	if (!strcmp(sender, "SYSOP") || !strcmp(sender, "deliver"))
		sysmail = 1;
	return _mail_file(filename, userid, title, sender, sysmail, 0);
}

int
system_mail_buf(char *buf, int size, char *userid, char *title, char *sender)
{
	return _mail_buf(buf, size, userid, title, sender, 1);
}

int
system_mail_file(char *filename, char *userid, char *title, char *sender)
{
	return _mail_file(filename, userid, title, sender, 1, 0);
}

int
system_mail_link(char *filename, char *userid, char *title, char *sender)
{
	return _mail_file(filename, userid, title, sender, 1, 1);
}

int
calc_mailsize(struct userec *lookupuser, int needlock)
{
	struct userec tmpu;
	int currmailsize = 0, size, filekilo;
	char maildir[256];
	struct fileheader tmpfh;
	char tmpmail[256];
	struct stat st;
	int fd;
	setmailfile(maildir, lookupuser->userid, "");
	if (stat(maildir, &st) == -1) {
		if (mkdir(maildir, 0775) == -1)
			return 0;
	} else {
		if (!(st.st_mode & S_IFDIR))
			return 0;
	}
	sprintf(maildir, "mail/%c/%s/.DIR", mytoupper(lookupuser->userid[0]),
		lookupuser->userid);
	fd = open(maildir, O_RDWR | O_CREAT, 0600);
	if (fd < 0)
		return 0;
	if (needlock)
		flock(fd, LOCK_EX);
	while (read(fd, &tmpfh, sizeof (tmpfh)) == sizeof (tmpfh)) {
		if (!tmpfh.sizebyte) {
			setmailfile(tmpmail, lookupuser->userid,
				    fh2fname(&tmpfh));
			size = file_size(tmpmail);
			tmpfh.sizebyte = numbyte(size);
			lseek(fd, -sizeof (struct fileheader), SEEK_CUR);
			write(fd, &tmpfh, sizeof (struct fileheader));
		}
		filekilo = bytenum(tmpfh.sizebyte) / 1024;
		if (filekilo == 0)
			filekilo = 1;
		currmailsize += filekilo;
	}
	memcpy(&tmpu, lookupuser, sizeof (struct userec));
	if (currmailsize > SHRT_MAX || currmailsize < 0)
		tmpu.mailsize = -1;
	else
		tmpu.mailsize = currmailsize;
	updateuserec(&tmpu, 0);
	if (needlock)
		flock(fd, LOCK_UN);
	close(fd);
	return currmailsize;
}

char *
check_mailperm(struct userec *lookupuser)
{
	static char buf[256];
	int maxsize, currsize;
	if (NULL == lookupuser)
		return "用户不存在\n";
	if (USERPERM(lookupuser, PERM_DENYMAIL))
		return "您已经被封禁了发信权\n";
	maxsize = max_mailsize(lookupuser);
	currsize = get_mailsize(lookupuser);
	if (currsize < 0 || currsize > maxsize) {
		snprintf(buf, sizeof (buf),
			 "您的邮箱已经超容了, 请控制邮件容量在 %d k 以下\n",
			 maxsize);
		return buf;
	}
	return NULL;
}

int
invalidaddr(const char *addr)
{
	if (*addr == '\0' || !strchr(addr, '@'))
		return 1;
	while (*addr) {
		if (!isalnum(*addr) && !strchr(".!@:-_", *addr))
			return 1;
		addr++;
	}
	return 0;
}

#ifdef INTERNET_EMAIL

int
bbs_sendmail(const char *fname, const char *title, const char *receiver,
	     const char *sender, int filter_ansi)
{
	FILE *fin, *fout;
	char *attach;
	int len;
	char b64_in[200];
	char tail_buf[3];
	int tail_len;
	char *buf;
	char sbuf[100];
	char *mime_type;
	const unsigned char *qpstr;
	int i;
	int outlen;

	if (strlen(receiver) > 100)
		return -1;
	if (invalidaddr(receiver))
		return -1;
	sprintf(sbuf, "/usr/lib/sendmail -f %s.bbs@%s '%s' 1>> %s/sendmail.err 2>> %s/sendmail.err",
		sender, MY_BBS_DOMAIN, receiver, MY_BBS_HOME, MY_BBS_HOME);
	fout = popen(sbuf, "w");
	fin = fopen(fname, "r");
	if (fin == NULL || fout == NULL) {
		if (fout)
			pclose(fout);
		if (fin)
			fclose(fin);
		return -1;
	}

	fprintf(fout, "Return-Path: %s.bbs@%s\n", sender, MY_BBS_DOMAIN);
	fprintf(fout, "Reply-To: %s.bbs@%s\n", sender, MY_BBS_DOMAIN);
	fprintf(fout, "From: %s <%s.bbs@%s>\n", sender, sender, MY_BBS_DOMAIN);
	fprintf(fout, "To: %s\n", receiver);
	fprintf(fout, "Subject: =?GB2312?Q?");
	qpstr = title;
	for (i = 0; qpstr[i]; i++) {
		fprintf(fout, "=%02X", qpstr[i]);
	}
	fprintf(fout, "?=\n");
	fprintf(fout, "X-Forwarded-By: %s\n", sender);

	if(!strcasecmp(sender, "SYSOP"))
		fprintf(fout, "X-Disclaimer: %s system messenger\n", MY_BBS_ID);
	else
		fprintf(fout, "X-Disclaimer: %s user message\n", MY_BBS_ID);
	fprintf(fout, "MIME-Version: 1.0\n");
	fprintf(fout, "Content-Type: multipart/mixed;\n");
	fprintf(fout, "              boundary=" MY_MIME_BOUNDARY "\n");
	fprintf(fout, "Content-Transfer-Encoding: 7bit\n\n");
	fprintf(fout, "This is a multi-part message in MIME format.\n");

	fprintf(fout, "--" MY_MIME_BOUNDARY "\n");
	fprintf(fout, "Content-Type: text/plain; charset=gb2312\n");
	fprintf(fout, "Content-Transfer-Encoding: Base64\n\n");

	f_b64_ntop_init(tail_buf, &tail_len, &outlen);
	while (fgets(b64_in, sizeof (b64_in), fin) != NULL) {
		if (NULL != (attach = checkbinaryattach(b64_in, fin, &len))) {
			if (len <= 0)
				continue;
			buf = malloc(len);
			if (buf == NULL)
				continue;
			fread(buf, len, 1, fin);
			f_b64_ntop_fini(fout, tail_buf, &tail_len);
			fprintf(fout, "\n--" MY_MIME_BOUNDARY "\n");
			mime_type = get_mime_type(attach);
			fprintf(fout, "Content-Type: %s; name=%s\n", mime_type,
				attach);
			fprintf(fout, "Content-Transfer-Encoding: Base64\n\n");
			f_b64_ntop_init(tail_buf, &tail_len, &outlen);
			f_b64_ntop(fout, buf, len, tail_buf, &tail_len,
				   &outlen);
			f_b64_ntop_fini(fout, tail_buf, &tail_len);
			fprintf(fout, "\n--" MY_MIME_BOUNDARY "\n");
			fprintf(fout,
				"Content-Type: text/plain; charset=gb2312\n");
			fprintf(fout, "Content-Transfer-Encoding: Base64\n\n");
			f_b64_ntop_init(tail_buf, &tail_len, &outlen);
			free(buf);
			continue;
		}
		if (filter_ansi)
			filteransi(b64_in);
		f_b64_ntop(fout, b64_in, strlen(b64_in), tail_buf, &tail_len,
			   &outlen);
	}
	f_b64_ntop_fini(fout, tail_buf, &tail_len);
	fprintf(fout, "\n--" MY_MIME_BOUNDARY "--\n");
	fprintf(fout, ".\n");

	fclose(fin);
	pclose(fout);
	return 0;
}

int
bbs_sendmail_noansi(fname, title, receiver, sender)
char *fname, *title, *receiver, *sender;
{
	return bbs_sendmail(fname, title, receiver, sender, 1);
}
#endif
