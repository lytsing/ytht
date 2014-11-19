#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "bbs.h"
#include "ythtlib.h"
#include "ythtbbs.h"
#include "errno.h"

struct fileheader *data;

int
main(int argc, char *argv[])
{
	FILE *dr;
	int file, n;
	char *ptr;
	int i;
	int now=time(NULL);

	if (argc < 2) {
		printf("no input file!\n");
		exit(1);
	}
	n = file_size(argv[1]) / sizeof (struct fileheader);
	if (n == 0) {
		printf("no context!\n");
		exit(2);
	}
	data = malloc(n * sizeof (struct fileheader));
	if (NULL == data) {
		printf("out of memory\n");
		exit(3);
	}
	dr = fopen(argv[1], "r");
	if (NULL == dr) {
		printf("can't open file to read\n");
		exit(4);
	}
	fread(data, sizeof (struct fileheader), n, dr);
	file = open("tmpfile", O_CREAT | O_TRUNC | O_WRONLY, 0770);
	if (file < 0) {
		printf("can't open file to write\n");
		exit(5);
	}
	for (i = 0; i<n; i++) {
		if (n-i>8000)
			continue;
		if (data[i].filetime > now-86400*5) {
			ptr = strrchr(data[i].title, '-');
			if (ptr) 
				*ptr = 0;
			write(file, &(data[i]), sizeof(struct fileheader));
		}
	}
	if (close(file))
		printf("close error=%d\n", errno);
	return 0;
}
