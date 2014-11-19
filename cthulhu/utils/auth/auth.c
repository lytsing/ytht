#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#define die2(x) { perror (x); exit (23); }

int
auth_user (domain, np)
	char *domain, *np;
{
	int fd = open (domain, O_RDONLY), ret = 1;
	ssize_t len, i, slen = strlen (np);
	char *s;

	if (-1 == fd) return (1);
	if (-1 == (len = lseek (fd, 0, SEEK_END)) || !len ||
	 (MAP_FAILED == (s = mmap (NULL, len, PROT_READ, MAP_PRIVATE, fd, 0))))
		goto out;

	for (i=0;i<len&&ret;) {
	   if (s[i] == *np && !memcmp (s+i, np, slen)) ret = 0;
	   while (i<len&&s[i++]!='\n');
	}
	munmap (s, len);
out:
	close (fd);
	return (ret);
}

#define sockfd 0 

int
main (argc, argv)
	int argc;
	char **argv;
{
	while (1) {
		int cfd = accept (sockfd, NULL, NULL);
		ssize_t ret;
		size_t len, i;
		char buffer[0x800];
		if (-1 == cfd)
			die2 ("accept failed");
		if (-1 == (ret = read (cfd, buffer, sizeof (buffer)-1)))
			goto err;
		buffer[ret] = 0;
		len = strlen (buffer+1);
		for (i=0;i<(size_t)ret;i++)
		  if (buffer[i]=='/') {
			buffer[i] = 0;
			break;
		  }
		ret = auth_user (buffer+1, buffer+len+2);
		if (-1 == write (cfd, &ret, 1))
err:		perror ("error");
		close (cfd);
	}
	return (0);
}
