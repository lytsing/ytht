#include "bbs.h"
#include <gd.h>
#include <string.h>
#define MAXFONTS 19		/* 0.ttf  ... 18.ttf */

int
__to16(char c)
{
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= '0' && c <= '9')
		return c - '0';
	return 0;
}

void
__unhcode(char *s)
{
	int m, n;
	for (m = 0, n = 0; s[m]; m++, n++) {
		if (s[m] == '+') {
			s[n] = ' ';
			continue;
		}
		if (s[m] == '%') {
			if (!s[m + 1] || !s[m + 2])
				break;
			s[n] = __to16(s[m + 1]) * 16 + __to16(s[m + 2]);
			m += 2;
			continue;
		}
		s[n] = s[m];
	}
	s[n] = 0;
}

char *
getsenv(char *s)
{
	char *t = getenv(s);
	if (t)
		return t;
	return "";
}

int
http_error()
{
	errlog("failed");
	printf("HTTP/1.0 500 Internal Server Error\n\n");
	return 0;
}

struct bbsinfo bbsinfo;

int
main()
{
	gdImagePtr im;
	int black;
	int white;
	int brect[8];
	int x, y;
	char *err;
	char *png;
	int pngsize;

	char s[5];		/* String to draw. */
	double sz = 40.;
	char fontpath[256];
	unsigned int r;
	char buf[50];
	char userid[IDLEN + 1];
	struct MD5Context mdc;
	unsigned int pass[4];
	if (initbbsinfo(&bbsinfo) < 0) {
		http_error();
		return -1;
	}
	bzero(pass, sizeof (pass));
	strsncpy(buf, getsenv("QUERY_STRING"), sizeof (buf));
	__unhcode(buf);
	if (strncmp(buf, "userid=", 7)) {
		userid[0] = 0;
	} else {
		strsncpy(userid, buf + 7, IDLEN + 1);
		//      errlog("%s",userid);
	}
	if (userid[0] == 0) {
		s[0] = 0;
	} else {
		MD5Init(&mdc);
		MD5Update(&mdc, (void *) (&bbsinfo.ucachehashshm->regkey),
			  sizeof (int) * 4);
		MD5Update(&mdc, userid, strlen(userid));
		MD5Final((char *) pass, &mdc);
		sprintf(s, "%d%d%d%d", pass[0] % 10, pass[1] % 10, pass[2] % 10,
			pass[3] % 10);
	}
	getrandomint(&r);
	sprintf(fontpath, MY_BBS_HOME "/etc/fonts/%d.ttf", r % MAXFONTS);	/* User supplied font */

/* obtain brect so that we can size the image */
	err = gdImageStringTTF(NULL, &brect[0], 0, fontpath, sz, 0., 0, 0, s);
	if (err) {
		http_error();
		return 1;
	}
/* create an image big enough for the string plus a little whitespace */
	x = brect[2] - brect[6] + 6;
	y = brect[3] - brect[7] + 6;
	im = gdImageCreate(x, y);

/* Background color (first allocated) */
	white = gdImageColorResolve(im, 255, 255, 255);
	black = gdImageColorResolve(im, 0, 0, 0);

/* render the string, offset origin to center string*/
/* note that we use top-left coordinate for adjustment
 * since gd origin is in top-left with y increasing downwards. */
	x = 3 - brect[6];
	y = 3 - brect[7];
	err = gdImageStringFT(im, &brect[0], black, fontpath, sz, 0.0, x, y, s);
	if (err) {
		http_error();
		return 1;
	}
/* Write img to stdout */
#if 0
	fprintf(stdout, "Content-type: image/png\r\n\r\n");
	gdImagePng(im, stdout);
#else
	png = gdImagePngPtr(im, &pngsize);
	if (!png) {
		http_error();
		return 1;
	}
	printf("Content-type: image/png\n");
	printf("Content-Length: %d\n\n", pngsize);
	fwrite(png, 1, pngsize, stdout);
	gdFree(png);
#endif

/* Destroy it */
	gdImageDestroy(im);
	return 0;
}
