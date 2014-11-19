#include <unistd.h>
#include "xalloc.h"

#define err_msg(x) write (2, x, sizeof (x) - 1)

char *
xalloc (size)
	size_t size;
{
	char *res = (char *) malloc (size);
	if (!size) return ((char *) xalloc);
	if (NULL == res) {
		err_msg ("xalloc: Out of memory. Exiting...\n");
		exit (23);
	}
	return (res);
}

char *
xresize (old, ns)
	char *old;
	size_t ns;
{
	char *new = (char *) realloc (old, ns);
	if (NULL == new) {
		err_msg ("xresize: Out of memory. Exiting...\n");
		exit (17);
	}
	return (new);
}
