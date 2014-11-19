//by zhch
#include "bbs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>

#define PORT	10111
char fromhost[80];

/* 转出的版面 */

char *oboards[] = {
	//"BBSDev",
	//"Mud_Builder",
	//"Mud",
	//"mechanics",
	//"Linux",
	//"Unix",
	//"AdvancedEdu",
	//"JAVA",
	//"programming",
	//"VB",
	"PieBridge",
	"nothing",
	"AdvancedEdu",
	NULL
};

char *eboards[] = {
	"sex",
	"joke",
	"PieBridge",
	"love",
	"Movie",
	"girl",
	"CollegeForum",
	"RomanticNovel",
	"Constellation",
	"Emprise",
	"TV",
	"First",
	"HumanRights",
	NULL
};

/* 允许访问的地址, '*' 代表所有地址 */

char *allow_hosts[] = {
	"64.55.183.11",		//mitbbs.com 2
	"220.249.10.10",	//lepton company, just 4 test
	NULL
};

char *allow_extra_hosts[] = {
	"127.0.0.1",		//for test
	"210.51.13.89",
	"211.94.188.181",
	NULL
};

/* 封禁的地址, 暂不支持通配符, 请用数字IP地址 */

char *deny_hosts[] = {
	"12.34.56.78",
	"10.0.0.1",
	NULL
};

int
valid_board(char *board)
{
	int i;
	for (i = 0; oboards[i] != NULL && i < 999; i++)
		if (!strcmp(oboards[i], board))
			return 1;
	return 0;
}

int
extra_board(char *board)
{
	int i;
	for (i = 0; eboards[i] != NULL && i < 999; i++)
		if (!strcmp(eboards[i], board))
			return 1;
	return 0;
}

int
valid_host(char *rhost)
{
	int i;
	for (i = 0; allow_hosts[i] != NULL && i < 999; i++)
		if (!strcmp(allow_hosts[i], rhost)
		    || !strcmp(allow_hosts[i], "*"))
			goto N;
	return 0;
      N:for (i = 0; deny_hosts[i] != NULL && i < 999; i++)
		if (!strcmp(deny_hosts[i], rhost))
			return 0;
	return 1;
}

int
extra_host(char *rhost)
{
	int i;
	for (i = 0; allow_extra_hosts[i] != NULL && i < 999; i++)
		if (!strcmp(allow_extra_hosts[i], rhost))
			return 1;
	return 0;
}

void
do_log(char *fmt, ...)
{
	FILE *fp;
	va_list ap;
	char cmd[256];
	time_t t = time(0);
	va_start(ap, fmt);
	vsnprintf(cmd, 255, fmt, ap);
	va_end(ap);
	fp = fopen("bbsoutd.log", "a");
	fprintf(fp, "%24.24s %-24s %s\n", ctime(&t), fromhost, cmd);
	fclose(fp);
}

int do_send(char *rhost);

int
main()
{
	static struct sockaddr_in xs;
	int fd, fd2, i;
	chdir(MY_BBS_HOME);
	if (fork())
		exit(0);
	for (i = 0; i <= getdtablesize(); i++)
		close(i);
	for (i = 1; i <= NSIG; i++)
		signal(i, SIG_IGN);
	xs.sin_family = AF_INET;
	xs.sin_port = htons(PORT);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	i = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof (i));
	if (bind(fd, (struct sockaddr *) &xs, sizeof xs) < 0)
		exit(0);
	if (listen(fd, 4) < 0)
		exit(0);
	umask((mode_t) 022);
	i = sizeof xs;
	while (1)
		if ((fd2 = accept(fd, (struct sockaddr *) &xs, &i)) > 0) {
			if (fork() == 0)
				break;
			close(fd2);
		}
	dup2(fd2, 0);
	close(fd2);
	dup2(0, 1);
	strcpy(fromhost, (char *) inet_ntoa(xs.sin_addr));
	do_send(fromhost);
	do_log("LOGOUT");
	exit(0);
}

int
do_send(char *rhost)
{
	FILE *fp, *fp2;
	struct fileheader x;
	int last, r, dt, len;
	char oboard[100], brk[100], dir[80], file[80], buf[256], *attach;
	int showextra = 0;
	srand(time(0) + getpid());
	do_log("LOGIN");
	if (extra_host(rhost))
		showextra = 1;
	else if (!valid_host(rhost))
		return printf("access denied.\n");
	sprintf(brk, "=====random_break_line_%d=====\n", rand());
	printf("%s", brk);
	fflush(stdout);
	if (fgets(buf, 80, stdin) <= 0)
		return -1;
	do_log("CMD: %s", buf);
	r = sscanf(buf, "select * from %s where dt < %d", oboard, &dt);
	if (r < 2)
		return printf("Usage: select * from board where dt < time\n");
	if (!strcmp(oboard, "oboard_ctl")) {
		int i;
		for (i = 0; oboards[i] != 0 && i < 999; i++)
			printf("%s\n", oboards[i]);
		do_log("boards list");
		return 0;
	}

	if (!showextra && !valid_board(oboard))
		return printf("bad boardname\n");

	if (showextra && !extra_board(oboard))
		return printf("bad boardname\n");

	if (dt > 86400 * 7)
		dt = 86400 * 7;	//不转过期文章。
	last = time(0) - dt;
	sprintf(dir, "boards/%s/.DIR", oboard);
	fp = fopen(dir, "r");
	if (!fp)
		return -1;
	while (1) {
		int t, lines = 0;
		if (fread(&x, sizeof (x), 1, fp) <= 0)
			break;
		t = x.filetime;
		if (t <= last || (!(x.accessed & FH_INND) && !showextra))
			continue;
		if (strstr(x.owner, "."))
			continue;	//非站内信件，不转。
		printf("%s\n", x.title);
		printf("%s.bbs@%s\n", fh2owner(&x), MY_BBS_DOMAIN);
		if (showextra) {
			printf("fileid: %d\n", x.filetime);
			printf("threadid: %d\n", x.thread);
			printf("size: %d\n", bytenum(x.sizebyte));
			printf("star: %d\n", x.staravg50 / 50);
			printf("vote: %d\n", x.hasvoted);
		}
		sprintf(file, "boards/%s/%s", oboard, fh2fname(&x));
		fp2 = fopen(file, "r");
		while (fp2) {
			if (fgets(buf, sizeof (buf), fp2) <= 0) {
				fclose(fp2);
				break;
			}
			if (lines == 0 && !strncmp(buf, "发信人", 6))
				sprintf(buf, "发信人: %s.bbs@%s, 原信区: %s\n",
					fh2owner(&x), MY_BBS_DOMAIN, oboard);
			lines++;
			if (NULL !=
			    (attach = checkbinaryattach(buf, fp2, &len))) {
				if (showextra)
					fseek(fp, len, SEEK_CUR);
				else
					uuencode(fp2, stdout, len, attach);
				continue;
			}
			printf("%s", buf);
		}
		printf("\n%s", brk);
		fflush(stdout);
	}
	fclose(fp);
	return 0;
}
