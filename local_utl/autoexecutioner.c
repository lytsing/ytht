/*    自动杀档系统    churinga 2004.7.11 		  */

#include "bbs.h"
#include "sysrecord.h"

struct bbsinfo bbsinfo;

void
executeuser(uident)
char *uident;
{
	char msgbuf[256];
	char repbuf[256];
	int id;
	struct userec *lookupuser;
	if (!(id = getuser(uident, &lookupuser)))
		return;	// user not exist, so need not del
	if (lookupuser->userid[0] == '\0'
	    || !strcmp(lookupuser->userid, "SYSOP"))
		return;
	kickoutuserec(lookupuser->userid);
	
	sprintf(repbuf, "%s 被执行杀档", uident);
	sprintf(msgbuf, "执行原因：时辰已到!");
	securityreport("deliver", msgbuf, repbuf);
}

int
main()
{
	int d_fd;
	char denyfile[] = "etc/tobeexecuted";
	char tmpfile[] = "etc/tobeexecuted.tmp";
	char lockfile[] = "etc/tobeexecuted.lock";
	char seps[] = " ";
	char *token;
	char userid[16];
	unsigned long duetime;
	struct stat st;
	time_t nowtime;
	char linebuf[256], tmpbuf[256];
	FILE *fp, *fp2;

	nowtime = time(NULL);

	if (initbbsinfo(&bbsinfo) < 0)
		return -1;
	if ((stat(denyfile, &st) < 0) || (st.st_size == 0))
		return -1;
	if ((d_fd = open(lockfile, O_RDWR | O_CREAT, 0660)) == -1)
		return -1;
	flock(d_fd, LOCK_EX);
	fp = fopen(denyfile, "r");
	if (!fp) {
		flock(d_fd, LOCK_UN);
		close(d_fd);
		return -1;
	}
	fp2 = fopen(tmpfile, "w");
	if (!fp2) {
		flock(d_fd, LOCK_UN);
		fclose(fp);
		close(d_fd);
		return -1;
	}
	while (fgets(linebuf, 256, fp)) {
		strcpy(tmpbuf, linebuf);
		token = strtok(linebuf, seps);
		duetime = 0;
		if (token) {
			strncpy(userid, token, 12);
		} else {
			errlog("format err: %s '%s'", denyfile, tmpbuf);
			continue;
		}
		token = strchr(tmpbuf, 27 /* \033 */);
		if (token) {
			duetime = (unsigned long) atol(token + 2);
		} else {
			errlog("format err: no \\033, %s '%s'", denyfile, tmpbuf);
			continue;
		}
		if (duetime <= nowtime) {
			// execute it!
			executeuser(userid);
		} else {
			// write to file ag
			fputs(tmpbuf, fp2);
		}
	}
	fclose(fp2);
	fclose(fp);
	if (rename(tmpfile, denyfile) == -1)
		errlog("rename from %s to %s failed!", tmpfile, denyfile);
	flock(d_fd, LOCK_UN);
	close(d_fd);
	return 0;
}

