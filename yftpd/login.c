#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include "dirlist.h"
#include "mystring.h"
#include "logging.h"
#include "../include/bbs.h"
#include "yftpdutmp.h"
#include "options.h"
#include "main.h"
#include "login.h"

struct userec currentuser;
int isanonymous = 0;
int checkuser(char *id, char *pw);

int
yftpd_login(char *password)
{
	struct stat buf;

	if (stat(PATH_DENY_LOGIN, &buf) == 0) {
		prints("421-Server disabled.\r\n");
		print_file(421, PATH_DENY_LOGIN);
		yftpd_log
		    ("Login as user '%s' failed: Server disabled.\n", user);
		exit(0);
	}
	if (!strcasecmp(user, "anonymous") || !strcasecmp(user, "guest")) {
		strcpy(user, "guest");
		isanonymous = 1;
	}
	if (!checkuser(user, password))
		return 1;
//	if (!(currentuser.userlevel & (PERM_SYSOP | PERM_BOARDS))) {
//		prints("421 You are not a board manager");
	if (!(currentuser.userlevel & PERM_LOGINOK)) {
		prints("421 You are not an authorized user");
		exit(1);
	}
	if (seek_in_file(PATH_ADMINISTRATOR, user))
		currentuser.userlevel |= PERM_SYSOP;
	switch (yftpdutmp_log(1)) {
	case -1:
		prints("421 Internel error, sorry...");
		exit(1);
	case -2:
		prints("421 Multilogin is not allowed.");
		exit(1);
	case -3:
		prints("421 MAXUSER %d reached. Couldn't login", MAX_FTPUSER);
		exit(1);
	default:
		break;
	}
	prints("230 User %s logged in.", currentuser.userid);
	yftpd_log("Successfully logged in as user '%s'.\n", user);
	state = STATE_AUTHENTICATED;
	return 0;
}

int
checkuser(char *id, char *pw)
{
	int currentuid;
	struct userec *urec;
	if ((currentuid = getuser(id, &urec)) <= 0)
		return 0;
	memcpy(&currentuser, urec, sizeof(struct userec));
	if (strcasecmp(id, currentuser.userid))
		return 0;
	if (userbansite(currentuser.userid, remotehostname))
		return 0;
	if (isanonymous)
		return 1;
	if (!USERPERM(&currentuser, PERM_BASIC))
		return 0;
	if (!checkpasswd(currentuser.passwd, currentuser.salt, pw)) {
		time_t t = time(NULL);
		logattempt(currentuser.userid, remotehostname, "FTP", t);
		return 0;
	}
	return 1;
}

int
hasreadperm(struct boardmem *bptr)
{
	char fn[256];
	if (bptr->header.level) {
		if (!USERPERM(&currentuser, bptr->header.level)
		    && !(bptr->header.level & PERM_NOZAP))
			return 0;
	}
	if (bptr->header.flag & CLOSECLUB_FLAG) {
		sprintf(fn, MY_BBS_HOME "/boards/%s/club_users",
			bptr->header.filename);
		if (seek_in_file(fn, currentuser.userid))
			return 1;
		else
			return 0;
	}

	return 1;
}

int
seek_in_file(const char *filename, const char *seekstr)
{
	FILE *fp;
	char buf[STRLEN];
	char *namep;
	if ((fp = fopen(filename, "r")) == NULL)
		return 0;
	while (fgets(buf, STRLEN, fp) != NULL) {
		namep = (char *) strtok(buf, ": \n\r\t");
		if (namep != NULL && strcasecmp(namep, seekstr) == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}
