/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu
    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
                        Peng Piaw Foong, ppfoong@csie.ncu.edu.tw
    
    Copyright (C) 1999, KCN,Zhou Lin, kcn@cic.tsinghua.edu.cn

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "bbs.h"
#include "bbstelnet.h"
#include <sys/mman.h>

#define BUFSIZE (8192)

char bigbuf[10240];
int numtowrite;

long
get_num_records(filename, size)
char *filename;
int size;
{
	struct stat st;

	if (stat(filename, &st) == -1)
		return 0;
	return (st.st_size / size);
}

int
apply_record(filename, fptr, size)
char *filename;
int (*fptr) (void *);
int size;
{
	char *buf;
	int fd, sizeread, n, i;

	if ((buf = malloc(size * NUMBUFFER)) == NULL) {
		errlog("too big %s", filename);
		return -1;
	}
	if ((fd = open(filename, O_RDONLY, 0)) == -1) {
		errlog("can't open file %s", filename);
		free(buf);
		return -1;
	}
	while ((sizeread = read(fd, buf, size * NUMBUFFER)) > 0) {
		n = sizeread / size;
		for (i = 0; i < n; i++) {
			if ((*fptr) (buf + i * size) == QUIT) {
				close(fd);
				free(buf);
				return QUIT;
			}
		}
		if (sizeread % size != 0) {
			close(fd);
			free(buf);
			errlog("sizeread: %d size: %d", sizeread, size);
			return -1;
		}
	}
	close(fd);
	free(buf);
	return 0;
}

int
get_record(filename, rptr, size, id)
char *filename;
void *rptr;
int size, id;
{
	int fd;

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -2;
	if (lseek(fd, size * (id - 1), SEEK_SET) == -1) {
		close(fd);
		return -3;
	}
	if (read(fd, rptr, size) != size) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int
get_records(filename, rptr, size, id, number)
char *filename;
void *rptr;
int size, id, number;
{
	int fd;
	int n;

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;
	if (lseek(fd, size * (id - 1), SEEK_SET) == -1) {
		close(fd);
		return 0;
	}
	if ((n = read(fd, rptr, size * number)) == -1) {
		close(fd);
		return -1;
	}
	close(fd);
	return (n / size);
}

int
get_records_extra(char *filename, char *rptr, int size, int id, int number,
		  int filetotal, char *extrabuf, int nextra)
{
	int n;
	if (!nextra)
		return get_records(filename, rptr, size, id, number);
	if (id < filetotal) {
		n = get_records(filename, rptr, size, id, number);
		if (n < 0)
			return -1;
		nextra = min(number - n, nextra);
		memcpy(rptr + n * size, extrabuf, nextra * size);
		return n + nextra;
	}
	n = id - filetotal;
	if (n >= nextra)
		return 0;
	nextra = nextra - n;
	memcpy(rptr, extrabuf + n * size, nextra);
	return nextra;
}

int
substitute_record(filename, rptr, size, id)
char *filename;
void *rptr;
int size, id;
{
#ifdef LINUX
	struct flock ldata;
#endif
	int retv = 0;
	int fd;
	if ((fd = open(filename, O_WRONLY | O_CREAT, 0660)) == -1)
		return -1;
#ifdef LINUX
	ldata.l_type = F_WRLCK;
	ldata.l_whence = 0;
	ldata.l_len = size;
	ldata.l_start = size * (id - 1);
	if (fcntl(fd, F_SETLKW, &ldata) == -1) {
		close(fd);
		errlog("reclock error %d", errno);
		return -1;
	}
#else
	flock(fd, LOCK_EX);
#endif
	if (lseek(fd, size * (id - 1), SEEK_SET) == -1) {
		errlog("subrec seek err %d", errno);
		retv = -1;
		goto FAIL;
	}
	if (safewrite(fd, rptr, size) != size) {
		errlog("subrec write err %d", errno);
		retv = -1;
		goto FAIL;
	}
      FAIL:
#ifdef LINUX
	ldata.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &ldata);
#else
	flock(fd, LOCK_UN);
#endif
	close(fd);
	return retv;
}

