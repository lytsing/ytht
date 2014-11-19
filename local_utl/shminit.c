#include "bbs.h"
#include "ythtbbs.h"

struct bbsinfo bbsinfo;

static int
check_passwd_size(void)
{
	struct stat st;
	/** 
	 * truncate the .PASSWDS to extend the file to the max size,
	 * so the mmap function will not interupted by SIGBUS
	 * but the behavior of truncate() may only works on Linux *
	 * */
	if (stat(PASSFILE, &st) == -1)
		return -1;
	if (MAXUSERS * sizeof (struct userec) > st.st_size) {
		if (truncate(PASSFILE, MAXUSERS * sizeof (struct userec)) == -1) {
			errlog("truncate .PASSWDS error, %d", errno);
			return -1;
		}
	} else if (MAXUSERS * sizeof (struct userec) < st.st_size) {
		errlog("MAXUSERS is too small");
		return -1;
	}
	return 0;
}

int
initucachehash()
{
	int i;
	if (load_ucache() < 0)
		return -1;
	for (i = 0; i < 4; i++) {
		getrandomint(&(bbsinfo.ucachehashshm->regkey[i]));
		getrandomint(&(bbsinfo.ucachehashshm->oldregkey[i]));
	}
	bbsinfo.ucachehashshm->keytime = time(NULL);
	bbsinfo.ucachehashshm->uptime = time(NULL);
	return 0;
}

int
initutmp()
{
	int i;
	bbsinfo.utmpshm->guesthash_head[0] = 1;
	for (i = 1; i <= MAXACTIVE; i++)
		bbsinfo.utmpshm->guesthash_next[i] = i + 1;
	bbsinfo.utmpshm->guesthash_next[MAXACTIVE] = 0;
	return 0;
}

static int
fillbcache(struct boardheader  *fptr, int *pcountboard)
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


int
initbcache()
{
	int lockfd, countboard;

	lockfd = open("bcache.lock", O_RDONLY | O_CREAT, 0600);
	if (lockfd < 0)
		return -1;
	flock(lockfd, LOCK_EX);

	countboard = 0;
	new_apply_record(BOARDS, sizeof (struct boardheader), (void *) fillbcache, &countboard);
	bbsinfo.bcacheshm->number = countboard;
	bbsinfo.bcacheshm->uptime = time(NULL);
	close(lockfd);
	return 0;
}


int
main()
{
	struct bbsinfo mybbsinfo;

	if (chdir(MY_BBS_HOME) == -1)
		return -1;

	bzero(&mybbsinfo, sizeof (struct bbsinfo));

	if (geteuid() != BBSUID) {
		printf
		    ("\nplease run this program as bbs user\ndon't run this program as root!\n");
		goto ERROR;
	}

	printf("clear old shm...");
	if (system
	    ("for i in `ipcs -m|grep " BBSUSER
	     "|awk '{print $2}'`;do ipcrm shm $i;done")) {
		printf("\ncan't clear old shm...\n");
		goto ERROR;
	}
	printf("ok\n");
	printf("creating bcache shm...");
	mybbsinfo.bcacheshm =
	    get_shm(getBBSKey(BCACHE_SHM), sizeof (struct BCACHE));
	if (mybbsinfo.bcacheshm == NULL)
		goto ERROR;
	printf("ok\n");
	shmdt(mybbsinfo.bcacheshm);

	printf("creating ucachehash shm...");
	mybbsinfo.ucachehashshm =
	    get_shm(getBBSKey(UCACHE_HASH_SHM), sizeof (struct UCACHEHASH));
	if (mybbsinfo.ucachehashshm == NULL)
		goto ERROR;
	printf("ok\n");
	shmdt(mybbsinfo.ucachehashshm);

	printf("creating utmp shm ...");
	mybbsinfo.utmpshm =
	    get_shm(getBBSKey(UTMP_SHM), sizeof (struct UTMPFILE));
	if (mybbsinfo.utmpshm == NULL)
		goto ERROR;
	printf("ok\n");
	shmdt(mybbsinfo.utmpshm);

	printf("creating uindex shm...");
	mybbsinfo.uindexshm =
	    get_shm(getBBSKey(UINDEX_SHM), sizeof (struct UINDEX));
	if (mybbsinfo.uindexshm == NULL)
		goto ERROR;
	printf("ok\n");
	shmdt(mybbsinfo.uindexshm);

	printf("creating wwwcache shm...");
	mybbsinfo.wwwcache =
	    get_shm(getBBSKey(WWWCACHE_SHM), sizeof (struct WWWCACHE));
	if (mybbsinfo.wwwcache == NULL)
		goto ERROR;
	printf("ok\n");
	shmdt(mybbsinfo.wwwcache);

	printf("check passwd file size...");
	if (check_passwd_size() < 0)
		goto ERROR;
	printf("ok\n");
	printf("attaching shm...");
	if (initbbsinfo(&bbsinfo) < 0)
		goto ERROR;
	printf("ok\n");

	printf("check for uhash...");
	if (uhash_uptime() > 0)
		goto ERROR;
	printf("ok\n");

	printf("building ucachehash shm...");
	if (initucachehash() < 0)
		goto ERROR;
	printf("ok\n");

	printf("building utmp shm...");
	if (initutmp() < 0)
		goto ERROR;
	printf("ok\n");

	printf("building bcache shm...");
	if (initbcache() < 0)
		goto ERROR;
	printf("ok\n");

	printf("everything is just fine\n");
	return 0;
      ERROR:
	printf("oops, shminit failed, please clear all shm and retry\n");
	return -1;
}
