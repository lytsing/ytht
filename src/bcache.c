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

extern int die;
int numboards = -1;

static int fillbcache(struct boardheader *fptr, int *pcountboard);
static void bmonlinesync(void);

static int
fillbcache(fptr, pcountboard)
struct boardheader *fptr;
int *pcountboard;
{
	struct boardmem *bptr;

	if (*pcountboard >= MAXBOARD)
		return 0;
	bptr = &(bbsinfo.bcache[*pcountboard]);
	(*pcountboard)++;
	memcpy(&(bptr->header), fptr, sizeof (struct boardheader));
	getlastpost(bptr->header.filename, &bptr->lastpost, &bptr->total);
	return 0;
}

void
attach_err(shmkey, name)
int shmkey;
char *name;
{
	sprintf(genbuf, "Error! %s error! key = %x.\n", name, shmkey);
	prints("%s", genbuf);
	refresh();
	exit(1);
}

void *
attach_shm(shmkey, shmsize)
int shmkey, shmsize;
{
	void *shmptr;

	shmptr = get_shm(shmkey, shmsize);
	if (shmptr == NULL)
		attach_err(shmkey, "shmat");
	return shmptr;
}

void
reload_boards()
{
	struct stat st;
	time_t now2;
	int lockfd, countboard;

	numboards = bbsinfo.bcacheshm->number;

	lockfd = open("bcache.lock", O_RDONLY | O_CREAT, 0600);
	if (lockfd < 0)
		return;
	flock(lockfd, LOCK_EX);

	now_t = time(NULL);
	if (bbsinfo.bcacheshm->uptime > now_t) {
		bbsinfo.bcacheshm->uptime = now_t - 100000;
	}

	if (stat(BOARDS, &st) < 0) {
		errlog("BOARDS stat error: %s", strerror(errno));
		st.st_mtime = now_t - 3600;
	}
	if (bbsinfo.bcacheshm->uptime < st.st_mtime
	    || bbsinfo.bcacheshm->uptime < now_t - 3600) {
		bbsinfo.bcacheshm->uptime = now_t;
		countboard = 0;
		new_apply_record(BOARDS, sizeof (struct boardheader),
				 (void *) fillbcache, &countboard);
		bbsinfo.bcacheshm->number = countboard;
		numboards = countboard;
		tracelog("system reload bcache %d", numboards);
		if (bbsinfo.utmpshm->syncbmonline) {
			bbsinfo.utmpshm->syncbmonline = 0;
			bmonlinesync();
		}
		time(&now2);
		if (stat(BOARDS, &st) >= 0 && st.st_mtime < now_t)
			bbsinfo.bcacheshm->uptime = now2;
	}
	close(lockfd);
}

void
resolve_boards()
{
	struct stat st;
	time_t now;
	static int n = 0;

	if (n == 0)
		reload_boards();
	numboards = bbsinfo.bcacheshm->number;
	if (n++ % 30)
		return;
	time(&now);
	if (stat(BOARDS, &st) < 0) {
		st.st_mtime = now - 3600;
	}
	if (bbsinfo.bcacheshm->uptime < st.st_mtime
	    || bbsinfo.bcacheshm->uptime < now - 3600)
		reload_boards();

}

int
apply_boards(func, param)
int (*func) (struct boardmem *, void *);
void *param;
{
	int i;

	resolve_boards();
	for (i = 0; i < numboards; i++)
		if ((*func) (&(bbsinfo.bcache[i]), param) == QUIT)
			return QUIT;
	return 0;
}

int gbccount = 0, gbcsame = 0;

struct boardmem *
getbcache(bname)
char *bname;
{
	int i;
	static struct boardmem *last = NULL;

	gbccount++;

	if (last && !strncasecmp(last->header.filename, bname, STRLEN)) {
		gbcsame++;
		return last;
	}

	resolve_boards();
	for (i = 0; i < numboards; i++)
		if (!strncasecmp
		    (bname, bbsinfo.bcache[i].header.filename, STRLEN)) {
			last = &(bbsinfo.bcache[i]);
			return &(bbsinfo.bcache[i]);
		}
	return NULL;
}