int
insert_record(fpath, data, size, pos, num)
char *fpath;
void *data;
int size;
int pos;
int num;
{
	int fd;
	off_t off, len;
	struct stat st;

	if ((fd = open(fpath, O_RDWR | O_CREAT, 0600)) < 0)
		return -1;

	flock(fd, LOCK_EX);

	fstat(fd, &st);
	len = st.st_size;

	/* lkchu.990428: ernie patch 如果 len=0 & pos>0 
	   (在刚开精华区目录进去贴上，选下一个) 时会写入垃圾 */
	off = len ? size * pos : 0;
	lseek(fd, off, SEEK_SET);

	size *= num;
	len -= off;
	if (len > 0) {
		fpath = (char *) malloc(pos = len + size);
		memcpy(fpath, data, size);
		read(fd, fpath + size, len);
		lseek(fd, off, SEEK_SET);
		data = fpath;
		size = pos;
	}

	write(fd, data, size);

	flock(fd, LOCK_UN);

	close(fd);

	if (len > 0)
		free(data);

	return 0;
}

#ifndef EXT_UTL

int
delete_range(filename, id1, id2)
char *filename;
int id1, id2;
{
	struct fileheader fhdr;
	struct fileheader *fhmw, *fhw;
	int fdr;
	int count;
	int wcount, wstart;
	char fullpath[STRLEN];
	struct stat st;

	if (digestmode == 4 || digestmode == 5) {
		return 0;
	}

	if ((fdr = open(filename, O_RDWR, 0)) == -1) {
		return -2;
	}
	flock(fdr, LOCK_EX);
	fstat(fdr, &st);
	if (st.st_size % sizeof (struct fileheader) != 0) {
		flock(fdr, LOCK_UN);
		close(fdr);
		return -1;
	}
	fhmw = malloc(st.st_size);
	if (fhmw == NULL) {
		flock(fdr, LOCK_UN);
		close(fdr);
		return -1;
	}
	fhw = fhmw;
	count = 0;
	wstart = -1;
	wcount = 0;
	while (read(fdr, &fhdr, sizeof (fhdr)) == sizeof (fhdr)) {
		count++;
		if ((fhdr.accessed & FH_MARKED) ||
		    (id2 == -1 && !(fhdr.accessed & FH_DEL)) ||
		    (id2 != -1 && (count < id1 || count > id2))) {
			if (wstart == -1)
				continue;
			memcpy(fhw, &fhdr, sizeof (struct fileheader));
			wcount++;
			fhw++;
		} else {
			if (wstart == -1)
				wstart = count - 1;
			if (!fhdr.filetime)
				continue;
/* add by KCN for delete ranger backup, modified by ylsdd */
#ifdef BACK_DELETE_RANGE
			if (uinfo.mode == RMAIL) {
				setmailfile(fullpath,
					    currentuser->userid,
					    fh2fname(&fhdr));
				update_mailsize_down(&fhdr,
						     currentuser->userid);
				deltree(fullpath);
			} else
				cancelpost(currboard,
					   currentuser->userid, &fhdr, 0);
#endif
		}
	}
	if (wstart != -1) {
		ftruncate(fdr, (wstart + wcount) * sizeof (struct fileheader));
		if (wcount > 0) {
			lseek(fdr, wstart * sizeof (struct fileheader),
			      SEEK_SET);
			write(fdr, fhmw, wcount * sizeof (struct fileheader));
		}
	}
	free(fhmw);
	flock(fdr, LOCK_UN);
	close(fdr);
	return 0;
}
#endif

int
update_file(dirname, size, ent, filecheck, fileupdate)
char *dirname;
int size, ent;
int (*filecheck) (void *);
void (*fileupdate) (void *);
{
	char abuf[BUFSIZE];
	int fd;

	if (size > BUFSIZE) {
		errlog("too big %s", dirname);
		return -1;
	}
	if ((fd = open(dirname, O_RDWR)) == -1)
		return -1;
	flock(fd, LOCK_EX);
	if (lseek(fd, size * (ent - 1), SEEK_SET) != -1) {
		if (read(fd, abuf, size) == size)
			if ((*filecheck) (abuf)) {
				lseek(fd, -size, SEEK_CUR);
				(*fileupdate) (abuf);
				if (safewrite(fd, abuf, size) != size) {
					errlog("update err");
					flock(fd, LOCK_UN);
					close(fd);
					return -1;
				}
				flock(fd, LOCK_UN);
				close(fd);
				return 0;
			}
	}
	lseek(fd, 0, SEEK_SET);
	while (read(fd, abuf, size) == size) {
		if ((*filecheck) (abuf)) {
			lseek(fd, -size, SEEK_CUR);
			(*fileupdate) (abuf);
			if (safewrite(fd, abuf, size) != size) {
				errlog("update err");
				flock(fd, LOCK_UN);
				close(fd);
				return -1;
			}
			flock(fd, LOCK_UN);
			close(fd);
			return 0;
		}
	}
	flock(fd, LOCK_UN);
	close(fd);
	return -1;
}
