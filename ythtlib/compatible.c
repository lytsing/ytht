#include "ythtlib.h"

#ifndef HAVE_FLOCK
int
flock(int fd, int operation)
{
	struct flock fl;
	int ret;
	int cmd;

	if (operation & LOCK_NB)
		cmd = F_SETLK;
	else
		cmd = F_SETLKW;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;
	switch (operation & (~LOCK_NB)) {
	case LOCK_SH:
		fl.l_type = F_RDLCK;
		ret = fcntl(fd, cmd, &fl);
		break;
	case LOCK_EX:
		fl.l_type = F_WRLCK;
		ret = fcntl(fd, cmd, &fl);
		break;
	case LOCK_UN:
		fl.l_type = F_UNLCK;
		ret = fcntl(fd, cmd, &fl);
		break;
	default:
		ret = -1;
		errno = EINVAL;
		return ret;
	}
	if (ret == -1) {
		if (errno == EAGAIN || errno == EACCES)
			errno = EWOULDBLOCK;
	}
	return ret;
}
#endif

#ifndef HAVE_STRCASESTR
#include <stdio.h>
#include <ctype.h>

char *
strcasestr(const char *haystack, const char *needle)
{
	const char *cp1 = haystack, *cp2 = needle;
	const char *cx;
	int tstch1, tstch2;

	/*
	 * printf("looking for '%s' in '%s'\n", needle, haystack); 
	 */
	if (cp1 && cp2 && *cp1 && *cp2)
		for (cp1 = haystack, cp2 = needle; *cp1;) {
			cx = cp1;
			cp2 = needle;
			do {
				/*
				 * printf("T'%c' ", *cp1); 
				 */
				if (!*cp2) {	/* found the needle */
					/*
					 * printf("\nfound '%s' in '%s'\n", needle, cx); 
					 */
					return (char *) cx;
				}
				if (!*cp1)
					break;

				tstch1 = toupper(*cp1);
				tstch2 = toupper(*cp2);
				if (tstch1 != tstch2)
					break;
				/*
				 * printf("M'%c' ", *cp1); 
				 */
				cp1++;
				cp2++;
			}
			while (1);
			if (*cp1)
				cp1++;
		}
	/*
	 * printf("\n"); 
	 */
	if (cp1 && *cp1)
		return (char *) cp1;

	return NULL;
}

#endif

#ifndef HAVE_STRNLEN
int strnlen(const char *str, int max)
{
	char *ptr;
	ptr=memchr(str, 0, max);
	if(ptr)
		return ptr-str;
	else
		return max;
}
#endif

#ifndef HAVE_BASENAME

#ifndef FILESYSTEM_PREFIX_LEN
#define FILESYSTEM_PREFIX_LEN(f) 0
#endif

#ifndef ISSLASH
#define ISSLASH(c) ((c) == '/')
#endif

/* In general, we can't use the builtin `basename' function if available,
   since it has different meanings in different environments.
   In some environments the builtin `basename' modifies its argument.  */

char *
basename(name)
char const *name;
{
	char const *base = name += FILESYSTEM_PREFIX_LEN(name);

	for (; *name; name++)
		if (ISSLASH(*name))
			base = name + 1;

	return (char *) base;
}
#endif				//have_basename

#ifndef HAVE_NFTW

#include <dirent.h>
#include <errno.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>

/* #define NDEBUG 1 */
#include <assert.h>

/* Support for the LFS API version.  */
#ifndef FTW_NAME
# define FTW_NAME ftw
# define NFTW_NAME nftw
# define INO_T ino_t
# define STAT stat
# define LXSTAT lstat
# define XSTAT stat
# define FTW_FUNC_T __ftw_func_t
# define NFTW_FUNC_T __nftw_func_t
#endif

/* st: modified for cygwin */
//#define dirfd(x) ((x)->dd_fd)
#define __readdir readdir

#ifndef MAX
#define MAX(h,i) ((h) > (i) ? (h) : (i))
#endif

/* also changed __set_errno */

struct dir_data {
	DIR *stream;
	char *content;
};

struct known_object {
	dev_t dev;
	INO_T ino;
};

