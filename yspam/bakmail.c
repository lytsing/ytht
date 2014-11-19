#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "bbs.h"
//#define DEBUG 1

int str_decode(register unsigned char *dst, register unsigned char *src);
int
main(int argc, char **argv)
{
	char *user;
	char *sender;
	char buffer[1024];
	char *message = malloc(1);
	long len = 1;
	char title[1024];
	char dtitle[1024];
	char filename[1024];
	char spam = 'N';
	struct yspam_ctx *yspam_ctx;
	unsigned long long mailid;
	int type;
	int ret;
	int fd;

	if (argc != 3)
		exit(-1);
	user = argv[1];
	sender = argv[2];
	chdir(MY_BBS_HOME "/maillog");
	/* read in the message from stdin */
	message[0] = 0;
	title[0] = 0;
	while (fgets(buffer, sizeof (buffer), stdin) != NULL) {
		if (!strncmp("Subject: ", buffer, 9)) {
			strncpy(title, buffer+9, sizeof(title));
			title[sizeof(title) - 1] = 0;
		}
		if (!strncmp("X-Bogosity: ", buffer, 12)) {
			spam = buffer[12];
		}
		len += strlen(buffer);
		message = realloc(message, len);
		if (message == NULL) {
			exit(-1);
		}
		strcat(message, buffer);
	}
	if (spam == 'Y')
		type = 2;
	else
		type = 1;
	yspam_ctx = yspam_init("127.0.0.1");
	str_decode(dtitle, title);
	ret = yspam_newmail(yspam_ctx, user, dtitle, type, sender, &mailid);
	if (ret < 0)
		sprintf(buffer, "0\n");
	else {
		sprintf(filename, MY_BBS_HOME"/maillog/%llu", mailid);
		fd = open(filename, O_WRONLY | O_CREAT, 0644);
		write(fd, message, strlen(message));
		close(fd);
		sprintf(buffer, "%llu\n", mailid);
	}
	write(STDOUT_FILENO, buffer, strlen(buffer));
	write(STDOUT_FILENO, message, strlen(message)); 
	return 0;
}
