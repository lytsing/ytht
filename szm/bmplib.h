/*
 * bmplib.h : header for bmplib(simple bmp reading library)
 *
 * Copyright(C) 2002 holelee
 *
 */

struct bmphandle_s;

typedef struct bmphandle_s *bmphandle_t;

bmphandle_t bmp_open(char *buf, int size);
void bmp_close(bmphandle_t bh);

int bmp_height(bmphandle_t bh);
int bmp_width(bmphandle_t bh);

struct bgrpixel
{
	unsigned char b, g, r;
};

struct bgrpixel bmp_getpixel(bmphandle_t bh, int x, int y);

