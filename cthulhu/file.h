#ifndef FILE_H
#define FILE_H

#include <time.h>
#include "string.h"

typedef struct {
	char *start;
	ssize_t len;
} file_t;

typedef struct {
	char     *filename;
#ifdef WANT_SENDFILE
	int      fd;
#else
	char     *start;
#endif
	time_t   mtime;
	size_t   len;
	long     hash;
	long     count;
	string_t header;
} xfile_t;

extern int open_file __P ((char *, file_t *, time_t *));
extern int close_file __P ((file_t *));
extern int xopen_file __P ((char *, xfile_t *));
extern int xclose_file __P ((xfile_t *));
extern int xreopen_file __P ((char *, xfile_t *));

#endif /* FILE_H */
