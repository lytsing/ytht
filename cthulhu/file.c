#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include "file.h"
#include "misc.h"
#include "xalloc.h"
#include "config.h"

#ifndef PAGE_SIZE
# ifdef PAGESIZE
#  define PAGE_SIZE PAGESIZE
# else
#  if (defined (__sparc__)) || (defined (__alpha__))
#   define PAGE_SIZE 0x2000
#  else
#   define PAGE_SIZE 0x1000
#  endif
# endif
#endif

int
open_file (filename, file, mtime)
	char *filename;
	file_t *file;
	time_t *mtime;
{
	int fd = open (filename, O_RDONLY);
	struct stat st;

	if (fd < 0) return (0);
	if (fstat (fd, &st) || !(file->len = st.st_size)) goto out;
	if (mtime) *mtime = st.st_mtime;
	if ((file->start = mmap (NULL, file->len, PROT_READ | PROT_WRITE,
		MAP_PRIVATE, fd, 0)) == MAP_FAILED) goto out;
	close (fd);
	return (1);
out:
	close (fd);
	return (0);
}

int
close_file (file)
	file_t *file;
{
	return (munmap (file->start, file->len));
}

int
xopen_file (filename, file)
	char *filename;
	xfile_t *file;
{
	int fd = open (filename, O_RDONLY | O_NDELAY);
	struct stat st;

	if (fd < 0) return (0);
	if (fstat (fd, &st) == -1) goto out;
	if (!(file->len = st.st_size)) goto nommap;
	if (S_ISDIR (st.st_mode)) { errno = EISDIR; goto out; }
#ifdef WANT_SENDFILE
	file->fd = fd;
#else
	if ((file->start = mmap (NULL, st.st_size, PROT_READ,
		MAP_SHARED, fd, 0)) == MAP_FAILED) goto out;
	close (fd);
#endif
nommap:
	file->mtime = st.st_mtime;
	file->hash  = hasch (filename);
	file->filename = strdup (filename);
	file->count = 0;
	if (!create_string (&file->header)) return (2);
	return (1);
out:
	close (fd);
	return (0);
}

#ifdef WANT_SENDFILE
int
xreopen_file (filename, xfile)
	char *filename;
	xfile_t *xfile;
{
	struct stat st;
	stat (filename, &st);
	if (st.st_mtime != xfile->mtime || xfile->len != st.st_size) {
		reset_string (&xfile->header);
		xfile->mtime = st.st_mtime;
		xfile->len = st.st_size;
		return (1);
	}
	return (0);
}
#else
int
xreopen_file (filename, xfile)
	char *filename;
	xfile_t *xfile;
{
	struct stat st;
	int fd;
	char *s = MAP_FAILED;

	if (stat (filename, &st) == -1 || !st.st_size) return (0);
	if (xfile->mtime == st.st_mtime && xfile->len == st.st_size)
		return (0);
	if ((xfile->len & -PAGE_SIZE) == (st.st_size & -PAGE_SIZE))
		goto ok;
	if (xfile->count > 2) return (0); /* we normally enter with count: 2 */
	if ((fd = open (filename, O_RDONLY)) == -1) return (0);
	s = mmap (NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close (fd);
	if (s == MAP_FAILED) return (0);
	munmap (xfile->start, xfile->len);
	xfile->start = s;
ok:
	reset_string (&xfile->header);
	xfile->len = st.st_size;
	xfile->mtime = st.st_mtime;
	return (1);
}
#endif

int
xclose_file (file)
	xfile_t *file;
{
	if (file->count-- > 0) return (1);
	if (file->header.len) delete_string (&file->header);
	if (file->filename) free (file->filename);
#ifdef WANT_SENDFILE
	close (file->fd);
#else
	if (file->start) munmap (file->start, file->len);
#endif
	return (0);
}
