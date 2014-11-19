#include "bbs.h"

#define MAXFILTER 100
int
main(int argn, char **argv)
{
	char array[MAXFILTER][60], line[1024];
	int i, n = 0;
	FILE *fp;
	if (argn < 2)
		return -1;
	fp = fopen(argv[1], "r");
	if (fp == NULL)
		goto FILTERING;
	for (i = 0; i < MAXFILTER; i++) {
		if (fgets(line, 60, fp) == NULL)
			break;
		strcpy(array[i], strtrim(line));
		if (!array[i][0])
			i--;
	}
	n = i;
FILTERING:
	while (fgets(line, 1024, stdin) != NULL) {
		for (i = 0; i < n; i++) {
			if (strcasestr(line, array[i]))
				break;
		}
		if (i == n || n == 0)
			fputs(line, stdout);
	}
	fclose(fp);
	return 0;
}
