#include "bbs.h"
#include "ythtbbs.h"

struct bbsinfo bbsinfo;
int
main(int argc, char *argv[])
{
	struct userec *lookupuser;
	struct userec tmpu;
	char *userid;
	char *bn;
	int uid;
	int fix = 0;

	bn=basename(argv[0]);

	while (1) {
		int c;
		c = getopt(argc, argv, "fh");
		if (c == -1)
			break;
		switch (c) {
		case 'f':
			fix = 1;
			break;
		case 'h':
			printf
			    ("%s -f [username]\n  do fix user mailbox size\n"
			     "%s [username]\n show user mailbox size\n",
			     bn, bn);
			return 0;
		case '?':
			printf
			    ("%s: Unknown argument.\nTry `%s -h' for more information.\n",
			     bn, bn);
			return 0;
		}
	}

	if (optind < argc) {
		userid = argv[optind++];
		if (optind < argc) {
			printf
			    ("%s: Too many arguments.\nTry `%s -h' for more information.\n",
			     bn, bn);
			return 0;
		}
	} else {
		printf("%s: No enough arguments!\n",bn);
		return 0;
	}
	if (initbbsinfo(&bbsinfo) < 0) {
		printf("can't init bbsinfo!\n");
		return -1;
	}
	uid = getuser(userid, &lookupuser);
	if (uid <= 0) {
		printf("can't find user %s!\n", userid);
		return -1;
	}
	printf("%s mailbox %dk\n", lookupuser->userid, lookupuser->mailsize);
	if (!fix)
		return 0;
	memcpy(&tmpu, lookupuser, sizeof (struct userec));
	tmpu.mailsize = -2;
	if (updateuserec(&tmpu, uid) < 0) {
		printf("update error!\n");
		return -1;
	} else
		printf("reset success!\n");
	return 0;
}
