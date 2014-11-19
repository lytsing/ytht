#include "bbslib.h"

static char *(replacestr[][2]) = { {
//"pku", "bbsboa?secstr=1"}, {
//"wenxue", "bbsboa?secstr=Y"}, {
NULL, NULL}};

char *
bbsred(char *command)
{
	static char buf[256];
	char path[128];
	struct userec *x;
	char *b;
	int clen;
	int i;
	b = getparm("b");
	if (b[0]) {
		char *ptr;
		strsncpy(buf, getsenv("QUERY_STRING"), sizeof (buf));
		ptr = strstr(buf, "b=");
		if (!ptr)
			return b;
		else
			return ptr + 2;
	}
	clen = strlen(command);
	if (!clen || isaword(specname, command)) {
		/*struct brcinfo *brcinfo = brc_readinfo(currentuser->userid);
		   sprintf(buf, "bbsboa?secstr=%c", brcinfo->lastsec[0]);
		   return buf; */
		return "bbsboa?secstr=?";
	}
	for (i = 0; replacestr[i][0]; i++) {
		if (!strcasecmp(command, replacestr[i][0]))
			return replacestr[i][1];
	}
	if (clen <= IDLEN) {
		if (getuser(command, &x) >= 1) {
			snprintf(path, 128,
				 "/groups/GROUP_0/Personal_Corpus/%c/%s",
				 mytoupper(x->userid[0]), x->userid);
			snprintf(buf, 256, MY_BBS_HOME "/0Announce%s", path);
			if (file_exist(buf)) {
				snprintf(buf, 256, "bbs0an?path=%s", path);
				return buf;
			}
		}
	}
	if (getboard2(command))
		snprintf(buf, 256, "bbshome?B=%d", getbnum(command));
	else {
		strcpy(buf, "bbsboa?secstr=?");
	}
	return buf;
}
