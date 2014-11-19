#include "bbs.h"
#define PASSWDFILE "../PASSBAK-05-10"

int
main(int argc, char *argv[])
{
	FILE *fp, *fw;
	struct userec rec;
	int size1 = sizeof (rec);
	time_t t1,t2=0;
	struct tm tm_t1={0,0,0,10,3,104,0,0,0};
	struct tm tm_t2={0,0,0,10,4,104,0,0,0};
	struct broad_item a;

	t1=mktime(&tm_t1);
	t2=mktime(&tm_t2);
	if ((fp = fopen(PASSWDFILE, "r")) == NULL) {
		perror("open PASSWDFILE");
		return -1;
	}
	if ((fw = fopen(BROADCAST_LIST, "w")) == NULL) {
		fclose(fp);
		perror("open BROADCASTFILE");
		return -1;
	}
	while (fread(&rec, 1, size1, fp) == size1) {
		a.inlist = 0;
		a.broaded = 0;
		if (rec.userid[0] && rec.firstlogin <= t1
		    && rec.stay >= 24 * 3600
		    && rec.userlevel & PERM_LOGINOK) {
			a.inlist = 1;
			printf("%s\n", rec.userid);
		}
		fwrite(&a, sizeof (struct broad_item), 1, fw);
	}
	fclose(fp);
	fclose(fw);
	return 0;
}
