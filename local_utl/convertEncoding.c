#include "ythtlib.h"

void
insertit(char *gbbuf, char *bigbuf, int len)
{
	while (len > 0) {
		len--;
		if (gbbuf[len] != '\\' && bigbuf[len] == '\\')
			memmove(bigbuf + len + 1, bigbuf + len,
				strlen(bigbuf + len) + 1);
	}
}

void
removeit(char *bigbuf, char *gbbuf, int len)
{
	while (*bigbuf) {
		if (*bigbuf == '\\' && *gbbuf != '\\') {
			memmove(gbbuf + 1, gbbuf + 2, strlen(gbbuf) + 1);
			bigbuf++;
		}
		bigbuf++;
		gbbuf++;
	}
}

int
main(int argn, char **argv)
{
	char buf[2048], buf2[2048 * 2];
	int n, n0;
	int sourcemode = 0;
	int direction = 0; //gb2big
	for(n=1;n<argn;n++) {
		switch(argv[n][0]) {
			case 's':
				sourcemode = 1;
				break;
			case 'g':
				direction = 1;
				break;
		}
	}
	
	conv_init();
	while (fgets(buf, sizeof (buf), stdin)) {
		strcpy(buf2, buf);
		n = strlen(buf);
		n0 = n;
		if (direction ==0) {
			gb2big(buf2, &n, 0);
			if(sourcemode)
				insertit(buf, buf2, n0);
		} else {
			big2gb(buf2, &n, 0);
			if(sourcemode)
				removeit(buf, buf2, n0);
		}
		printf("%s", buf2);
	}
	return 0;
}
