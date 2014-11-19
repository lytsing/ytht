#include "bbs.h"
#define numboards 296 
int now;
int 
mail_file(char *filename, char *userid, char *title, char *sender)
{
        FILE *fp, *fp2;
        char buf[256], dir[256];
        struct fileheader header;
        int t;
        bzero(&header, sizeof (header));
        fh_setowner(&header, sender, 0);
	sprintf(buf, "mail/%c/%s/", mytoupper(userid[0]), userid);
        mkdir(buf, 0770);
        t = trycreatefile(buf, "M.%d.A", now, 100);
        if (t < 0)
                return -1;
        header.filetime = t;
        strsncpy(header.title, title, sizeof (header.title));
        fp = fopen(buf, "w");
        if (fp == 0)
                return -2;
        fp2 = fopen(filename, "r");
        if (fp2) {
                while (1) {
                        int retv;
                        retv = fread(buf, 1, sizeof (buf), fp2);
                        if (retv <= 0)
                                break;
                        fwrite(buf, 1, retv, fp);
                }
                fclose(fp2);
        }
        fclose(fp);
        setmailfile(dir, userid, ".DIR");
        append_record(dir, &header, sizeof (header));
        return 0;
}

int
mailmsg(char *userid)
{
	char file[200];
	char title[50];
	struct stat st;
	sprintf(file, "home/%c/%s/msgfile", mytoupper(userid[0]), userid);
	sprintf(title, "[May  9 22:00] ����ѶϢ����");
	if (stat(file, &st) != -1) {
		printf("mail %s\n", userid);
		mail_file(file, userid, title, userid);
	}
	return 0;
}

int
_mailmsg(struct userec *user) {
	return mailmsg(user->userid);
}

struct bbsinfo bbsinfo;

int
main()
{
	if(initbbsinfo(&bbsinfo)<0)
		return -1;
	now = time(NULL);
	new_apply_record(PASSFILE, sizeof (struct userec), (void *)_mailmsg, NULL);
	return 0;
}
