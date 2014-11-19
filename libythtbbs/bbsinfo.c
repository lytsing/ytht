#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include "ythtbbs.h"

static struct bbsinfo *binfo = NULL;
struct userec *passwdptr;
struct mmapfile passwd_mf = { ptr:NULL };

key_t
getBBSKey(int projID)
{
	//return projID;
	//To use ftok, only the lowest 8 bits of projID are used.
	//So we need to re-define the constants in bbsinfo.h, such
	//as BBSLOGMSQ, ENDLINE_SHM
	return ftok(MY_BBS_HOME, projID);
}

int
initbbsinfo(struct bbsinfo *bbsinfo)
{
	bzero(bbsinfo, sizeof (*bbsinfo));
	binfo = bbsinfo;

	if (chdir(MY_BBS_HOME) == -1)
		goto ERROR;

	bbsinfo->bcacheshm =
	    get_old_shm(getBBSKey(BCACHE_SHM), sizeof (struct BCACHE));
	if (bbsinfo->bcacheshm == NULL)
		goto ERROR;
	bbsinfo->bcache = bbsinfo->bcacheshm->bcache;

	bbsinfo->ucachehashshm =
	    get_old_shm(getBBSKey(UCACHE_HASH_SHM), sizeof (struct UCACHEHASH));
	if (bbsinfo->ucachehashshm == NULL)
		goto ERROR;

	bbsinfo->utmpshm =
	    get_old_shm(getBBSKey(UTMP_SHM), sizeof (struct UTMPFILE));
	if (bbsinfo->utmpshm == NULL)
		goto ERROR;

	bbsinfo->uindexshm =
	    get_old_shm(getBBSKey(UINDEX_SHM), sizeof (struct UINDEX));
	if (bbsinfo->uindexshm == NULL)
		goto ERROR;

	bbsinfo->wwwcache =
	    get_old_shm(getBBSKey(WWWCACHE_SHM), sizeof (struct WWWCACHE));
	if (bbsinfo->wwwcache == NULL)
		goto ERROR;

	if (mmapfile(".PASSWDS", &passwd_mf) < 0)
		goto ERROR;
	passwdptr = (struct userec *) passwd_mf.ptr;
	return 0;

      ERROR:
	mmapfile(NULL, &passwd_mf);
	if (bbsinfo->bcacheshm)
		shmdt(bbsinfo->bcacheshm);
	if (bbsinfo->ucachehashshm)
		shmdt(bbsinfo->ucachehashshm);
	if (bbsinfo->utmpshm)
		shmdt(bbsinfo->utmpshm);
	if (bbsinfo->uindexshm)
		shmdt(bbsinfo->uindexshm);
	if (bbsinfo->wwwcache)
		shmdt(bbsinfo->wwwcache);
	return -1;
}

/*
   CaseInsensitive ucache_hash
 */
static unsigned int
ucache_hash_deep(const char *userid)
{
	int n1, n2, n;
	struct ucache_hashtable *hash;

	if (!*userid)
		return 0;
	hash = &(binfo->ucachehashshm->hashtable);

	n1 = mytoupper(*userid++);
	n1 -= 'A';

	n1 = hash->hash[0][n1];

	n = 1;
	while (n1 < 0) {
		n1 = -n1 - 1;
		if (!*userid) {
			n1 = hash->hash[n1][0];
		} else {
			n2 = mytoupper(*userid++);
			n2 -= 'A';
			n1 = hash->hash[n1][n2];
		}
		n++;
	}
	return n;
}

/*
   CaseInsensitive ucache_hash 
 */
unsigned int
ucache_hash(const char *userid)
{
	int n1, n2, n, len;
	struct ucache_hashtable *hash;

	if (!*userid)
		return 0;

	hash = &(binfo->ucachehashshm->hashtable);
	n1 = mytoupper(*userid++);
	n1 -= 'A';

	n1 = hash->hash[0][n1];

	while (n1 < 0) {
		n1 = -n1 - 1;
		if (!*userid) {
			n1 = hash->hash[n1][0];
		} else {
			n2 = mytoupper(*userid++);
			n2 -= 'A';
			n1 = hash->hash[n1][n2];
		}
	}
	n1 = (n1 * UCACHE_HASHBSIZE) % UCACHE_HASHSIZE + 1;
	if (!*userid)
		return n1;

	n2 = 0;
	len = strlen(userid);
	while (*userid) {
		n = mytoupper(*userid++);
		n2 += (n - 'A') * len;
		len--;
	}
	n1 = (n1 + n2 % UCACHE_HASHBSIZE) % UCACHE_HASHSIZE + 1;
	return n1;
}

