#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "ythtbbs.h"

#define NEEDALIGN 0

#if NEEDALIGN
static unsigned tmpushort;
#endif

static void
shorter_brc(struct onebrc *brc)
{
	int i;
	if (brc->num < 2)
		return;
	if (brc->list[0] >= 2 && brc->list[brc->num - 1] == 1
	    && brc->list[brc->num - 2] == 2)
		brc->num--;
	for (i = brc->num - 1; i > 0; i--) {
		if (brc->list[i - 1] - brc->list[i] == 1)
			brc->num--;
		else
			break;
	}
}

static void
compress_brc(struct onebrc_c *brc_c, struct onebrc *brc)
{
	int i, diff;
	char *bits, *ptr = brc_c->data;
	bzero(brc_c, sizeof (*brc_c));
	strcpy(ptr, brc->board);
	ptr += strlen(brc->board) + 1;
	*ptr = brc->num;
	ptr++;
#if !NEEDALIGN
	*(unsigned *) ptr = brc->list[0];
#else
	memcpy(ptr, &brc->list[0], 4);
#endif
	ptr += sizeof (unsigned);
	bits = ptr;
	ptr += ((brc->num - 1) * 2 + 7) / 8;
	for (i = 0; i < brc->num - 1; i++) {
		diff = brc->list[i] - brc->list[i + 1];
		if (diff < 256) {
			*(unsigned char *) ptr = (unsigned char) diff;
			ptr += sizeof (unsigned char);
		} else if (diff < 256 * 256) {
#if !NEEDALIGN
			*(unsigned short *) ptr = (unsigned short) diff;
#else
			tmpushort = (unsigned short) diff;
			memcpy(ptr, &tmpushort, 2);
#endif
			ptr += sizeof (unsigned short);
			bits[i / 4] |= 1 << (i % 4);
		} else if (diff < 256 * 256 * 256) {
			ptr[0] = ((unsigned char *) &diff)[0];
			ptr[1] = ((unsigned char *) &diff)[1];
			ptr[2] = ((unsigned char *) &diff)[2];
			ptr += 3;
			bits[i / 4] |= 1 << (i % 4 + 4);
		} else {
#if !NEEDALIGN
			*(unsigned *) ptr = (unsigned) diff;
#else
			memcpy(ptr, &diff, 4);
#endif
			ptr += sizeof (unsigned);
			bits[i / 4] |= 1 << (i % 4);
			bits[i / 4] |= 1 << (i % 4 + 4);
		}
	}
	if (brc->notetime) {
#if !NEEDALIGN
		*(int *) ptr = brc->notetime;
#else
		memcpy(ptr, &brc->notetime, 4);
#endif
		ptr += sizeof (int);
	}
#if !NEEDALIGN
	brc_c->len = ptr - (char *) brc_c;
#else
	tmpushort = ptr - (char *) brc_c;
	memcpy(&brc_c->len, &tmpushort, 2);
#endif
}

static void
uncompress_brc(struct onebrc *brc, struct onebrc_c *brc_c)
{
	int i, diff, bl, bh, num;
	char *ptr, *bits;
	brc->changed = 0;
	ptr = brc_c->data;
	strcpy(brc->board, ptr);
	ptr += strlen(ptr) + 1;
	brc->num = *ptr & 0x7f;
	ptr++;
#if !NEEDALIGN
	brc->list[0] = *(unsigned *) ptr;
#else
	memcpy(&brc->list[0], ptr, 4);
#endif
	ptr += sizeof (unsigned);
	bits = ptr;
	ptr += ((brc->num - 1) * 2 + 7) / 8;
	num = brc->num - 1;
	for (i = 0; i < num; i++) {
		bl = bits[i / 4] & (1 << (i % 4));
		bh = bits[i / 4] & (1 << ((i % 4) + 4));
		if (!bh) {
			if (!bl) {
				diff = *(unsigned char *) ptr;
				ptr += sizeof (unsigned char);
			} else {
#if !NEEDALIGN
				diff = *(unsigned short *) ptr;
#else
				memcpy(&tmpushort, ptr, 2);
				diff = tmpushort;
#endif
				ptr += sizeof (unsigned short);
			}
		} else {
			if (!bl) {
				diff = 0;
				((unsigned char *) &diff)[0] = ptr[0];
				((unsigned char *) &diff)[1] = ptr[1];
				((unsigned char *) &diff)[2] = ptr[2];
				ptr += 3;
			} else {
#if !NEEDALIGN
				diff = *(unsigned *) ptr;
#else
				memcpy(&diff, ptr, 4);
#endif
				ptr += sizeof (unsigned);
			}
		}
		brc->list[i + 1] = brc->list[i] - diff;
	}
#if !NEEDALIGN
	if (ptr - (char *) brc_c < brc_c->len) {
		brc->notetime = *(int *) ptr;
	} else
		brc->notetime = 0;
#else
	memcpy(&tmpushort, &brc_c->len, 2);
	if (ptr - (char *) brc_c < tmpushort) {
		memcpy(&brc->notetime, ptr, 4);
	} else
		brc->notetime = 0;
#endif
}