struct ftw_data {
	/* Array with pointers to open directory streams.  */
	struct dir_data **dirstreams;
	size_t actdir;
	size_t maxdir;

	/* Buffer containing name of currently processed object.  */
	char *dirbuf;
	size_t dirbufsize;

	/* Passed as fourth argument to `nftw' callback.  The `base' member
	   tracks the content of the `dirbuf'.  */
	struct FTW ftw;

	/* Flags passed to `nftw' function.  0 for `ftw'.  */
	int flags;

	/* Conversion array for flag values.  It is the identity mapping for
	   `nftw' calls, otherwise it maps the values to those know by
	   `ftw'.  */
	const int *cvt_arr;

	/* Callback function.  We always use the `nftw' form.  */
	NFTW_FUNC_T func;

	/* Device of starting point.  Needed for FTW_MOUNT.  */
	dev_t dev;

	/* Data structure for keeping fingerprints of already processed
	   object.  This is needed when not using FTW_PHYS.  */
	void *known_objects;
};

/* Internally we use the FTW_* constants used for `nftw'.  When the
   process called `ftw' we must reduce the flag to the known flags
   for `ftw'.  */
static const int nftw_arr[] = {
	FTW_F, FTW_D, FTW_DNR, FTW_NS, FTW_SL, FTW_DP, FTW_SLN
};

static const int ftw_arr[] = {
	FTW_F, FTW_D, FTW_DNR, FTW_NS, FTW_F, FTW_D, FTW_NS
};

/* Forward declarations of local functions.  */
static int ftw_dir(struct ftw_data *data, struct STAT *st);

static int
object_compare(const void *p1, const void *p2)
{
	/* We don't need a sophisticated and useful comparison.  We are only
	   interested in equality.  However, we must be careful not to
	   accidentally compare `holes' in the structure.  */
	const struct known_object *kp1 = p1, *kp2 = p2;
	int cmp1;
	cmp1 = (kp1->dev > kp2->dev) - (kp1->dev < kp2->dev);
	if (cmp1 != 0)
		return cmp1;
	return (kp1->ino > kp2->ino) - (kp1->ino < kp2->ino);
}

static inline int
add_object(struct ftw_data *data, struct STAT *st)
{
	struct known_object *newp = malloc(sizeof (struct known_object));
	if (newp == NULL)
		return -1;
	newp->dev = st->st_dev;
	newp->ino = st->st_ino;
	return tsearch(newp, &data->known_objects, object_compare) ? 0 : -1;
}

static inline int
find_object(struct ftw_data *data, struct STAT *st)
{
	struct known_object obj = { dev:st->st_dev, ino:st->st_ino };
	return tfind(&obj, &data->known_objects, object_compare) != NULL;
}

static inline int
open_dir_stream(struct ftw_data *data, struct dir_data *dirp)
{
	int result = 0;

	if (data->dirstreams[data->actdir] != NULL) {
		/* Oh, oh.  We must close this stream.  Get all remaining
		   entries and store them as a list in the `content' member of
		   the `struct dir_data' variable.  */
		size_t bufsize = 1024;
		char *buf = malloc(bufsize);

		if (buf == NULL)
			result = -1;
		else {
			DIR *st = data->dirstreams[data->actdir]->stream;
			/* st: removed 64 from dirent64 */
			struct dirent *d;
			size_t actsize = 0;

			/* st: removed 64 from __readdir64 */
			while ((d = __readdir(st)) != NULL) {
				size_t this_len = strlen(d->d_name);
				if (actsize + this_len + 2 >= bufsize) {
					char *newp;
					bufsize += MAX(1024, 2 * this_len);
					newp = realloc(buf, bufsize);
					if (newp == NULL) {
						/* No more memory.  */
						int save_err = errno;
						free(buf);
						errno = save_err;
						result = -1;
						break;
					}
					buf = newp;
				}

				memcpy(buf + actsize, d->d_name, this_len);
				buf[actsize + this_len] = 0;
				actsize += this_len + 1;
			}

			/* Terminate the list with an additional NUL byte.  */
			buf[actsize++] = '\0';

			/* Shrink the buffer to what we actually need.  */
			data->dirstreams[data->actdir]->content =
			    realloc(buf, actsize);
			if (data->dirstreams[data->actdir]->content == NULL) {
				int save_err = errno;
				free(buf);
				errno = save_err;
				result = -1;
			} else {
				closedir(st);
				data->dirstreams[data->actdir]->stream = NULL;
				data->dirstreams[data->actdir] = NULL;
			}
		}
	}

	/* Open the new stream.  */
	if (result == 0) {
		assert(data->dirstreams[data->actdir] == NULL);

		dirp->stream = opendir(data->dirbuf);
		if (dirp->stream == NULL)
			result = -1;
		else {
			dirp->content = NULL;
			data->dirstreams[data->actdir] = dirp;

			if (++data->actdir == data->maxdir)
				data->actdir = 0;
		}
	}

	return result;
}

