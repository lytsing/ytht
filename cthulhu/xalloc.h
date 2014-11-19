#ifndef XALLOC_H
#define XALLOC_H

#include <sys/types.h>
#include <stdlib.h>
#include "config.h"

extern char *xalloc __P ((size_t));
extern char *xresize __P ((char *, size_t));

#define xmalloc(x) (x *) xalloc (sizeof (x))
/*#define xresize realloc*/
#define xfree(a,b) if (a) free (a)

#endif /* XALLOC_H */
