#include "bbs.h"
int
h(char *id)
{
	int n1 = 0;
	int n2 = 0;
	while (*id) {
		n1 += ((unsigned char) toupper(*id)) % 26;
		id++;
		if (!*id)
			break;
		n2 += ((unsigned char) toupper(*id)) % 26;
		id++;
	}
	n1 %= 26;
	n2 %= 26;
	return n1 * 26 + n2;
}

int d[26 * 26];

int
main()
{
	//FIX ME
	/*
	struct userec u;
	int i, j, k, fd = open(PASSFILE, O_RDONLY);
	struct bbsinfo bbsinfo;
	char *ptr = "Ô½¼×ÈýÇ§";
	if (initbbsinfo(&bbsinfo) < 0)
		return -1;
	if (fd < 0)
		printf("can't open" PASSFILE);
	bzero(d, sizeof (d));
	i = 0;
	while (read(fd, &u, sizeof (u)) > 0) {
		if (!u.userid[0])
			continue;
		i++;
		d[h(u.userid)]++;
	}
	printf("---%d---%d--%d--%d\n", i, i / 26 / 26, UCACHE_HASH_SIZE,
	       UCACHE_HASH_SIZE / 26 / 26);
	close(fd);
	for (i = 0; i < 26; i++) {
		k = 0;
		printf("i=%d ", i);
		for (j = 0; j < 26; j++) {
			k += d[i * 26 + j];
			printf("%d ", d[i * 26 + j]);
		}
		printf("%d\n", k);
	}
	printf("--- %d %d %s---\n", h(ptr), h(ptr) % 26, ptr);
	i = finduseridhash(bbsinfo.ucachehashshm->uhi, UCACHE_HASH_SIZE, ptr);
	k = getUserNum(ptr);
	printf("--- %d %d %s--\n", i, k, bbsinfo.ucacheshm->userid[k - 1]);
	for(i=0;i<UCACHE_HASH_SIZE;i++) {
		if(!strcmp(bbsinfo.ucachehashshm->uhi[i].userid, ptr)||bbsinfo.ucachehashshm->uhi[i].num==k) {
			printf("%s %d\n", bbsinfo.ucachehashshm->uhi[i].userid, i);
		}
	}
	i = insertuseridhash(bbsinfo.ucachehashshm->uhi, UCACHE_HASH_SIZE, ptr, k);
	printf("%d\n", i);
	return 0;
	*/
	return 0;
}
