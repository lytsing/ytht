#include "bbs.h"
struct boardheader ob;
int
main()
{
        FILE *fp1, *fp2;
        time_t now;
	char buf[10];
	int i=1;
        time(&now);
        fp1 = fopen(".BOARDS", "r");
        fp2 = fopen(".NEWBOARDS", "w");
        while (fread(&ob, sizeof (ob), 1, fp1) != 0) {
		ob.flag &= ~CLOSECLUB_FLAG;
                if (ob.clubnum != 0) {
                	if (ob.flag & CLUBLEVEL_FLAG) {   //OPEN CLUB
				ob.clubnum = 0;
				ob.flag |= CLUBLEVEL_FLAG;
                	} else {
				ob.flag |= CLOSECLUB_FLAG;
				printf("%s 为 closeclub，是否转换为半开放closeclub?", ob.filename);
				scanf("%s", buf);
				printf("\n");
				if (buf[0]=='y' || buf[0]=='Y') {
					ob.clubnum = 0;
					ob.flag &= ~CLUBLEVEL_FLAG;
				}
				else {
					ob.clubnum = i;
					i++;
					ob.flag |= CLUBLEVEL_FLAG;
				}
			}
		}
		fwrite(&ob, sizeof(ob), 1, fp2);
        }
        fclose(fp1);
        fclose(fp2);
        return 0;
}