static inline int
process_entry(struct ftw_data *data, struct dir_data *dir, const char *name,
	      size_t namlen)
{
	struct STAT st;
	int result = 0;
	int flag = 0;

	if (name[0] == '.' && (name[1] == '\0'
			       || (name[1] == '.' && name[2] == '\0')))
		/* Don't process the "." and ".." entries.  */
		return 0;

	if (data->dirbufsize < data->ftw.base + namlen + 2) {
		/* Enlarge the buffer.  */
		char *newp;

		data->dirbufsize *= 2;
		newp = realloc(data->dirbuf, data->dirbufsize);
		if (newp == NULL)
			return -1;
		data->dirbuf = newp;
	}

	memcpy(data->dirbuf + data->ftw.base, name, namlen);
	data->dirbuf[data->ftw.base + namlen] = 0;

	if (((data->flags & FTW_PHYS)
	     ? LXSTAT(data->dirbuf, &st)
	     : XSTAT(data->dirbuf, &st)) < 0) {
		if (errno != EACCES && errno != ENOENT)
			result = -1;
		else if (!(data->flags & FTW_PHYS)
			 && LXSTAT(data->dirbuf, &st) == 0
			 && S_ISLNK(st.st_mode))
			flag = FTW_SLN;
		else
			flag = FTW_NS;
	} else {
		if (S_ISDIR(st.st_mode))
			flag = FTW_D;
		else if (S_ISLNK(st.st_mode))
			flag = FTW_SL;
		else
			flag = FTW_F;
	}

	if (result == 0
	    && (flag == FTW_NS
		|| !(data->flags & FTW_MOUNT) || st.st_dev == data->dev)) {
		if (flag == FTW_D) {
			if ((data->flags & FTW_PHYS)
			    || (!find_object(data, &st)
				/* Remember the object.  */
				&& (result = add_object(data, &st)) == 0)) {
				result = ftw_dir(data, &st);

				if (result == 0 && (data->flags & FTW_CHDIR)) {
					/* Change back to current directory.  */
					int done = 0;
					if (dir->stream != NULL)
						if (fchdir(dirfd(dir->stream))
						    == 0)
							done = 1;

					if (!done) {
						if (data->ftw.base == 1) {
							if (chdir("/") < 0)
								result = -1;
						} else {
							/* Please note that we overwrite a slash.  */
							data->dirbuf[data->ftw.
								     base - 1] =
							    '\0';

							if (chdir(data->dirbuf)
							    < 0)
								result = -1;

							data->dirbuf[data->ftw.
								     base - 1] =
							    '/';
						}
					}
				}
			}
		} else
			result =
			    (*data->func) (data->dirbuf, &st,
					   data->cvt_arr[flag], &data->ftw);
	}

	return result;
}