int
updatelastpost(char *board)
{
	struct boardmem *bptr;
	bptr = getbcache(board);
	if (bptr == NULL)
		return -1;
	getlastpost(bptr->header.filename, &bptr->lastpost, &bptr->total);
	return 0;
}

int
hasreadperm(struct boardheader *bh)
{
	if (bh->clubnum != 0)
		return ((HAS_CLUBRIGHT(bh->clubnum, uinfo.clubrights))
			|| USERPERM(currentuser, PERM_SYSOP));
	if (bh->level & PERM_SPECIAL3)
		return die;
	return bh->level & PERM_POSTMASK || USERPERM(currentuser, bh->level)
	    || (bh->level & PERM_NOZAP);
}

int
getbnum(bname)
char *bname;
{
	int i;
	static int cachei = 0;

	resolve_boards();

	if (!strncasecmp(bname, bbsinfo.bcache[cachei].header.filename, STRLEN)) {
		if (hasreadperm(&(bbsinfo.bcache[cachei].header)))
			return cachei + 1;
		return 0;
	}
	for (i = 0; i < numboards; i++) {
		if (strncasecmp
		    (bname, bbsinfo.bcache[i].header.filename, STRLEN))
			continue;
		cachei = i;
		if (hasreadperm(&(bbsinfo.bcache[i].header)))
			return i + 1;
	}
	return 0;
}

int
canberead(bname)
char *bname;
{
	int i;
	if ((i = getbnum(bname)) == 0)
		return 0;
	return hasreadperm(&(bbsinfo.bcache[i - 1].header));
}

int
noadm4political(bnum)
int bnum;
{
	time_t now = time(NULL);
	if (!bbsinfo.utmpshm->watchman || now < bbsinfo.utmpshm->watchman)
		return 0;
	return political_board(bbsinfo.bcache[bnum - 1].header.filename, bnum);

}

int
haspostperm(bnum)
int bnum;
{
	if (bnum == 0)
		return 0;
	if (digestmode)
		return 0;
	if (strcmp(bbsinfo.bcache[bnum - 1].header.filename, DEFAULTBOARD) == 0)
		return 1;
	if (!USERPERM(currentuser, PERM_POST))
		return 0;
	if (bbsinfo.bcache[bnum - 1].header.level & PERM_SPECIAL3)
		return die;
	if (bbsinfo.bcache[bnum - 1].header.level != 0)
		if (!
		    (USERPERM
		     (currentuser,
		      (bbsinfo.bcache[bnum - 1].
		       header.level & ~PERM_NOZAP) & ~PERM_POSTMASK)))
			return 0;
	return 1;
}

int
posttest(uid, bname)
char *uid;
char *bname;
{
	int i, j;
	struct userec *lookupuser;

	i = getuser(uid, &lookupuser);
	if (i == 0)
		return 0;
	if (strcmp(bname, DEFAULTBOARD) == 0)
		return 1;
	if ((i = getbnum(bname)) == 0)
		return 0;
	if (bbsinfo.bcache[i - 1].header.flag & CLUB_FLAG) {
		setbfile(genbuf, currboard, "club_users");
		return seek_in_file(genbuf, uid);
	}
	if (bbsinfo.bcache[i - 1].header.level == 0)
		return 1;
	j = ((bbsinfo.bcache[i - 1].header.
	      level & ~PERM_NOZAP) & ~PERM_POSTMASK);
	if (!j)
		return 1;
	return (lookupuser->userlevel & j);
}