static int
brc_c_unreadt(struct onebrc_c *brc_c, int t)
{
	int i, diff, bl, bh, num, thist;
	char *ptr, *bits;

	ptr = brc_c->data;
	ptr += strlen(ptr) + 1;
	num = *ptr & 0x7f;
	ptr++;
#if !NEEDALIGN
	thist = *(unsigned *) ptr;
#else
	memcpy(&thist, ptr, 4);
#endif
	if (t > thist)
		return 1;
	else if (t == thist)
		return 0;
	ptr += sizeof (unsigned);
	bits = ptr;
	ptr += ((num - 1) * 2 + 7) / 8;
	//if (num > 4)
	//      num = 4;
	num--;
	for (i = 0; i < num; i++) {
		bl = bits[i / 4] & (1 << (i % 4));
		bh = bits[i / 4] & (1 << ((i % 4) + 4));
		if (!bh) {
			if (!bl) {
				diff = *(unsigned char *) ptr;
				ptr += sizeof (unsigned char);
			} else {
#if !NEEDALIGN
				diff = *(unsigned short *) ptr;
#else
				memcpy(&tmpushort, ptr, 2);
				diff = tmpushort;
#endif
				ptr += sizeof (unsigned short);
			}
		} else {
			if (!bl) {
				diff = 0;
				((unsigned char *) &diff)[0] = ptr[0];
				((unsigned char *) &diff)[1] = ptr[1];
				((unsigned char *) &diff)[2] = ptr[2];
				ptr += 3;
			} else {
#if !NEEDALIGN
				diff = *(unsigned *) ptr;
#else
				memcpy(&diff, ptr, 4);
#endif
				ptr += sizeof (unsigned);
			}
		}
		thist -= diff;
		if (t > thist)
			return 1;
		else if (t == thist)
			return 0;
	}
	return 0;
}

static void
settmpbrc(char *filename, char *userid)
{
	unsigned char ch;
	if (!*userid)
		ch = 0;
	else if (strncasecmp(userid, "guest.", 6)) {
		ch = toupper(*userid);
		if (ch < 'A' || ch > 'Z')
			ch = 'A' + ch % 26;
	} else {
		ch = userid[strlen(userid) - 1];
		if (ch > '9' || ch < '0')
			ch = '0' + ch % 10;
	}
	sprintf(filename, "%s/%c/%s", PATHTMPBRC, ch, userid);
}

void
brc_init(struct allbrc *allbrc, char *userid, char *filename)
{
	int fd;
	char filename1[80];
	allbrc->changed = 0;
	if (strncmp(userid, "guest.", 6))
		allbrc->maxsize = sizeof (allbrc->brc_c);
	else
		allbrc->maxsize = BRC_MAXSIZE_GUEST;
	settmpbrc(filename1, userid);
	if ((fd = open(filename1, O_RDONLY)) < 0) {
		if (filename == NULL || (fd = open(filename, O_RDONLY)) < 0) {
			allbrc->size = 0;
			return;
		}
	}
	allbrc->size = read(fd, allbrc->brc_c, BRC_MAXSIZE);
	close(fd);
	if (allbrc->size < 0)
		allbrc->size = 0;
}

void
brc_fini(struct allbrc *allbrc, char *userid)
{
	int fd;
	char filename1[80];
	char tmpfile[80];
	if (!allbrc->changed)
		return;
	sprintf(filename1, "%s.tmp", userid);
	settmpbrc(tmpfile, filename1);
	if ((fd = open(tmpfile, O_WRONLY | O_CREAT, 0660)) < 0)
		return;
	write(fd, allbrc->brc_c, allbrc->size);
	close(fd);
	settmpbrc(filename1, userid);
	rename(tmpfile, filename1);
	allbrc->changed = 0;
}

