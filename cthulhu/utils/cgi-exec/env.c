#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "config.h"

#define ENV_FILE CGI_CONFIG_DIR"environ"

extern void _put_env __P ((char *, char *, size_t, size_t));

static char *envstr;
static off_t envlen;
size_t envlinenum;

int
read_environ ()
{
	int fd = open (ENV_FILE, O_RDONLY);
	size_t i;

	if (-1 == fd) return (0);
	if (-1 == (envlen = lseek (fd, 0, SEEK_END)) ||
		(MAP_FAILED == (envstr = mmap (NULL, envlen, PROT_READ,
		MAP_SHARED, fd, 0)))) {
		close (fd);
		return (0);
	}
	for (envlinenum=i=0;i<envlen;i++) if (envstr[i]=='\n') envlinenum++;
	close (fd);
	return (1);
}

void
set_global_env ()
{
	size_t i = 0;
	while (i<envlen) {
		size_t old = i, tmp;
		while (i<envlen&&envstr[i++]!='=');
		tmp = i;
		while (i<envlen&&envstr[i]!='\n') i++;
		_put_env (envstr+old, envstr+tmp, tmp-old-1, i-tmp);
		i++;
	}
}
