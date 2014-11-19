/*
 * bmplib.c : bmp reading library
 *            only little endian machine supported
 *
 * Copyright(C) 2002 holelee
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmplib.h"

#define BI_RGB 0
#define BI_RLE4 1
#define BI_RLE8 2
#define BI_BITFIELD 3

struct palette_s {
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char filter;
};

struct bmphandle_s {
	/* bmp header contents */
	int filesize;
	int reserved;
	int dataoffset;
	int headersize;
	int width;
	int height;
	short nplanes;
	short bpp;
	int compression;
	int bitmapsize;
	int hres;
	int vres;
	int ncolors;
	int importantcolors;

	/* pixel data, getpixel function pointer */
	int npalette;
	int bytes_per_line;
	struct palette_s *palette;
	unsigned char *data;
	struct bgrpixel (*getpixel) (bmphandle_t, int, int);
	unsigned bsize_blue, bsize_green, bsize_red;
	unsigned boffset_blue, boffset_green, boffset_red;
};

static struct bgrpixel
getpixel_8bpp(bmphandle_t bh, int x, int y)
{
	struct bgrpixel ret;
	unsigned char *pdata;
	int offset = (bh->height - y - 1) * bh->bytes_per_line + x;
	int pixel8;

	pdata = bh->data + offset;
	pixel8 = *pdata;
	/* palette lookup */
	ret.b = bh->palette[pixel8].blue;
	ret.g = bh->palette[pixel8].green;
	ret.r = bh->palette[pixel8].red;

	return ret;
}

static struct bgrpixel
getpixel_16bpp(bmphandle_t bh, int x, int y)
{
	/* BI_RGB case */
	struct bgrpixel ret;
	unsigned short *pdata;
	unsigned *mask = (unsigned *) (bh->palette);
	int offset = (bh->height - y - 1) * bh->bytes_per_line + (x << 1);

	pdata = (unsigned short *) (bh->data + offset);
	ret.b =
	    ((*pdata & mask[2]) >> bh->boffset_blue) << (8 - bh->bsize_blue);
	ret.g =
	    ((*pdata & mask[1]) >> bh->boffset_green) << (8 - bh->bsize_green);
	ret.r = ((*pdata & mask[0]) >> bh->boffset_red) << (8 - bh->bsize_red);

	return ret;
}

static struct bgrpixel
getpixel_32bpp(bmphandle_t bh, int x, int y)
{
	struct bgrpixel ret;
	unsigned *pdata;
	unsigned *mask = (unsigned *) (bh->palette);
	int offset = (bh->height - y - 1) * bh->bytes_per_line + (x << 2);

	pdata = (unsigned *) (bh->data + offset);

	ret.b =
	    ((*pdata & mask[2]) >> bh->boffset_blue) << (8 - bh->bsize_blue);
	ret.g =
	    ((*pdata & mask[1]) >> bh->boffset_green) << (8 - bh->bsize_green);
	ret.r = ((*pdata & mask[0]) >> bh->boffset_red) << (8 - bh->bsize_red);

	return ret;
}

static struct bgrpixel
getpixel_24bpp(bmphandle_t bh, int x, int y)
{
	struct bgrpixel ret;
	unsigned char *pdata;
	int offset = (bh->height - y - 1) * bh->bytes_per_line + x * 3;

	if (offset < 0 || offset > bh->bitmapsize - 3 || x < 0 || y < 0
	    || x >= bh->width || y >= bh->height) {
		ret.r = 0;
		ret.g = 0;
		ret.b = 0;
		return ret;
	}

	pdata = bh->data + offset;
	ret.b = *pdata;
	ret.g = *(pdata + 1);
	ret.r = *(pdata + 2);

	return ret;
}

static int
bmp_readheader(char *buf, int size, bmphandle_t bh)
{
	int tsize;
	int remnant;

	if (size < 54)
		return -1;
	/* check ID */
	if (buf[0] != 'B' || buf[1] != 'M')
		return -1;
	/* Does not support other IDs such as IC */

	/* reading header */
	memcpy(&bh->filesize, buf + 2, 52);

	bh->npalette = (bh->dataoffset - 54) >> 2;
	/* dword boundary on line end */
	tsize = bh->width * bh->bpp / 8;
	remnant = tsize % 4;
	if (remnant == 0)
		bh->bytes_per_line = tsize;
	else
		bh->bytes_per_line = tsize + (4 - remnant);

	return 0;
}

static int
bmp_readdata(char *buf, int size, bmphandle_t bh)
{
	if (bh->dataoffset < 0 || bh->bitmapsize < 0)
		return -1;
	if (size < bh->dataoffset + bh->bitmapsize)
		return -1;
	if (bh->bitmapsize != bh->bytes_per_line * bh->height)
		return -1;
	if (bh->compression != BI_RGB && bh->compression != BI_BITFIELD)
		return -1;
	bh->data = (unsigned char *) malloc(bh->bitmapsize);
	memcpy(bh->data, buf + bh->dataoffset, bh->bitmapsize);
	return 0;
}

