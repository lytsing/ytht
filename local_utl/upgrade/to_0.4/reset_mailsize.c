#include <stdio.h>
#include <time.h>

struct userec {			/* Structure used to hold information in */
	char userid[12 + 2];	/* PASSFILE */
	char flags[2];
	time_t firstlogin;
	time_t lastlogin;
	time_t lastlogout;
	unsigned char dieday:3, inprison:1, nouse1:4;
	char nouse[3];
	unsigned long int lasthost;
	char username[40];
	unsigned short numdays;	//曾经登录的天数
	short mailsize;         //in kilobytes
	unsigned int numlogins;
	unsigned int numposts;
	time_t stay;
	unsigned userlevel;
	unsigned long int ip;
	unsigned int userdefine;
	char passwd[16];	// MD5PASSLEN = 16
	int salt;		//salt == 0 means des; salt!=0 means md5
	time_t kickout;
};

main()
{
	FILE *fp;
	struct userec u;
	fp = fopen("/home/bbs/.PASSWDS", "r+");
	while (fread(&u, sizeof (u), 1, fp) != 0) {
		fseek(fp, -sizeof(u), SEEK_CUR);
		u.mailsize = -2;
		fwrite(&u, sizeof(u), 1, fp);
	}
}
