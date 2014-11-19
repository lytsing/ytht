#include "ythtbbs.h"
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
struct bbsinfo bbsinfo;
struct UTMPFILE *tocheck;
struct UINDEX *tocheck2;
struct UCACHEHASH *tocheck3;

int
iphash(char *fromhost)
{
	struct in_addr addr;
	inet_aton(fromhost, &addr);
	return (addr.s_addr % 4099) + 1;
}

int
checkutmp()
{
	int fd;
	int i;
	int j;
	int k, l;
	int h, m;
	int b;
	fd = open(MY_BBS_HOME "/.UTMP." MY_BBS_DOMAIN, O_WRONLY);
	if (fd < 0) {
		printf("utmp get lock file");
		return -1;
	}
	tocheck = malloc(sizeof (struct UTMPFILE));
	tocheck2 = malloc(sizeof (struct UINDEX));
	if (tocheck == NULL || tocheck2 == NULL) {
		printf("not enough memory");
		return -1;
	}
	flock(fd, LOCK_EX);
	memcpy(tocheck, bbsinfo.utmpshm, sizeof (struct UTMPFILE));
	memcpy(tocheck2, bbsinfo.uindexshm, sizeof (struct UINDEX));
	flock(fd, LOCK_UN);
	close(fd);
	for (k = 0; k < MAXUSERS; k++) {
		if (k == 1)
			continue;
		for (i = 0; i < 6; i++) {
			j = tocheck2->user[k][i];
			if (!j)
				continue;
			if (tocheck->uinfo[j - 1].uid != k + 1)
				printf("uindex err at %d %s\n", k,
				       tocheck->uinfo[j - 1].userid);
		}
	}
	for (k = 0; k < MAXACTIVE; k++) {
		b = 0;
		if (tocheck->uinfo[k].active
		    && strcmp(tocheck->uinfo[k].userid, "guest")) {
			l = tocheck->uinfo[k].uid - 1;
			for (i = 0; i < 6; i++) {
				j = tocheck2->user[l][i];
				if (j == k + 1)
					b = 1;
			}
			if (b == 0)
				printf("uindex err type 2 at %d %s %d\n", k,
				       tocheck->uinfo[k].userid, l);
		}
	}
	i = tocheck->guesthash_head[0];
	j = 0;
	while (i != 0) {
		if (tocheck->uinfo[i - 1].active != 0)
			printf("pos %d should empty but not\n", i);
		tocheck->uinfo[i - 1].active = 1;
		i = tocheck->guesthash_next[i];
		j++;
	}
	printf("empty slots in hash: %d\n", j);
	printf("active:  %d, should active: %d\n", tocheck->activeuser,
	       MAXACTIVE - j);
	j = 0;
	b = 0;
	for (i = 0; i < MAXACTIVE; i++) {
		if (tocheck->uinfo[i].active == 0) {
			k = i + 1;
			l = 0;
			while (tocheck->guesthash_next[k] != 0) {
				k = tocheck->guesthash_next[k];
				l++;
			}
			//if (l!=0)
			//      printf("seems broken link at %d %d\n", i+1, l);
			j++;
		}
		if (tocheck->uinfo[i].active == 1 && tocheck->uinfo[i].pid == 1
		    && tocheck->uinfo[i].uid == 3) {
			h = iphash(tocheck->uinfo[i].from);
			k = tocheck->guesthash_head[h];
			m = 0;
			while (k != 0) {
				if (i + 1 == k) {
					m++;
				}
				k = tocheck->guesthash_next[k];
			}
			if (m != 1)
				printf("www guest hash error, %d %d\n", i, m);
		}
		if (tocheck->uinfo[i].active == 0 && tocheck->uinfo[i].pid == 0) {
			for (h = 1; h <= 4099; h++) {
				k = tocheck->guesthash_head[h];
				m = 0;
				while (k != 0) {
					if (i + 1 == k) {
						m++;
					}
					k = tocheck->guesthash_next[k];
				}
				if (m >= 1) {
					b++;
				}
			}
		}
	}
	printf("empty slots in uinfo but not in hash: %d %d\n", j, b);
	return 0;
}

int
checkuhash()
{
	int fd;
	int i;
	int k;
	char *gu;
	fd = lock_passwd();
	tocheck3 = malloc(sizeof (struct UCACHEHASH));
	gu = calloc(sizeof (char), MAXUSERS);
	if (tocheck3 == NULL || gu == NULL) {
		printf("not enough memory");
		exit(0);
	}
	memcpy(tocheck3, bbsinfo.ucachehashshm, sizeof (struct UCACHEHASH));
	unlock_passwd(fd);
	for (i = 0; i < MAXUSERS; i++) {
		if (!passwdptr[i].userid[0]) {
			gu[i] = 1;
			continue;
		}
		k = tocheck3->hash_head[ucache_hash(passwdptr[i].userid)];
		while (k != 0 && k != i + 1) {
			if(k < 0 || k > MAXUSERS){
				printf("s: %s %d\n", passwdptr[i].userid, k);
				break;
			}
			k = tocheck3->next[k];
		}
		if (k != i + 1)
			printf("user hash error: %d, %s\n", i,
			       passwdptr[i].userid);
	}
	k = tocheck3->hash_head[0];
	while (k != 0) {
		gu[k - 1] += 2;
		k = tocheck3->next[k];
	}
	for (i = 0; i < MAXUSERS; i++) {
		if (gu[i] == 1)
			printf("user hash error: %d, NULL\n", k);

	}
	for (i = 0; i < UCACHE_HASHSIZE; i++) {
		k = tocheck3->hash_head[i];
		if(k < 0 || k > MAXUSERS){
			printf("e: %d %d\n", i, tocheck3->hash_head[i]);
			continue;
		}
		while (k != 0) {
			if (i != ucache_hash(passwdptr[k - 1].userid))
				printf("user hash error type 2: %d, %d, %s\n",
				       i, k, passwdptr[k - 1].userid);
			k = tocheck3->next[k];
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	initbbsinfo(&bbsinfo);
	checkutmp();
	checkuhash();
	return 0;
}
