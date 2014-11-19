#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include "szm.h"
int
main(int argc, char *argv[])
{
	struct szm_ctx *ctx;
	void *out;
	int size;
	int fd;
	struct stat st;
	char *in;
	int ret;

	if (argc == 1)
		ctx = szm_init("127.0.0.1");
	else
		ctx = szm_init(argv[1]);
	fd = open("in.bmp", O_RDONLY);
	stat("in.bmp", &st);
	in = malloc(st.st_size);
	read(fd, in, st.st_size);
	close(fd);
	ret = szm_box_bmp(ctx, in, st.st_size, 400, &out, &size);
	if (ret < 0) {
		printf("err code is %d\n", ret);
		return 0;
	}
	printf("%d\n", size);
	fd = open("out.jpg", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	write(fd, out, size);
	close(fd);
	szm_fini(ctx);
	return 0;
}
