#include "bbs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include "bshm.h"

#define PORT	10111

/* iconf �ĸ�ʽ: ת��վ��ַ	�Է�ת�Ű���	����ת����� */

char *iconf[] = {
	"bbs.nju.edu.cn",
	"sesa.nju.edu.cn",
	"bbs.hhu.edu.cn",
	"bbs.ustb.edu.cn",
	"bbs.sjtu.edu.cn",
	"bbs.whnet.edu.cn",
	"bbs.pku.edu.cn",
	//"fb2000.dhs.org",
	//"bbs.feeling.dhs.org",
	"sbbs.seu.edu.cn",
	"bbs.swjtu.edu.cn",
	NULL
};

int
get_mail(char *host)
{
	int fd;
	FILE *fp;
	struct sockaddr_in xs;
	struct hostent *he;
	char buf[200];
	buf[199] = 0;
//      file: �����ļ���, buf: ��ʱ����, brk: �ָ���.

	bzero((char *) &xs, sizeof (xs));
	xs.sin_family = AF_INET;
	if ((he = gethostbyname(host)) != NULL)
		bcopy(he->h_addr, (char *) &xs.sin_addr, he->h_length);
	else
		xs.sin_addr.s_addr = inet_addr(host);
	xs.sin_port = htons(PORT);
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(fd, (struct sockaddr *) &xs, sizeof (xs)) < 0) {
		close(fd);
		printf("%s: û������\n", host);
		return -1;
	}
	fp = fdopen(fd, "r+");
	if (fgets(buf, sizeof (buf), fp) == 0)
		return -3;
	if (strlen(buf) < 10) {
		fclose(fp);
		return -2;
	}
	fprintf(fp, "select * from oboard_ctl where dt < 0\n");
	fflush(stdout);
	printf("-------%s-------\n", host);
	while (fgets(buf, sizeof (buf), fp) != 0)
		printf("%s", buf);
	fclose(fp);
	return 0;
}

int
main(int argn, char **argv)
{
	int i;
	if (argn > 1)
		get_mail(argv[1]);
	else
		for (i = 0; iconf[i] != NULL; i++) {
			get_mail(iconf[i]);
		}
	return 0;
}
