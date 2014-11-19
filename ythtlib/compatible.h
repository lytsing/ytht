#ifndef YTHT_COMPATIBLE_H
#define YTHT_COMPATIBLE_H

#include "sys_config.h"
#ifndef HAVE_FLOCK
#include <errno.h>
#define LOCK_SH 1		/* Shared lock.  */
#define LOCK_EX 2		/* Exclusive lock.  */
#define LOCK_NB 4		/* Nonblocking */
#define LOCK_UN 8		/* Unlock.  */
int flock(int fd, int operation);
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#ifndef HAVE_STRCASESTR
char *strcasestr(const char *haystack, const char *needle);
#endif

#ifndef HAVE_STRNLEN
int strnlen(const char *str, int max);
#endif

#ifndef HAVE_BASENAME
/* When to make backup files. */
enum backup_type {
	/* Never make backups. */
	none,

	/* Make simple backups of every file. */
	simple,

	/* Make numbered backups of files that already have numbered backups,
	   and simple backups of the others. */
	numbered_existing,

	/* Make numbered backups of every file. */
	numbered
};

extern enum backup_type backup_type;
extern char const *simple_backup_suffix;

#ifndef __BACKUPFILE_P
# if defined __STDC__ || __GNUC__
#  define __BACKUPFILE_P(args) args
# else
#  define __BACKUPFILE_P(args) ()
# endif
#endif

char *basename __BACKUPFILE_P((char const *));
char *find_backup_file_name __BACKUPFILE_P((char const *));
enum backup_type get_version __BACKUPFILE_P((char const *));
void addext __BACKUPFILE_P((char *, char const *, int));

#endif				//have_basename

#ifndef HAVE_NFTW

#include <sys/types.h>
#include <sys/stat.h>

#ifndef __USE_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED
#endif

/* Values for the FLAG argument to the user function passed to `ftw'
   and 'nftw'.  */
    enum {
	FTW_F,			/* Regular file.  */
#define FTW_F	 FTW_F
	FTW_D,			/* Directory.  */
#define FTW_D	 FTW_D
	FTW_DNR,		/* Unreadable directory.  */
#define FTW_DNR	 FTW_DNR
	FTW_NS,			/* Unstatable file.  */
#define FTW_NS	 FTW_NS

#if defined __USE_BSD || defined __USE_XOPEN_EXTENDED

	FTW_SL,			/* Symbolic link.  */
# define FTW_SL	 FTW_SL
#endif

#ifdef __USE_XOPEN_EXTENDED
/* These flags are only passed from the `nftw' function.  */
	FTW_DP,			/* Directory, all subdirs have been visited. */
# define FTW_DP	 FTW_DP
	FTW_SLN			/* Symbolic link naming non-existing file.  */
# define FTW_SLN FTW_SLN
#endif				/* extended X/Open */
};

#ifdef __USE_XOPEN_EXTENDED
/* Flags for fourth argument of `nftw'.  */
enum {
	FTW_PHYS = 1,		/* Perform physical walk, ignore symlinks.  */
# define FTW_PHYS	FTW_PHYS
	FTW_MOUNT = 2,		/* Report only files on same file system as the
				   argument.  */
# define FTW_MOUNT	FTW_MOUNT
	FTW_CHDIR = 4,		/* Change to current directory while processing it.  */
# define FTW_CHDIR	FTW_CHDIR
	FTW_DEPTH = 8		/* Report files in directory before directory itself. */
# define FTW_DEPTH	FTW_DEPTH
};

/* Structure used for fourth argument to callback function for `nftw'.  */
struct FTW {
	int base;
	int level;
};
#endif				/* extended X/Open */

/* Convenient types for callback functions.  */
typedef int (*__ftw_func_t) (__const char *__filename,
			     __const struct stat * __status, int __flag);
#ifdef __USE_LARGEFILE64
typedef int (*__ftw64_func_t) (__const char *__filename,
			       __const struct stat64 * __status, int __flag);
#endif
#ifdef __USE_XOPEN_EXTENDED
typedef int (*__nftw_func_t) (__const char *__filename,
			      __const struct stat * __status, int __flag,
			      struct FTW * __info);
# ifdef __USE_LARGEFILE64
typedef int (*__nftw64_func_t) (__const char *__filename,
				__const struct stat64 * __status,
				int __flag, struct FTW * __info);
# endif
#endif

/* Call a function on every element in a directory tree.  */
#ifndef __USE_FILE_OFFSET64
extern int ftw(__const char *__dir, __ftw_func_t __func, int __descriptors);
#else
# ifdef __REDIRECT
extern int __REDIRECT(ftw, (__const char *__dir, __ftw_func_t __func,
			    int __descriptors), ftw64);
# else
#  define ftw ftw64
# endif
#endif
#ifdef __USE_LARGEFILE64
extern int ftw64(__const char *__dir, __ftw64_func_t __func, int __descriptors);
#endif

#ifdef __USE_XOPEN_EXTENDED
/* Call a function on every element in a directory tree.  FLAG allows
   to specify the behaviour more detailed.  */
# ifndef __USE_FILE_OFFSET64
extern int nftw(__const char *__dir, __nftw_func_t __func, int __descriptors,
		int __flag);
# else
#  ifdef __REDIRECT
extern int __REDIRECT(nftw, (__const char *__dir, __nftw_func_t __func,
			     int __descriptors, int __flag), nftw64);
#  else
#   define nftw nftw64
#  endif
# endif
# ifdef __USE_LARGEFILE64
extern int nftw64(__const char *__dir, __nftw64_func_t __func,
		  int __descriptors, int __flag);
# endif
#endif

#else				//have_nftw
#include <ftw.h>
#endif
#endif
