#include "bbs.h"

struct oldfriend {
	char id[13];
	char exp[15];
};

struct newfriend {
	char id[13];
	char exp[40];
};

int
report()
{
	return 0;
}

int
transfer(uid)
char *uid;
{
	struct oldfriend fh;
	struct newfriend nfh;
	char fname[80];
	char dname[80];
	char genbuf[120];
	FILE *fp, *fp2;

	memset(&fh, 0, sizeof (struct oldfriend));
	memset(&nfh, 0, sizeof (struct newfriend));

	sprintf(fname, MY_BBS_HOME "/home/%c/%s/friends", toupper(uid[0]), uid);
	sprintf(dname, MY_BBS_HOME "/home/%c/%s/friends.new", toupper(uid[0]),
		uid);

	if ((fp = fopen(fname, "rb")) == NULL) {
		return 0;
	}
	if ((fp2 = fopen(dname, "wb")) == NULL) {
		return 0;
	}
	while (1) {
		if (fread(&fh, sizeof (fh), 1, fp) <= 0)
			break;
		strcpy(nfh.id, fh.id);
		strcpy(nfh.exp, fh.exp);
		append_record(dname, &nfh, sizeof (nfh));
	}
	fclose(fp);
	fclose(fp2);
	sprintf(genbuf, "chown bbs.bbs %s", dname);
	system(genbuf);
	sprintf(genbuf, "mv -f %s %s", dname, fname);
	system(genbuf);
	return 1;
}

int
main()
{
	FILE *rec;
	int i = 0;
	struct userec user;

	rec = fopen(PASSFILE, "rb");

	printf("\033[1;31;5mFriends Records Transfering to FB2.63M...\033[m");
	while (1) {
		if (fread(&user, sizeof (user), 1, rec) <= 0)
			break;
		if (user.numlogins <= 0)
			continue;
		if (transfer(user.userid) == 1) {
			printf("\033[1m%.12s\033[36m Transferred\033[m\n", user.userid);
		} else
			printf("\033[1m%.12s\033[34m No overrides File...\033[m\n",
			       user.userid);

		i++;
	}
	printf("\n\033[1m%d \033[32mFriends Records Tranferred to FB2.63M...\033[m\n",
	       i);
	fclose(rec);
	return 0;
}
