#include "ythtbbs.h"
#include <sys/mman.h>

int
shouldbroadcast(int userindex)
{
	struct broad_item *pi, *tmp;
	int fd;
	struct stat st;

	if (userindex < 1)
		return 0;
	fd = open(BROADCAST_LIST, O_RDWR);
	if (fd < 0) {
		//errlog("open file!");
		return 0;
	}
	if (fstat(fd, &st) < 0
	    || userindex >= st.st_size / sizeof (struct broad_item)) {
		close(fd);
//		errlog("stat file!");
		return 0;
	}
	pi = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if ((void *) -1 == pi) {
		errlog("mmap file!");
		return 0;
	}
	tmp = pi;
	pi += userindex - 1;
	if (!pi->inlist || pi->broaded) {
		munmap(tmp, st.st_size);
		return 0;
	}
	pi->broaded = 1;
	munmap(tmp, st.st_size);
	return 1;
}
