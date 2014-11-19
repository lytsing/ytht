#include "bbs.h"
#define PASSWDFILE "../PASSBAK-20031212"


#include <time.h>

time_t getregtime(char *id,char *file)
{
	char buf[256];
	sethomefile(buf,id,file);
	return file_time(buf);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	struct userec rec;
	int size1 = sizeof (rec);
	time_t t1,t2,t3=0;
	struct tm tm_t1={0,0,0,9,10,103,0,0,0};
	struct tm tm_t2={0,0,0,12,10,103,0,0,0};

	t1=mktime(&tm_t1);
	t2=mktime(&tm_t2);
	if ((fp = fopen(PASSWDFILE, "r")) == NULL) {
		perror("open PASSWDFILE");
		return -1;
	}

	while (fread(&rec, 1, size1, fp) == size1){
		if(rec.firstlogin >= t2)
			continue;
		if(rec.stay < 24*3600)
			continue;
		if(rec.userlevel & PERM_LOGINOK )
			t3=1;
		if(!t3)	
			t3=getregtime(rec.userid,"register");
		if(!t3)
			t3=getregtime(rec.userid,"register.old");
		if(!t3){
			printf("%s noreg?\n",rec.userid);
			continue;
		}
		printf("%s\n",rec.userid);
	}
	fclose(fp);
	return 0;
}