int
hideboard(bname)
char *bname;
{
	register int i;

	if (strcmp(bname, DEFAULTBOARD) == 0)
		return 0;
	if ((i = getbnum(bname)) == 0)
		return 1;
	if (bbsinfo.bcache[i - 1].header.level & PERM_NOZAP)
		return 0;
	if (bbsinfo.bcache[i - 1].header.flag & CLOSECLUB_FLAG)
		return 1;
	return (bbsinfo.bcache[i - 1].header.
		level & PERM_POSTMASK) ? 0 : bbsinfo.bcache[i - 1].header.level;
}

int
normal_board(bname)
char *bname;
{
	register int i;

	if (strcmp(bname, DEFAULTBOARD) == 0)
		return 1;
	if ((i = getbnum(bname)) == 0)
		return 0;
	if (bbsinfo.bcache[i - 1].header.flag & CLOSECLUB_FLAG)
		return 0;
	if (bbsinfo.bcache[i - 1].header.level & PERM_NOZAP)
		return 1;
	return (bbsinfo.bcache[i - 1].header.level == 0);
}

int
njuinn_board(bname)
char *bname;
{
	register int i;

	if ((i = getbnum(bname)) == 0)
		return 0;
	return (bbsinfo.bcache[i - 1].header.flag2 & NJUINN_FLAG);
}

int
innd_board(bname)
char *bname;
{
	register int i;

	if ((i = getbnum(bname)) == 0)
		return 0;
	return (bbsinfo.bcache[i - 1].header.flag & INNBBSD_FLAG);
}

int
is1984_board(bname)
char *bname;
{
	register int i;

	if ((i = getbnum(bname)) == 0)
		return 0;
	return (bbsinfo.bcache[i - 1].header.flag & IS1984_FLAG);
}

int
political_board(bname, bnum)
char *bname;
int bnum;
{
	register int i;
	if (bnum == 0) {
		if ((i = getbnum(bname)) == 0)
			return 0;
	} else
		i = bnum;
	if (bbsinfo.bcache[i - 1].header.flag & POLITICAL_FLAG)
		return 1;
	else
		return 0;
}

int
club_board(bname, bnum)
char *bname;
int bnum;
{
	register int i;
	if (bnum == 0) {
		if ((i = getbnum(bname)) == 0)
			return 0;
	} else
		i = bnum;
	if (bbsinfo.bcache[i - 1].header.flag & CLUB_FLAG)
		return 1;
	return 0;
}

int
clubsync(bnum)
int bnum;
{
	FILE *fp;
	int old_right;
	static int club_rights_time = 0;
	char fn[STRLEN];
	struct stat st1, st2;
	if (bnum == 0)
		return 0;
	if (bbsinfo.bcache[bnum - 1].header.clubnum == 0)	//仅考虑完全的closeclub
		return 1;
	if (USERPERM(currentuser, PERM_SYSOP))	//站务总能把持这样的club
		return 1;
	if (bbsinfo.bcache[bnum - 1].header.clubnum >
	    CLUB_SIZE * sizeof (int) * 8) {
		errlog("clubnum too large..");
		return 0;
	}
	setuserfile(fn, "clubrights");
	if (stat(fn, &st1))
		memset(&(uinfo.clubrights), 0, CLUB_SIZE * sizeof (int));
	else if (club_rights_time < st1.st_mtime) {
		if ((fp = fopen(fn, "r")) != NULL) {
			fread(&(uinfo.clubrights), sizeof (int), CLUB_SIZE, fp);
			club_rights_time = st1.st_mtime;
			fclose(fp);
		} else
			memset(&(uinfo.clubrights), 0,
			       CLUB_SIZE * sizeof (int));
	}
	old_right =
	    HAS_CLUBRIGHT(bbsinfo.bcache[bnum - 1].header.clubnum,
			  uinfo.clubrights);
	setbfile(fn, bbsinfo.bcache[bnum - 1].header.filename, "club_users");
	if (!stat(fn, &st2))
		if (club_rights_time < st2.st_mtime) {
			if (seek_in_file(fn, currentuser->userid))
				uinfo.clubrights[bbsinfo.bcache[bnum - 1].
						 header.clubnum / (8 *
								   sizeof
								   (int))] |=
				    (1 << bbsinfo.bcache[bnum - 1].header.
				     clubnum % (8 * sizeof (int)));
			else
				uinfo.clubrights[bbsinfo.bcache[bnum - 1].
						 header.clubnum / (8 *
								   sizeof
								   (int))] &=
				    ~(1 << bbsinfo.bcache[bnum - 1].header.
				      clubnum % (8 * sizeof (int)));
			if (old_right !=
			    HAS_CLUBRIGHT(bbsinfo.bcache[bnum - 1].header.
					  clubnum, uinfo.clubrights)) {
				setuserfile(fn, "clubrights");
				if ((fp = fopen(fn, "w")) != NULL) {
					fwrite(&(uinfo.clubrights),
					       sizeof (int), CLUB_SIZE, fp);
					fclose(fp);
					club_rights_time = time(NULL);
				}
			}
		}
	return HAS_CLUBRIGHT(bbsinfo.bcache[bnum - 1].header.clubnum,
			     uinfo.clubrights);
}