int
ucache_hashinit()
{
	FILE *fp;
	char line[256];
	int i, j, ptr, data;

	fp = fopen(MY_BBS_HOME "/uhashgen.dat", "rt");
	if (!fp) {
		errlog("can't found " MY_BBS_HOME "/uhashgen.dat");
		return -1;
	}
	i = 0;
	while (fgets(line, sizeof (line), fp)) {
		if (line[0] == '#')
			continue;
		j = 0;
		ptr = 0;
		while ((line[ptr] >= '0' && line[ptr] <= '9')
		       || line[ptr] == '-') {
			data = ptr;
			while ((line[ptr] >= '0' && line[ptr] <= '9')
			       || line[ptr] == '-')
				ptr++;
			line[ptr++] = 0;
			binfo->ucachehashshm->hashtable.hash[i][j++] =
			    atoi(&line[data]);
		}
		i++;
		if (i >
		    (int) sizeof (binfo->ucachehashshm->hashtable.hash) / 26) {
			errlog("hashline %d exceed", i);
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

//insertuseridhash和finduseridhash的num都是按照起始值为1进行的
//这个其实是调用者决定的
//调用者要保证插入的 num 处对应条目即将或已经被设置为userid
//tohead参数表示，如果插入的是空用户，插入在表的头部还是尾部
int
insertuseridhash(const char *userid, const int num, const int tohead)
{
	int i;
	unsigned int h;
	static int lastspace = -1;
	struct UCACHEHASH *uhash = binfo->ucachehashshm;
	h = ucache_hash(userid);
	i = uhash->hash_head[h];
	if (h != 0 || tohead) {
		uhash->next[num] = uhash->hash_head[h];
		uhash->hash_head[h] = num;
	} else {
		if (lastspace == -1)
			uhash->hash_head[h] = num;
		else
			uhash->next[lastspace] = num;
		uhash->next[num] = 0;
		lastspace = num;
	}
	if (userid[0] == 0)
		uhash->next[0]++;
	return 0;
}

int
finduseridhash(const char *userid)
{
	int i;
	unsigned h;
	struct UCACHEHASH *uhash = binfo->ucachehashshm;
	h = ucache_hash(userid);
	i = uhash->hash_head[h];
	while (i) {
		if (!strcasecmp(userid, passwdptr[i - 1].userid)) {
			return i;
		}
		i = uhash->next[i];
	}
	return -1;
}

int
deluseridhash(const char *userid)
{
	int i;
	unsigned h;
	struct UCACHEHASH *uhash = binfo->ucachehashshm;
	int prev = -1;
	h = ucache_hash(userid);
	i = uhash->hash_head[h];
	while (i) {
		if (!strcasecmp(userid, passwdptr[i - 1].userid)) {
			if (prev != -1) {
				uhash->next[prev] = uhash->next[i];
			} else {
				uhash->hash_head[h] = uhash->next[i];
			}
			if (userid[0] == 0)
				uhash->next[0]--;
			return i;
		}
		prev = i;
		i = uhash->next[i];
	}
	return -1;
}

//hash的初始化
int
load_ucache()
{
	int i;
	int lockfd;

	lockfd = lock_passwd();
	bzero(binfo->ucachehashshm, sizeof (struct UCACHEHASH));

	if (ucache_hashinit() < 0) {
		unlock_passwd(lockfd);
		return -1;
	}

	for (i = 0; i < MAXUSERS; i++) {
		if (passwdptr[i].userid[0]
		    && finduseridhash(passwdptr[i].userid) > 0) {
			unlock_passwd(lockfd);
			errlog("duplicate user %s", passwdptr[i].userid);
			return -1;
		}
		insertuseridhash(passwdptr[i].userid, i + 1, 0);
	}
	unlock_passwd(lockfd);
	return 0;
}

char *
u_namearray(char buf[][IDLEN + 1], int max, int *pnum, char *tag, char *atag,
	    int *full)
/* 根据tag ,生成 匹配的user id 列表 (针对所有注册用户)*/
{
	struct UCACHEHASH *reg_ushm = binfo->ucachehashshm;
	int n, num, i;
	int hash, len, ksz, alen;
	char tagv[IDLEN + 1];

	len = strlen(tag);
	alen = strlen(atag);
	if (len > IDLEN)
		return NULL;
	if (!len) {
		return NULL;
	}
	ksz = ucache_hash_deep(tag);

	strcpy(tagv, tag);

	if (len >= ksz || len == IDLEN) {
		tagv[ksz] = 0;
		hash = ucache_hash(tagv) - 1;
		for (n = 0; n < UCACHE_HASHBSIZE; n++) {
			num =
			    reg_ushm->hash_head[(hash + n % UCACHE_HASHBSIZE) %
						UCACHE_HASHSIZE + 1];
			while (num) {
				if (!strncasecmp
				    (passwdptr[num - 1].userid, atag, alen)
				    && (passwdptr[num - 1].kickout == 0)) {
					strcpy(buf[(*pnum)++], passwdptr[num - 1].userid);	/*如果匹配, add into buf */
					if (*pnum >= max) {
						*full = 1;
						return buf[0];
					}
				}
				num = reg_ushm->next[num];
			}
		}
	} else {
		for (i = 'A'; i <= 'Z'; i++) {
			tagv[len] = i;
			tagv[len + 1] = 0;

			u_namearray(buf, max, pnum, tagv, atag, full);
			if (*full == 1)
				return buf[0];
			if (mytoupper(tagv[len]) == 'Z') {
				tagv[len] = 'A';
				return buf[0];
			} else {
				tagv[len]++;
			}
		}

	}
	return buf[0];
}

int
usersum()
{
	return MAXUSERS - binfo->ucachehashshm->next[0];
}

time_t
uhash_uptime()
{
	return binfo->ucachehashshm->uptime;
}

//uid的第一位置编号1
struct user_info *
queryUIndex(int uid, struct user_info *reader, int pid, int *pos)
{
	int i, uent, testreject = 0;
	struct user_info *uentp;
	if (uid <= 0 || uid > MAXUSERS) {
		*pos = -1;
		return NULL;
	}
	for (i = 0; i < 6; i++) {
		uent = binfo->uindexshm->user[uid - 1][i];
		if (uent <= 0)
			continue;
		uentp = &binfo->utmpshm->uinfo[uent - 1];
		if (!uentp->active || !uentp->pid || uentp->uid != uid)
			continue;
		if (pid != 0 && uentp->pid != pid)
			continue;
		if (reader && !testreject) {
			if (isreject(uentp, reader)) {
				*pos = -1;
				return NULL;
			}
			testreject = 1;
		}
		if (reader && binfo->utmpshm->uinfo[uent - 1].invisible
		    && !USERPERM(reader, (PERM_SYSOP | PERM_SEECLOAK)))
			continue;
		*pos = uent;
		return uentp;
	}
	*pos = -1;
	return NULL;
}

void
update_max_online()
{
	FILE *maxfp;
	int temp_max = 0;
	struct UTMPFILE *u = binfo->utmpshm;
	if (u->activeuser <= u->maxtoday)
		return;
	u->maxtoday = u->activeuser;
	if (u->activeuser <= u->maxuser)
		return;
	u->maxuser = u->activeuser;
	maxfp = fopen(MY_BBS_HOME "/.max_login_num", "r+");
	if (maxfp == NULL) {
		maxfp = fopen(MY_BBS_HOME "/.max_login_num", "w+");
		fprintf(maxfp, "%d", u->maxuser);
		fclose(maxfp);
	} else {
		fscanf(maxfp, "%d", &temp_max);
		if (temp_max > u->maxuser) {
			u->maxuser = temp_max;
		} else {
			fseek(maxfp, 0, SEEK_SET);
			fprintf(maxfp, "%d", u->maxuser);
		}
		fclose(maxfp);
	}
}

static void
add_uindex(int uid, int utmpent)
{
	int i, uent;
	if (uid <= 0 || uid > MAXUSERS || uid == getuser("guest", NULL))
		return;
	for (i = 0; i < 6; i++)
		if (binfo->uindexshm->user[uid - 1][i] == utmpent)
			return;
	for (i = 0; i < 6; i++) {
		uent = binfo->uindexshm->user[uid - 1][i];
		if (uent <= 0 || !binfo->utmpshm->uinfo[uent - 1].active ||
		    binfo->utmpshm->uinfo[uent - 1].uid != uid) {
			binfo->uindexshm->user[uid - 1][i] = utmpent;
			return;
		}
	}
}

static void
remove_uindex(int uid, int utmpent)
{
	int i;
	if (uid <= 0 || uid > MAXUSERS || uid == getuser("guest", NULL))
		return;
	for (i = 0; i < 6; i++) {
		if (binfo->uindexshm->user[uid - 1][i] == utmpent) {
			binfo->uindexshm->user[uid - 1][i] = 0;
			return;
		}
	}
}

static void
utmp_kickidle()
{
	int now;
	pid_t pid;
	int n, h, i;
	struct user_info *uentp, *u;
	now = time(NULL);
	if (now < binfo->utmpshm->uptime + 60 && now >= binfo->utmpshm->uptime)
		return;
	binfo->utmpshm->uptime = now;
	for (n = 0; n < MAXACTIVE; n++) {
		uentp = &(binfo->utmpshm->uinfo[n]);
		pid = uentp->pid;
		if (pid == 0)
			continue;
		if (uentp->active == 0)
			continue;
		if (pid == 1)	//www
			if ((now - uentp->lasttime < 20 * 60)
			    && !uentp->wwwinfo.iskicked)
				continue;
		if (pid > 1 && now - uentp->lasttime < 120)
			continue;
		if (pid > 1 && kill(pid, 0) == 0) {	//如果进程存在
#ifdef DOTIMEOUT
			if (now - uentp->lasttime >
			    ((uentp->ext_idle) ? IDLE_TIMEOUT *
			     3 : IDLE_TIMEOUT)) {
				kill(pid, SIGHUP);
			}
			continue;
		}
#endif
		if (!strcasecmp(uentp->userid, "guest")
		    && uentp->pid == 1) {
			u = &(binfo->utmpshm->uinfo[n]);
			h = utmp_iphash(u->from);
			i = binfo->utmpshm->guesthash_head[h];
			if (i == n + 1) {
				binfo->utmpshm->guesthash_head[h] =
				    binfo->utmpshm->guesthash_next[i];
			} else {
				while (binfo->utmpshm->guesthash_next[i] != 0) {
					if (binfo->utmpshm->guesthash_next[i] ==
					    n + 1) {
						binfo->utmpshm->
						    guesthash_next[i] =
						    binfo->utmpshm->
						    guesthash_next[n + 1];
						break;
					} else {
						i = binfo->utmpshm->
						    guesthash_next[i];
					}
				}
			}
			binfo->utmpshm->wwwguestnum--;
		}
		binfo->utmpshm->guesthash_next[n + 1] =
		    binfo->utmpshm->guesthash_head[0];
		binfo->utmpshm->guesthash_head[0] = n + 1;
		binfo->utmpshm->activeuser--;
		remove_uindex(uentp->uid, n + 1);
		uentp->active = 0;
		uentp->pid = 0;
		uentp->invisible = 1;
		uentp->sockactive = 0;
		uentp->sockaddr = 0;
		uentp->destuid = 0;
		uentp->lasttime = 0;
	}
}

int
utmp_login(struct user_info *up)
{
	struct user_info *uentp;
	int j;
	int h;

	j = binfo->utmpshm->guesthash_head[0] - 1;
	if (j < 0 || j >= MAXACTIVERUN) {
		return -1;
	}
	uentp = &(binfo->utmpshm->uinfo[j]);
	if (uentp->active && uentp->pid) {
		return -1;
	}
	binfo->utmpshm->guesthash_head[0] =
	    binfo->utmpshm->guesthash_next[j + 1];
	binfo->utmpshm->guesthash_next[j + 1] = 0;
	binfo->utmpshm->uinfo[j] = *up;
	binfo->utmpshm->activeuser++;
	update_max_online();
	add_uindex(up->uid, j + 1);
	if (!strcasecmp(up->userid, "guest") && up->pid == 1) {
		h = utmp_iphash(up->from);
		binfo->utmpshm->guesthash_next[j + 1] =
		    binfo->utmpshm->guesthash_head[h];
		binfo->utmpshm->guesthash_head[h] = j + 1;
		binfo->utmpshm->wwwguestnum++;
	}
	utmp_kickidle();
	return j + 1;
}

int
utmp_logout(int *utmpent, struct user_info *up)
{
	if (*utmpent < 0) {
		errlog("reenter update_utmp2!");
	} else if (!binfo->utmpshm->uinfo[*utmpent - 1].active)
		errlog("release a empty record! %d", *utmpent);
	else {
		remove_uindex(binfo->utmpshm->uinfo[*utmpent - 1].uid,
			      *utmpent);
		memcpy(&(binfo->utmpshm->uinfo[*utmpent - 1]), up,
		       sizeof (struct user_info));
		binfo->utmpshm->guesthash_next[*utmpent] =
		    binfo->utmpshm->guesthash_head[0];
		binfo->utmpshm->guesthash_head[0] = *utmpent;
		binfo->utmpshm->activeuser--;
	}
	*utmpent = -1;
	return 0;
}
