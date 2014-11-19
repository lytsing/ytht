#ifndef MISC_H
#define MISC_H

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "string.h"
#include "config.h"
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

/*extern int __date __P ((char *, time_t));*/
#ifndef FUNNY_STRING
extern int __date __P ((string_t *, time_t));
#endif
extern long hasch __P ((char *));
extern char *get_mime_by_extension __P ((char *));
extern int init_mime __P ((void));
extern int reinit_mime __P ((void));
extern int casecmp __P ((const char *, const char *, size_t));
extern int do_listen __P ((void));
extern int wildmat __P ((char *, char *));
extern void sigchld __P ((int));
extern int select_loop __P ((int));

typedef struct {
	const char  *str;
	size_t len;
} entry_t;

#endif /* MISC_H */
