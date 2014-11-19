#include <stdio.h>
#include <unistd.h>
#include <ythtbbs.h>

int
main(int argc, char **argv)
{
	char buf[256];
	char *obuf;
	int len;
	char c;
	int b2g = 1;

	while ((c = getopt(argc, argv, "r")) != -1) {
		switch (c) {
		case 'r':
			b2g = 0;
			break;
		default:
			fprintf(stderr, "usage: %s -r\n", argv[0]);
			fprintf(stderr,
				"\t-r: From GB to BIG5 (default: from BIG5 to GB)\n");
			return -1;
		}
	}

	conv_init();
	while (fgets(buf, 256, stdin)) {
		len = strlen(buf);
		if (b2g)
			obuf = big2gb(buf, &len, 0);
		else
			obuf = gb2big(buf, &len, 0);
		printf("%s", obuf);
	}
	return 0;
}