static int
bmp_readpalette(char *buf, int size, bmphandle_t bh)
{
	if (sizeof (struct palette_s) * bh->npalette >= size - 54)
		return -1;
	memcpy(bh->palette, buf + 54, sizeof (struct palette_s) * bh->npalette);
	return 0;
}

static void
calculate_boffset(bmphandle_t bh, int len)
{
	int i;
	unsigned *mask = (unsigned *) (bh->palette);
	unsigned temp;
	unsigned highest = 0x1UL << (len - 1);

	/* red */
	temp = mask[0];
	for (i = 0; i < len; i++) {
		if (temp & 0x01)
			break;
		temp >>= 1;
	}
	bh->boffset_red = i;
	for (i = 0; i < len; i++) {
		if (temp & highest)
			break;
		temp <<= 1;
	}
	bh->bsize_red = len - i;

	/* green */
	temp = mask[1];
	for (i = 0; i < len; i++) {
		if (temp & 0x01)
			break;
		temp >>= 1;
	}
	bh->boffset_green = i;
	for (i = 0; i < len; i++) {
		if (temp & highest)
			break;
		temp <<= 1;
	}
	bh->bsize_green = len - i;

	/* blue */
	temp = mask[2];
	for (i = 0; i < len; i++) {
		if (temp & 0x01)
			break;
		temp >>= 1;
	}
	bh->boffset_blue = i;
	for (i = 0; i < len; i++) {
		if (temp & highest)
			break;
		temp <<= 1;
	}
	bh->bsize_blue = len - i;
}

bmphandle_t
bmp_open(char *buf, int size)
{
	bmphandle_t bh;
	bh = (bmphandle_t) malloc(sizeof (*bh));
	memset(bh, 0, sizeof (*bh));	/* I don't like calloc */
	if (bmp_readheader(buf, size, bh))
		goto error;
	if (bh->bpp < 24 && bh->npalette > 256)
		goto error;
	if (bh->npalette != 0) {
		bh->palette =
		    (struct palette_s *) malloc(sizeof (struct palette_s) *
						bh->npalette);
		memset(bh->palette, 0,
		       sizeof (struct palette_s) * bh->npalette);
		if (bmp_readpalette(buf, size, bh))
			goto error;
	}

	if (bmp_readdata(buf, size, bh))
		goto error;

	switch (bh->bpp) {
	case 1:
	case 4:
		goto error;
	case 8:
		bh->getpixel = getpixel_8bpp;
		break;

	case 16:
		bh->getpixel = getpixel_16bpp;
		if (bh->compression == BI_RGB) {
			unsigned *mask;
			if (bh->palette != NULL)	/* something wrong */
				goto error;
			mask = (unsigned *) malloc(sizeof (unsigned) * 3);
			mask[2] = 0x001F;	/* blue mask */
			mask[1] = 0x03E0;	/* green mask */
			mask[0] = 0x7C00;	/* red mask */
			bh->palette = (struct palette_s *) mask;
			bh->boffset_blue = 0;
			bh->boffset_green = 5;
			bh->boffset_red = 10;
			bh->bsize_blue = 5;
			bh->bsize_green = 5;
			bh->bsize_red = 5;
		} else {	/* BI_BITFIELD */

			if (bh->palette == NULL)	/* something wrong */
				goto error;
			calculate_boffset(bh, 16);
		}
		break;
	case 24:
		bh->getpixel = getpixel_24bpp;
		break;
	case 32:
		bh->getpixel = getpixel_32bpp;
		if (bh->compression == BI_RGB) {
			unsigned *mask;
			if (bh->palette != NULL)	/* something wrong */
				goto error;

			mask = (unsigned *) malloc(sizeof (unsigned) * 3);
			mask[2] = 0x000000FF;	/* blue mask */
			mask[1] = 0x0000FF00;	/* green mask */
			mask[0] = 0x00FF0000;	/* red mask */
			bh->palette = (struct palette_s *) mask;
			bh->boffset_blue = 0;
			bh->boffset_green = 8;
			bh->boffset_red = 16;
			bh->bsize_blue = 8;
			bh->bsize_green = 8;
			bh->bsize_red = 8;
		} else {	/* BI_BITFILED */

			if (bh->palette == NULL)	/* something wrong */
				goto error;
			calculate_boffset(bh, 32);
		}
		break;
	default:
		goto error;
	}
	return bh;
      error:
	bmp_close(bh);
	return NULL;
}

void
bmp_close(bmphandle_t bh)
{
	if (bh->data)
		free(bh->data);
	free(bh);
}

int
bmp_width(bmphandle_t bh)
{
	return bh->width;
}

int
bmp_height(bmphandle_t bh)
{
	return bh->height;
}

struct bgrpixel
bmp_getpixel(bmphandle_t bh, int x, int y)
{
	return bh->getpixel(bh, x, y);
}
