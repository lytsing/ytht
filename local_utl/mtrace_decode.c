#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char *argv[])
{
	FILE *fp;
	char buf[256], *ptr;
	int i, j;
	fp = popen("ldd /home/bbs/bin/bbs|grep libc", "r");
	if (!fp) {
		printf("Error!\n");
		return -1;
	}
	fgets(buf, sizeof (buf), fp);
	pclose(fp);
	ptr = strchr(buf, '(');
	if (!ptr) {
		printf("ldd error!\n");
		return -1;
	}
	strtok(ptr + 1, ")");
	i = strtol(ptr + 1, NULL, 16);
	while (fgets(buf, sizeof (buf), stdin)) {
		fputs(buf, stdout);
		ptr = strrchr(buf, 'x');
		if (!ptr || ptr == buf || '0' != *(ptr - 1))
			continue;
		j = strtol(ptr - 1, NULL, 16);
		snprintf(buf, sizeof (buf),
			 "addr2line -e /usr/lib/debug/libc.so.6 0x%x", j - i);
		system(buf);
	}
	return 0;
}
