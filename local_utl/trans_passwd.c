#include "bbs.h"
#include "ythtlib.h"
#include "ythtbbs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void
upgradepasswd()
{
	FILE *fr, *fw;
	struct old_userec oldrec;
	struct userec rec;
	struct userdata data;
	struct in_addr in;
	int num = 0;
	fr = fopen(PASSFILE, "r");
	if (NULL == fr) {
		printf("Can't open passwd file for read!\n");
		exit(1);
	}
	fw = fopen(MY_BBS_HOME "/.NEWPASSWDS", "w");
	if (NULL == fw) {
		printf("Can't open passwd file for write!\n");
		fclose(fr);
		exit(2);
	}
	while (fread(&oldrec, sizeof (oldrec), 1, fr) == 1) {
		if (!oldrec.userid[0])
			continue;
		num++;
		bzero(&rec, sizeof(rec));
		bzero(&data, sizeof(data));
		strcpy(rec.userid, oldrec.userid);
		memcpy(rec.flags, oldrec.flags, 2);
		rec.firstlogin = oldrec.firstlogin;
		rec.lastlogin = oldrec.lastlogin;
		rec.lastlogout = oldrec.lastlogout;
		rec.dietime = oldrec.dietime;
		if (inet_aton(oldrec.lasthost, &in)) 
			rec.lasthost = in.s_addr;
		strcpy(rec.username, oldrec.username);
		rec.numdays = oldrec.numdays;
		if (oldrec.signature >= -1 && oldrec.signature <=100) 
			rec.signature = oldrec.signature;
		else
			rec.signature = 0;
		rec.numlogins = oldrec.numlogins;
		rec.numposts = oldrec.numposts;
		rec.stay = oldrec.stay;
		rec.userlevel = oldrec.userlevel;
		if (inet_aton(oldrec.ip, &in)) 
			rec.ip = in.s_addr;
		rec.userdefine = oldrec.userdefine;
		memcpy(rec.passwd, oldrec.passwd, OLDPASSLEN);
		strncpy(data.realmail, oldrec.realmail, 60);
		data.realmail[59] = 0;
		strncpy(data.email, oldrec.email, 60);
		data.email[59] = 0;
		strncpy(data.realname, oldrec.realname, 40);
		data.realname[39] = 0;
		strncpy(data.address, oldrec.address, 40);
		data.address[39] = 0;
		fwrite(&rec, sizeof (rec), 1, fw);
		saveuserdata(rec.userid, &data);
	}
	fclose(fr);
	fclose(fw);
	printf("total user=%d", num);
}

int
main()
{
	upgradepasswd();
	return 0;
}
