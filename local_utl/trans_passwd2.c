#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "bbs.h"
#include "ythtbbs.h"
struct olduserec {			/* Structure used to hold information in */
	char userid[IDLEN + 2];	/* PASSFILE */
	char flags[2];
	time_t firstlogin;
	time_t lastlogin;
	time_t lastlogout;
	time_t dietime;
	unsigned long int lasthost;
	char username[NAMELEN];
	unsigned short numdays;	//曾经登录的天数
	short signature;         //in kilobytes
	unsigned int numlogins;
	unsigned int numposts;
	time_t stay;
	unsigned userlevel;
	unsigned long int ip;
	unsigned int userdefine;
	char passwd[MD5LEN];	// MD5PASSLEN = 16
	int salt;		//salt == 0 means des; salt!=0 means md5
	time_t kickout;
};

void
upgradepasswd()
{
	FILE *fr, *fw;
	struct olduserec oldrec;
	struct userec rec;
	char buf[20];
	fr = fopen(PASSFILE, "r");
	if (NULL == fr) {
		printf("Can't open passwd file for read!\n");
		exit(1);
	}
	fw = fopen("/home/bbs/.NEWPASSWDS", "w");
	if (NULL == fw) {
		printf("Can't open passwd file for write!\n");
		fclose(fr);
		exit(2);
	}
	while (fread(&oldrec, sizeof (oldrec), 1, fr) == 1) {
		memcpy(&rec, &oldrec, sizeof(oldrec));
		rec.inprison = 0;
		rec.nouse1 = 0;
		rec.nouse[0] = rec.nouse[1] = rec.nouse[2] = 0;
		rec.mailsize = 0;
		if (oldrec.dietime) {
			rec.dieday = 1;
		} else {
			rec.dieday = 0;
		}
		if (oldrec.signature) {
			sprintf(buf, "%d", oldrec.signature);
			saveuservalue(oldrec.userid, "signature", buf);
		}
		fwrite(&rec, sizeof (rec), 1, fw);
	}
	fclose(fr);
	fclose(fw);
}

int
main()
{
	upgradepasswd();
	return 0;
}
