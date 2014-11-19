/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu
    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
                        Peng Piaw Foong, ppfoong@csie.ncu.edu.tw
    
    Copyright (C) 1999, KCN,Zhou Lin, kcn@cic.tsinghua.edu.cn

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "bbs.h"
#include "bbstelnet.h"

typedef struct {
	char *match;
	char *replace;
} tag_logout;

int
countlogouts(filename)
char filename[STRLEN];
{
	FILE *fp;
	char buf[256];
	int count = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return 0;

	while (fgets(buf, 255, fp) != NULL) {
		if (strstr(buf, "@logout@") || strstr(buf, "@login@"))
			count++;
	}
	fclose(fp);
	return count + 1;
}

void
user_display(filename, number, mode)
char *filename;
int number, mode;
{
	FILE *fp;
	char buf[256];
	int count = 1;

	clear();
	move(1, 0);
	if ((fp = fopen(filename, "r")) == NULL)
		return;
	while (fgets(buf, 255, fp) != NULL) {
		if (strstr(buf, "@logout@") || strstr(buf, "@login@")) {
			count++;
			continue;
		}
		if (count == number) {
			if (mode == YEA)
				showstuff(buf);
			else {
				prints("%s", buf);
			}
		} else if (count > number)
			break;
		else
			continue;
	}
	fclose(fp);
	return;
}

void
showstuff(buf)
char buf[256];
{
	extern time_t login_start_time;
	int frg, i, matchfrg, strlength, cnt, tmpnum;
	static char userid[IDLEN + 2], username[NAMELEN], numlogins[10],
	    numposts[10], rgtday[35], lasttime[35], thistime[35],
	    lastlogout[35], stay[10], alltime[20], ccperf[20], perf[10],
	    exp[10], ccexp[20], lasthost[16], ip[16];
	char buf2[STRLEN], *ptr, *ptr2;
	struct in_addr in;
	time_t now;

	static const tag_logout loglst[] = {
		{"userid", userid},
		{"username", username},
		{"lasthost", lasthost},
		{"ip", ip},
		{"rgtday", rgtday},
		{"log", numlogins},
		{"pst", numposts},
		{"lastlogin", lasttime},
		{"lastlogout", lastlogout},
		{"now", thistime},
		{"bbsname", MY_BBS_NAME},
		{"stay", stay},
		{"alltime", alltime},
		{"exp", exp},
		{"cexp", ccexp},
		{"perf", perf},
		{"cperf", ccperf},
		{NULL, NULL}
	};
	if (!strchr(buf, '$') || !currentuser) {
		prints("%s", buf);
		return;
	}
	now = time(0);
	strcpy(userid, currentuser->userid);
	strcpy(username, currentuser->username);
	tmpnum = countexp(currentuser);
	sprintf(exp, "%d", tmpnum);
	strcpy(ccexp, cuserexp(tmpnum));
	tmpnum = countperf(currentuser);
	sprintf(perf, "%d", tmpnum);
	strcpy(ccperf, cperf(tmpnum));
	sprintf(alltime, "%ldСʱ%ld����",
		(long int) (currentuser->stay / 3600),
		(long int) ((currentuser->stay / 60) % 60));
	sprintf(rgtday, "%24.24s", ctime(&(currentuser->firstlogin)));
	sprintf(lasttime, "%24.24s", ctime(&(currentuser->lastlogin)));
	sprintf(lastlogout, "%24.24s", ctime(&(currentuser->lastlogout)));
	sprintf(thistime, "%24.24s", ctime(&now));
	sprintf(stay, "%ld", (long int) ((time(0) - login_start_time) / 60));
	sprintf(numlogins, "%d", currentuser->numlogins);
	sprintf(numposts, "%d", currentuser->numposts);
	in.s_addr = currentuser->lasthost;
	sprintf(lasthost, "%s", inet_ntoa(in));
	in.s_addr = currentuser->ip;
	sprintf(ip, "%s", (currentuser->ip == 0) ? "" : inet_ntoa(in));
	frg = 1;
	ptr2 = buf;
	do {
		if ((ptr = strchr(ptr2, '$'))) {
			matchfrg = 0;
			*ptr = '\0';
			prints("%s", ptr2);
			ptr += 1;
			for (i = 0; loglst[i].match != NULL; i++) {
				if (strstr(ptr, loglst[i].match) == ptr) {
					strlength = strlen(loglst[i].match);
					ptr2 = ptr + strlength;
					for (cnt = 0; *(ptr2 + cnt) == ' ';
					     cnt++) ;
					sprintf(buf2, "%-*.*s",
						cnt ? strlength +
						cnt : strlength + 1,
						strlength + cnt,
						loglst[i].replace);
					prints("%s", buf2);
					ptr2 += (cnt ? (cnt - 1) : cnt);
					matchfrg = 1;
					break;
				}
			}
			if (!matchfrg) {
				prints("$");
				ptr2 = ptr;
			}
		} else {
			prints("%s", ptr2);
			frg = 0;
		}
	}
	while (frg);
	return;
}