static int
ftw_dir(struct ftw_data *data, struct STAT *st)
{
	struct dir_data dir;
	/* st: removed 64 from dirent64 */
	struct dirent *d;
	int previous_base = data->ftw.base;
	int result;
	char *startp;

	/* Open the stream for this directory.  This might require that
	   another stream has to be closed.  */
	result = open_dir_stream(data, &dir);
	if (result != 0) {
		if (errno == EACCES)
			/* We cannot read the directory.  Signal this with a special flag.  */
			result =
			    (*data->func) (data->dirbuf, st, FTW_DNR,
					   &data->ftw);

		return result;
	}

	/* First, report the directory (if not depth-first).  */
	if (!(data->flags & FTW_DEPTH)) {
		result = (*data->func) (data->dirbuf, st, FTW_D, &data->ftw);
		if (result != 0)
			return result;
	}

	/* If necessary, change to this directory.  */
	if (data->flags & FTW_CHDIR) {
		if (fchdir(dirfd(dir.stream)) < 0) {
			if (errno == ENOSYS) {
				if (chdir(data->dirbuf) < 0)
					result = -1;
			} else
				result = -1;
		}

		if (result != 0) {
			int save_err = errno;
			closedir(dir.stream);
			errno = save_err;

			if (data->actdir-- == 0)
				data->actdir = data->maxdir - 1;
			data->dirstreams[data->actdir] = NULL;

			return result;
		}
	}

	/* Next, update the `struct FTW' information.  */
	++data->ftw.level;
	startp = strchr(data->dirbuf, '\0');
	/* There always must be a directory name.  */
	assert(startp != data->dirbuf);
	if (startp[-1] != '/')
		*startp++ = '/';
	data->ftw.base = startp - data->dirbuf;

	/* st: reamoved 64 from __readdir64 */
	while (dir.stream != NULL && (d = __readdir(dir.stream)) != NULL) {
		result =
		    process_entry(data, &dir, d->d_name, strlen(d->d_name));
		if (result != 0)
			break;
	}

	if (dir.stream != NULL) {
		/* The stream is still open.  I.e., we did not need more
		   descriptors.  Simply close the stream now.  */
		int save_err = errno;

		assert(dir.content == NULL);

		closedir(dir.stream);
		errno = save_err;

		if (data->actdir-- == 0)
			data->actdir = data->maxdir - 1;
		data->dirstreams[data->actdir] = NULL;
	} else {
		int save_err;
		char *runp = dir.content;

		while (result == 0 && *runp != '\0') {
			char *endp = strchr(runp, '\0');

			result = process_entry(data, &dir, runp, endp - runp);

			runp = endp + 1;
		}

		save_err = errno;
		free(dir.content);
		errno = save_err;
	}

	/* Prepare the return, revert the `struct FTW' information.  */
	data->dirbuf[data->ftw.base - 1] = '\0';
	--data->ftw.level;
	data->ftw.base = previous_base;

	/* Finally, if we process depth-first report the directory.  */
	if (result == 0 && (data->flags & FTW_DEPTH))
		result = (*data->func) (data->dirbuf, st, FTW_DP, &data->ftw);

	return result;
}