int
apply_ulist(fptr)
int (*fptr) (struct user_info *);
{
	int i;

	for (i = 0; i < MAXACTIVE; i++) {
		if (bbsinfo.utmpshm->uinfo[i].active == 0)
			continue;
		if ((*fptr) (&bbsinfo.utmpshm->uinfo[i]) == QUIT)
			return QUIT;
	}
	return 0;
}

int
search_ulist(uentp, fptr, farg)
struct user_info *uentp;
int (*fptr) (int, struct user_info *);
int farg;
{
	int i;

	for (i = 0; i < MAXACTIVE; i++) {
		if (bbsinfo.utmpshm->uinfo[i].active == 0)
			continue;
		*uentp = bbsinfo.utmpshm->uinfo[i];
		if ((*fptr) (farg, uentp)) {
			return i + 1;
		}
	}
	return 0;
}

int
search_ulistn(uentp, fptr, farg, unum)
struct user_info *uentp;
int (*fptr) (int, struct user_info *);
int farg;
int unum;
{
	int i, j;
	j = 1;
	for (i = 0; i < MAXACTIVE; i++) {
		*uentp = bbsinfo.utmpshm->uinfo[i];
		if ((*fptr) (farg, uentp)) {
			if (j == unum)
				return i + 1;
			else
				j++;
		}

	}
	return 0;
}

/*Function Add by SmallPig*/
int
count_logins(uentp, fptr, farg, show)
struct user_info *uentp;
int (*fptr) (int, struct user_info *);
int farg;
int show;
{
	int i, j;

	j = 0;
	for (i = 0; i < MAXACTIVE; i++) {
		*uentp = bbsinfo.utmpshm->uinfo[i];
		if ((*fptr) (farg, uentp)) {
			if (show == 0)
				j++;
			else {
				j++;
				prints
				    ("(%d) 目前在干嘛: %s, 来自: %s \n",
				     j, ModeType(uentp->mode), uentp->from);
			}
		}
	}
	return j;
}

int
t_search_ulist(int uid)
{
	int i;
	int num = 0;

	struct user_info *uentp;
	int uent;

	if (uid == getuser("guest", NULL)) {
		prints("目前可能在站上，状态如下：\n难得糊涂\n");
		return 0;
	}
	for (i = 0; i < 6; i++) {
		uent = bbsinfo.uindexshm->user[uid - 1][i];
		if (uent <= 0)
			continue;
		uentp = &(bbsinfo.utmpshm->uinfo[uent - 1]);
		if (!uentp->active || !uentp->pid || isreject(uentp, &uinfo))
			continue;
		if (uentp->invisible == 1) {
			if (USERPERM(currentuser, (PERM_SYSOP | PERM_SEECLOAK))) {
				prints("\033[1;32m隐身中   \033[m");
				continue;
			} else
				continue;
		}
		num++;
		if (num == 1)
			prints("目前在站上，状态如下：\n");
		prints("%s%-16s\033[m ",
		       uentp->pid == 1 ? "\033[35m" : "\033[1m",
		       ModeType(uentp->mode));
		if ((num) % 5 == 0)
			prints("\n");
	}
	prints("\n");
	return 0;
}

