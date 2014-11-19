#include <ght_hash_table.h>
#include <bbs.h>
main()
{
	ght_hash_table_t *tbl;
	int i, n = 0;
	char userid[20];
	struct userec x;
	struct {
		int n;
		struct userec x;
	} *nx, *nx1;
	FILE *fp = fopen(PASSFILE, "rt");
	if (!fp) {
		printf("can't open");
		return 0;
	}
	tbl = ght_create(150000);
	while (fread(&x, sizeof (x), 1, fp) == 1) {
		n++;
		if (!x.userid[0])
			continue;
		for (i = 0; x.userid[i]; i++) {
			userid[i] = toupper(x.userid[i]);
		}
		userid[i] = 0;
		if ((nx = ght_get(tbl, strlen(userid), userid))) {
			printf("DUP! userid %s\n\t%d\t%s\t",
			       userid, nx->n, Ctime(nx->x.firstlogin));
			printf("%s\n", Ctime(nx->x.lastlogin));
			printf("\t%d\t%s\t", n, Ctime(x.firstlogin));
			printf("%s\n", Ctime(x.lastlogin));
			continue;
		}
		nx = malloc(sizeof (*nx));
		nx->n = n;
		nx->x = x;
		ght_insert(tbl, nx, strlen(userid), userid);
	}
}