static int
ftw_startup(const char *dir, int is_nftw, void *func, int descriptors,
	    int flags)
{
	struct ftw_data data;
	struct STAT st;
	int result = 0;
	int save_err;
	int len;
	char *cwd = NULL;
	char *cp;

	/* First make sure the parameters are reasonable.  */
	if (dir[0] == '\0') {
		errno = ENOENT;
		return -1;
	}

	if (access(dir, R_OK) != 0)
		return -1;

	data.maxdir = descriptors < 1 ? 1 : descriptors;
	data.actdir = 0;
	data.dirstreams = (struct dir_data **) alloca(data.maxdir
						      *
						      sizeof (struct dir_data
							      *));
	memset(data.dirstreams, '\0', data.maxdir * sizeof (struct dir_data *));

#ifdef PATH_MAX
	data.dirbufsize = MAX(2 * strlen(dir), PATH_MAX);
#else
	data.dirbufsize = 2 * strlen(dir);
#endif
	data.dirbuf = (char *) malloc(data.dirbufsize);
	if (data.dirbuf == NULL)
		return -1;
	len = strlen(dir);
	memcpy(data.dirbuf, dir, len);
	cp = data.dirbuf + len;
	/* Strip trailing slashes.  */
	while (cp > data.dirbuf + 1 && cp[-1] == '/')
		--cp;
	*cp = '\0';

	data.ftw.level = 0;

	/* Find basename.  */
	while (cp > data.dirbuf && cp[-1] != '/')
		--cp;
	data.ftw.base = cp - data.dirbuf;

	data.flags = flags;

	/* This assignment might seem to be strange but it is what we want.
	   The trick is that the first three arguments to the `ftw' and
	   `nftw' callback functions are equal.  Therefore we can call in
	   every case the callback using the format of the `nftw' version
	   and get the correct result since the stack layout for a function
	   call in C allows this.  */
	data.func = (NFTW_FUNC_T) func;

	/* Since we internally use the complete set of FTW_* values we need
	   to reduce the value range before calling a `ftw' callback.  */
	data.cvt_arr = is_nftw ? nftw_arr : ftw_arr;

	/* No object known so far.  */
	data.known_objects = NULL;

	/* Now go to the directory containing the initial file/directory.  */
	if ((flags & FTW_CHDIR) && data.ftw.base > 0) {
		/* GNU extension ahead.  */
		cwd = getcwd(NULL, 0);
		if (cwd == NULL)
			result = -1;
		else {
			/* Change to the directory the file is in.  In data.dirbuf
			   we have a writable copy of the file name.  Just NUL
			   terminate it for now and change the directory.  */
			if (data.ftw.base == 1)
				/* I.e., the file is in the root directory.  */
				result = chdir("/");
			else {
				char ch = data.dirbuf[data.ftw.base - 1];
				data.dirbuf[data.ftw.base - 1] = '\0';
				result = chdir(data.dirbuf);
				data.dirbuf[data.ftw.base - 1] = ch;
			}
		}
	}

	/* Get stat info for start directory.  */
	if (result == 0) {
		if (((flags & FTW_PHYS)
		     ? LXSTAT(data.dirbuf, &st)
		     : XSTAT(data.dirbuf, &st)) < 0) {
			if (errno == EACCES)
				result =
				    (*data.func) (data.dirbuf, &st, FTW_NS,
						  &data.ftw);
			else if (!(flags & FTW_PHYS)
				 && errno == ENOENT
				 && LXSTAT(dir, &st) == 0
				 && S_ISLNK(st.st_mode))
				result =
				    (*data.func) (data.dirbuf, &st,
						  data.cvt_arr[FTW_SLN],
						  &data.ftw);
			else
				/* No need to call the callback since we cannot say anything
				   about the object.  */
				result = -1;
		} else {
			if (S_ISDIR(st.st_mode)) {
				/* Remember the device of the initial directory in case
				   FTW_MOUNT is given.  */
				data.dev = st.st_dev;

				/* We know this directory now.  */
				if (!(flags & FTW_PHYS))
					result = add_object(&data, &st);

				if (result == 0)
					result = ftw_dir(&data, &st);
			} else {
				int flag = S_ISLNK(st.st_mode) ? FTW_SL : FTW_F;

				result =
				    (*data.func) (data.dirbuf, &st,
						  data.cvt_arr[flag],
						  &data.ftw);
			}
		}
	}

	/* Return to the start directory (if necessary).  */
	if (cwd != NULL) {
		int save_err = errno;
		chdir(cwd);
		free(cwd);
		errno = save_err;
	}

	/* Free all memory.  */
	save_err = errno;
	tdestroy(data.known_objects, free);
	free(data.dirbuf);
	errno = save_err;

	return result;
}

/* Entry points.  */

int
FTW_NAME(path, func, descriptors)
const char *path;
FTW_FUNC_T func;
int descriptors;
{
	return ftw_startup(path, 0, func, descriptors, 0);
}

int
NFTW_NAME(path, func, descriptors, flags)
const char *path;
NFTW_FUNC_T func;
int descriptors;
int flags;
{
	return ftw_startup(path, 1, func, descriptors, flags);
}

#endif				//have_nftw

#ifndef HAVE_VERSIONSORT

int __strverscmp(const char *, const char *);

int
versionsort(const void *a, const void *b)
{
	return __strverscmp((*(const struct dirent **) a)->d_name,
			    (*(const struct dirent **) b)->d_name);
}

#include <string.h>
#include <ctype.h>
/* states: S_N: normal, S_I: comparing integral part, S_F: comparing
           fractionnal parts, S_Z: idem but with leading Zeroes only */