void
update_ulist(uentp, uent)
struct user_info *uentp;
int uent;
{
	if (uent > 0 && uent <= MAXACTIVE) {
		memcpy(&bbsinfo.utmpshm->uinfo[uent - 1].invisible,
		       &(uentp->invisible),
		       sizeof (struct user_info) -
		       ((char *) &uentp->invisible - (char *) uentp));
/*utmpshm->uinfo[ uent - 1 ] = *uentp;*/
	}
}

void
update_utmp()
{
	extern time_t last_utmp_update;
	last_utmp_update = uinfo.lasttime;
	update_ulist(&uinfo, utmpent);
}

						    /* added by djq 99.7.19 *//* function added by douglas 990305
						       set uentp to the user who is calling me
						       solve the "one of 2 line call sb. to five" problem
						     */ int
who_callme(uentp, fptr, farg, me)
struct user_info *uentp;
int (*fptr) (int, struct user_info *);
int farg;
int me;
{
	int i;

	for (i = 0; i < MAXACTIVE; i++) {
		*uentp = bbsinfo.utmpshm->uinfo[i];
		if ((*fptr) (farg, uentp) && uentp->destuid == me)
			return i + 1;
	}
	return 0;
}

int
getbmnum(userid)
char *userid;
{
	int i, k, oldbm = 0;
	reload_boards();
	for (k = 0; k < numboards; k++) {
		for (i = 0; i < BMNUM; i++) {
			if (bbsinfo.bcache[k].header.bm[i][0] == 0) {
				if (i < 4) {
					i = 3;
					continue;
				}
				break;
			}
			if (!strcmp(bbsinfo.bcache[k].header.bm[i], userid))
				oldbm++;
		}
	}
	return oldbm;
}

struct user_info *
query_uindex(int uid, int dotest)
{
	int i, uent, testreject = 0;
	struct user_info *uentp, *wwwu = NULL;
	if (uid <= 0 || uid > MAXUSERS)
		return 0;
	for (i = 0; i < 6; i++) {
		uent = bbsinfo.uindexshm->user[uid - 1][i];
		if (uent <= 0)
			continue;
		uentp = &(bbsinfo.utmpshm->uinfo[uent - 1]);
		if (!uentp->active || !uentp->pid || uentp->uid != uid)
			continue;
		if (dotest && !testreject) {
			if (isreject(uentp, &uinfo))
				return 0;
			testreject = 1;
		}
		if (dotest && bbsinfo.utmpshm->uinfo[uent - 1].invisible
		    && !USERPERM(currentuser, (PERM_SYSOP | PERM_SEECLOAK)))
			continue;
		if (uentp->pid != 1)
			return uentp;
		else
			wwwu = uentp;

	}
	return wwwu;
}

int
count_uindex(int uid)
{
	int i, uent, count = 0;
	struct user_info *uentp;
	if (uid <= 0 || uid > MAXUSERS)
		return 0;
	for (i = 0; i < 6; i++) {
		uent = bbsinfo.uindexshm->user[uid - 1][i];
		if (uent <= 0)
			continue;
		uentp = &(bbsinfo.utmpshm->uinfo[uent - 1]);
		if (!uentp->active || !uentp->pid || uentp->uid != uid)
			continue;
		if (uentp->pid > 1 && kill(uentp->pid, 0) < 0) {
			bbsinfo.uindexshm->user[uid - 1][i] = 0;
			continue;
		}
		count++;
	}
	return count;
}