static void *
brc_getblock(struct allbrc *allbrc, char *name)
{
	char *ptr, *ptr0;
	short len;
	ptr0 = allbrc->brc_c + allbrc->size;
	for (ptr = allbrc->brc_c; ptr < ptr0;) {
#if !NEEDALIGN
		len = ((struct onebrc_c *) ptr)->len;
#else
		memcpy(&len, &((struct onebrc_c *) ptr)->len, 2);
#endif
		if (!len)
			break;
		if (ptr + len > ptr0)
			break;
		if (!strncmp
		    (((struct onebrc_c *) ptr)->data, name, BRC_STRLEN - 1))
			return ptr;
		ptr += len;
	}
	return NULL;
}

static void
brc_putblock(struct allbrc *allbrc, struct onebrc_c *brc_c)
{
	struct allbrc *tmpallbrc;
	char *ptr, *ptr1, *ptr0, *ptr10;
	short len;
	tmpallbrc = malloc(sizeof (struct allbrc));
	tmpallbrc->maxsize = allbrc->maxsize;
	ptr1 = tmpallbrc->brc_c;
	ptr10 = tmpallbrc->brc_c + tmpallbrc->maxsize;

	ptr = allbrc->brc_c;
	ptr0 = allbrc->brc_c + allbrc->size;

	//保证#INFO段是在第一个位置(其实是为了避免#INFO段丢失)
	if (ptr < ptr0 && strcmp(brc_c->data, "#INFO")
	    && !strcmp(((struct onebrc_c *) ptr)->data, "#INFO")) {
#if !NEEDALIGN
		len = ((struct onebrc_c *) ptr)->len;
#else
		memcpy(&len, &((struct onebrc_c *) ptr)->len, 2);
#endif
		memcpy(ptr1, ptr, len);
		ptr += len;
		ptr1 += len;
	}
#if !NEEDALIGN
	memcpy(ptr1, brc_c, brc_c->len);
	ptr1 += brc_c->len;
#else
	memcpy(&tmpushort, &brc_c->len, 2);
	memcpy(ptr1, brc_c, tmpushort);
	ptr1 += tmpushort;
#endif

	while (ptr < ptr0) {
#if !NEEDALIGN
		len = ((struct onebrc_c *) ptr)->len;
#else
		memcpy(&len, &((struct onebrc_c *) ptr)->len, 2);
#endif
		if (len <= 0)
			break;
		if (ptr + len > ptr0)
			break;
		if (ptr1 + len > ptr10)
			break;
		if (!strncmp
		    (((struct onebrc_c *) ptr)->data, brc_c->data,
		     BRC_STRLEN - 1)) {
			ptr += len;
			continue;
		}
		memcpy(ptr1, ptr, len);
		ptr += len;
		ptr1 += len;
	}
	tmpallbrc->size = ptr1 - tmpallbrc->brc_c;
	tmpallbrc->changed = 1;
	memcpy(allbrc, tmpallbrc, ptr1 - (char *) tmpallbrc);
	free(tmpallbrc);
}

void
brc_getboard(struct allbrc *allbrc, struct onebrc *brc, char *board)
{
	struct onebrc_c *ptr = brc_getblock(allbrc, board);
	if (ptr) {
		uncompress_brc(brc, ptr);
		return;
	}
	strncpy(brc->board, board, BRC_STRLEN - 1);
	brc->board[BRC_STRLEN - 1] = 0;
	brc->changed = 0;
	brc->num = 1;
	brc->cur = 0;
	brc->list[0] = 1;
	brc->notetime = 0;
}

int
brc_unreadt_quick(struct allbrc *allbrc, char *board, int t)
{
	struct onebrc_c *ptr = brc_getblock(allbrc, board);
	if (ptr)
		return brc_c_unreadt(ptr, t);
	return 1;
}

void
brc_putboard(struct allbrc *allbrc, struct onebrc *brc)
{
	struct onebrc_c brc_c;
	if (!brc->changed)
		return;
	brc->changed = 0;
	shorter_brc(brc);
	compress_brc(&brc_c, brc);
	brc_putblock(allbrc, &brc_c);
}