#define  S_N    0x0
#define  S_I    0x4
#define  S_F    0x8
#define  S_Z    0xC
/* result_type: CMP: return diff; LEN: compare using len_diff/diff */
#define  CMP    2
#define  LEN    3
/* Compare S1 and S2 as strings holding indices/version numbers,
   returning less than, equal to or greater than zero if S1 is less than,
   equal to or greater than S2 (for more info, see the texinfo doc).
*/ int
__strverscmp(s1, s2)
const char *s1;
const char *s2;
{
	const unsigned char *p1 = (const unsigned char *) s1;
	const unsigned char *p2 = (const unsigned char *) s2;
	unsigned char c1, c2;
	int state;
	int diff;

	/* Symbol(s)    0       [1-9]   others  (padding)
	   Transition   (10) 0  (01) d  (00) x  (11) -   */
	static const unsigned int next_state[] = {
		/* state    x    d    0    - */
		/* S_N */ S_N, S_I, S_Z, S_N,
		/* S_I */ S_N, S_I, S_I, S_I,
		/* S_F */ S_N, S_F, S_F, S_F,
		/* S_Z */ S_N, S_F, S_Z, S_Z
	};

	static const int result_type[] = {
		/* state   x/x  x/d  x/0  x/-  d/x  d/d  d/0  d/-
		   0/x  0/d  0/0  0/-  -/x  -/d  -/0  -/- */

		/* S_N */ CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
		CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
		/* S_I */ CMP, -1, -1, CMP, +1, LEN, LEN, CMP,
		+1, LEN, LEN, CMP, CMP, CMP, CMP, CMP,
		/* S_F */ CMP, CMP, CMP, CMP, CMP, LEN, CMP, CMP,
		CMP, CMP, CMP, CMP, CMP, CMP, CMP, CMP,
		/* S_Z */ CMP, +1, +1, CMP, -1, CMP, CMP, CMP,
		-1, CMP, CMP, CMP
	};

	if (p1 == p2)
		return 0;

	c1 = *p1++;
	c2 = *p2++;
	/* Hint: '0' is a digit too.  */
	state = S_N | ((c1 == '0') + (isdigit(c1) != 0));

	while ((diff = c1 - c2) == 0 && c1 != '\0') {
		state = next_state[state];
		c1 = *p1++;
		c2 = *p2++;
		state |= (c1 == '0') + (isdigit(c1) != 0);
	}

	state = result_type[state << 2 | (((c2 == '0') + (isdigit(c2) != 0)))];

	switch (state) {
	case CMP:
		return diff;

	case LEN:
		while (isdigit(*p1++))
			if (!isdigit(*p2++))
				return 1;

		return isdigit(*p2) ? -1 : diff;

	default:
		return state;
	}
}

//weak_alias(__strverscmp, strverscmp);

#endif				//have_versionsort

#ifndef HAVE_MEMMEM
#ifndef SOLARIS
#include <sys/cdefs.h>
#endif

#include <sys/types.h>
#include <limits.h>
#include <string.h>

/*
 * Find the first occurrence of byte string p in byte string s.
 *
 * This implementation uses simplified Boyer-Moore algorithm (only
 * bad-character shift table is used).
 * See:
 * Boyer R.S., Moore J.S. 1977, "A fast string searching algorithm",
 * Communications of ACM. 20:762-772.
 */
void *memmem(s, slen, p, plen)
register const void *s, *p;
size_t slen, plen;
{
    register const u_char *str, *substr;
    register size_t i, max_shift, curr_shift;

    size_t shift[UCHAR_MAX + 1];

    if (!plen)
        return ((void *) s);
    if (plen > slen)
        return (NULL);

    str = (const u_char *) s;
    substr = (const u_char *) p;

    for (i = 0; i <= UCHAR_MAX; i++)
        shift[i] = plen + 1;
    for (i = 0; i < plen; i++)
        shift[substr[i]] = plen - i;

    i = 0;
    max_shift = slen - plen;
    while (i <= max_shift) {
        if (*str == *substr && !memcmp(str + 1, substr + 1, plen - 1))
            return ((void *) str);
        curr_shift = shift[str[plen]];
        str += curr_shift;
        i += curr_shift;
    }
    return (NULL);
}
#endif                          /* ! HAVE_MEMMEM */