int
count_uindex_telnet(int uid)
{
	int i, uent, count = 0;
	struct user_info *uentp;
	if (uid <= 0 || uid > MAXUSERS)
		return 0;
	for (i = 0; i < 6; i++) {
		uent = bbsinfo.uindexshm->user[uid - 1][i];
		if (uent <= 0)
			continue;
		uentp = &(bbsinfo.utmpshm->uinfo[uent - 1]);
		if (!uentp->active || uentp->pid <= 1 || uentp->uid != uid)
			continue;
		if (uentp->pid > 1 && kill(uentp->pid, 0) < 0) {
			bbsinfo.uindexshm->user[uid - 1][i] = 0;
			continue;
		}
		count++;
	}
	return count;
}

char *
get_temp_sessionid(char *ts)
{
	myitoa(utmpent - 1, ts, 3);
	myitoa(usernum, ts + 3, 4);
	strncpy(ts + 7, uinfo.sessionid, 3);
	ts[9] = 0;
	return ts;
}

void
show_small_bm(char *board)
{
	struct boardmem *bptr;
	int i;
	short active, invisible;
	bptr = getbcache(board);
	if (bptr == NULL)
		return;
	move(0, 0);
	prints("\033[m");
	prints("\033[1;44;33m");
	for (i = 4; i < 10; i++) {	//仅对12个小版务的显示
		if (bptr->header.bm[i][0] == 0) {
			if (i == 4) {
				prints("本版无小版主 ");
				continue;
			} else {
				prints("             ");
				continue;
			}
		}
		active = bptr->bmonline & (1 << i);
		invisible = bptr->bmcloak & (1 << i);
		if (active && !invisible)
			prints("\033[32m%13s\033[33m", bptr->header.bm[i]);
		else if (active && invisible
			 && (USERPERM(currentuser, PERM_SEECLOAK)
			     || !strcmp(bptr->header.bm[i],
					currentuser->userid)))
			prints("\033[36m%13s\033[33m", bptr->header.bm[i]);
		else
			prints("%13s", bptr->header.bm[i]);
	}
	move(1, 0);
	prints("\033[1;44;33m");
	for (i = 10; i < BMNUM; i++) {
		if (bptr->header.bm[i][0] == 0) {
			prints("             ");
			continue;
		}
		active = bptr->bmonline & (1 << i);
		invisible = bptr->bmcloak & (1 << i);
		if (active && !invisible)
			prints("\033[32m%13s\033[33m", bptr->header.bm[i]);
		else if (active && invisible
			 && (USERPERM(currentuser, PERM_SEECLOAK)
			     || !strcmp(bptr->header.bm[i],
					currentuser->userid)))
			prints("\033[36m%13s\033[33m", bptr->header.bm[i]);
		else
			prints("%13s", bptr->header.bm[i]);

	}
	prints("\033[m");
	return;
}

int
setbmstatus(int online)
{
	return dosetbmstatus(bbsinfo.bcache, currentuser->userid, online,
			     uinfo.invisible);
}

static void
bmonlinesync()
{
	int j, k;
	struct user_info *uentp;
	for (j = 0; j < numboards; j++) {
		if (!bbsinfo.bcache[j].header.filename[0])
			continue;
		bbsinfo.bcache[j].bmonline = 0;
		bbsinfo.bcache[j].bmcloak = 0;
		for (k = 0; k < BMNUM; k++) {
			if (bbsinfo.bcache[j].header.bm[k][0] == 0) {
				if (k < 4) {
					k = 3;	//继续检查小版主
					continue;
				}
				break;	//小版主也检查完了
			}
			uentp =
			    query_uindex(getuser
					 (bbsinfo.bcache[j].header.bm[k], NULL),
					 0);
			if (!uentp)
				continue;
			bbsinfo.bcache[j].bmonline |= (1 << k);
			if (uentp->invisible)
				bbsinfo.bcache[j].bmcloak |= (1 << k);
		}
	}
	tracelog("system reload bmonline");
}