void
brc_getinfo(struct allbrc *allbrc, struct brcinfo *info)
{
	struct onebrc_c *brc_c = brc_getblock(allbrc, "#INFO");
	char *ptr;
	int size;
	bzero(info, sizeof (*info));
	if (!brc_c)
		return;
	ptr = brc_c->data + 6;
#if !NEEDALIGN
	size = brc_c->len - (ptr - (char *) brc_c);
#else
	memcpy(&tmpushort, &brc_c->len, 2);
	size = tmpushort - (ptr - (char *) brc_c);
#endif
	if (size > sizeof (*info))
		size = sizeof (*info);
	memcpy(info, ptr, size);
}

void
brc_putinfo(struct allbrc *allbrc, struct brcinfo *info)
{
	struct onebrc_c brc_c = { 0, "#INFO" };
	char *ptr = brc_c.data + 6;	//跳过字符串"#INFO", 向后附加info资料
	int size =
	    min(sizeof (*info), sizeof (brc_c) - (ptr - (char *) &brc_c));
	memcpy(ptr, info, size);
#if !NEEDALIGN
	brc_c.len = ptr - (char *) &brc_c + size;
#else
	tmpushort = ptr - (char *) &brc_c + size;
	memcpy(&brc_c.len, &tmpushort, 2);
#endif
	brc_putblock(allbrc, &brc_c);
}

static int
brc_locate(struct onebrc *brc, int t)
{
	if (brc->num == 0) {
		brc->cur = 0;
		return 0;
	}
	if (brc->cur >= brc->num)
		brc->cur = brc->num - 1;
	if (t <= brc->list[brc->cur]) {
		while (brc->cur < brc->num) {
			if (t == brc->list[brc->cur])
				return 1;
			if (t > brc->list[brc->cur])
				return 0;
			brc->cur++;
		}
		return 0;
	}
	while (brc->cur > 0) {
		if (t < brc->list[brc->cur - 1])
			return 0;
		brc->cur--;
		if (t == brc->list[brc->cur])
			return 1;
	}
	return 0;
}

static void
brc_insert(struct onebrc *brc, int t)
{
	if (brc->num < BRC_MAXNUM)
		brc->num++;
	if (brc->cur >= brc->num)
		return;
	brc->changed = 1;
	memmove(&brc->list[brc->cur + 1], &brc->list[brc->cur],
		sizeof (brc->list[0]) * (brc->num - brc->cur - 1));
	brc->list[brc->cur] = t;
}

static void
brc_set(struct onebrc *brc, int t)
{
	if (brc->num && brc->cur >= brc->num)
		return;
	brc->changed = 1;
	brc->list[brc->cur] = t;
	brc->num = brc->cur + 1;
}

void
brc_addlistt(struct onebrc *brc, int t)
{
	if (brc_unreadt(brc, t)) {
		brc_insert(brc, t);
	}
}

int
brc_unreadt(struct onebrc *brc, int t)
{
	if (brc_locate(brc, t))
		return 0;
	if (brc->num <= 0)
		return 1;
	if (brc->cur < brc->num)
		return 1;
	return 0;
}

void
brc_clearto(struct onebrc *brc, int t)
{
	brc_locate(brc, t);
	brc_set(brc, t);
}

#define BRC_MAXSIZEOLD     50000

static char *
brc_getrecord(char *ptr, struct onebrc *brc)
{
	char *tmp;
	strncpy(brc->board, ptr, BRC_STRLEN);
	ptr += BRC_STRLEN;
	brc->num = (*ptr++) & 0xff;
	tmp = ptr + brc->num * sizeof (int);
	if (brc->num > BRC_MAXNUM) {
		brc->num = BRC_MAXNUM;
	}
	memcpy(brc->list, ptr, brc->num * sizeof (int));
	return tmp;
}

void
brc_init_old(struct allbrc *allbrc, char *filename)
{
	FILE *fp;
	int brc_size;
	char *brc_buf, *ptr;
	struct onebrc brc;
	bzero(allbrc, sizeof (*allbrc));
	if (!(fp = fopen(filename, "r"))) {
		return;
	}
	brc_buf = malloc(BRC_MAXSIZEOLD);
	brc_size = fread(brc_buf, 1, BRC_MAXSIZE, fp);
	fclose(fp);
	ptr = brc_buf;
	while (ptr < &brc_buf[brc_size] && (*ptr >= ' ' && *ptr <= 'z')) {
		bzero(&brc, sizeof (brc));
		ptr = brc_getrecord(ptr, &brc);
		brc.changed = 1;
		brc_putboard(allbrc, &brc);
	}
	free(brc_buf);
}
