#include "bbs.h"
struct boardheader ob;
int
main()
{
	FILE *fp1, *fp2, *fp3;
	char *ptr;
	char buf[256], fn[256];
	int cr[4];
	fp1 = fopen(".BOARDS", "r");
	while (fread(&ob, sizeof (ob), 1, fp1) != 0) {
		if (ob.clubnum == 0)
			continue;
		sprintf(fn, MY_BBS_HOME "/boards/%s/club_users", ob.filename);
		fp2 = fopen(fn, "r");
		if (fp2 == NULL)
			continue;
		while (fgets(buf, 256, fp2)) {
			ptr = strchr(buf, '\n');
			if (ptr)
				*ptr = 0;
			sprintf(fn, MY_BBS_HOME "/home/%c/%s/clubrights",
				mytoupper(buf[0]), buf);
			bzero(cr, sizeof (cr));
			fp3 = fopen(fn, "r");
			if (fp3) {
				fread(cr, sizeof (int), 4, fp3);
				fclose(fp3);
			}
			cr[ob.clubnum / (8 * sizeof (int))] |=
			    (1 << ob.clubnum %
			     (8 * sizeof (int)));
			fp3 = fopen(fn, "w");
			if (fp3) {
				fwrite(cr, sizeof(int), 4, fp3);
				fclose(fp3);
				printf("set %s %s \n", ob.filename, buf);
			}

		}
		fclose(fp2);
	}
	fclose(fp1);
	return 0;
}
