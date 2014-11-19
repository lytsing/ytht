#include "convcode.h"
#include "convtable.c"

static char gb2big_savec[2];
static char big2gb_savec[2];

static inline void
b2g(unsigned char *s)
{
	unsigned char n;
	int m;
	if ((m = s[1] - 0x40) >= 0) {
		n = s[0] << 1;
		s[0] = b2gtab[m][n];
		s[1] = b2gtab[m][n + 1];
	} else {
		s[0] = '?';
	}
}

static inline void
g2b(unsigned char *s)
{
	unsigned char n;
	int m;
	if ((m = s[1] - 0x40) >= 0) {
		n = s[0] << 1;
		s[0] = g2btab[m][n];
		s[1] = g2btab[m][n + 1];
	} else {
		s[0] = '?';
	}
}

int
sgb2big(char *str0, int len)
{
	unsigned char *str, *strend1;
	str = str0;
	strend1 = str + len - 1;
	while (str < strend1) {
		if ((str[0] & 0x80)) {
			g2b(str);
			str += 2;
		} else
			str++;
	}
	if (str == strend1 && (str[0] & 0x80))
		return str[0];
	return 0;
}

int
sbig2gb(char *str0, int len)
{
	unsigned char *str, *strend1;
	str = str0;
	strend1 = str + len - 1;
	while (str < strend1) {
		if ((str[0] & 0x80)) {
			b2g(str);
			str += 2;
		} else
			str++;
	}
	if (str == strend1 && (str[0] & 0x80))
		return str[0];
	return 0;
}

void
conv_init()
{
	gb2big_savec[0] = 0;
	big2gb_savec[0] = 0;
	gb2big_savec[1] = 0;
	big2gb_savec[1] = 0;
}

char *
gb2big(char *s, int *plen, int inst)
{
	if (*plen == 0)
		return s;
	if (gb2big_savec[inst]) {
		*(--s) = gb2big_savec[inst];
		(*plen)++;
	}
	gb2big_savec[inst] = sgb2big(s, *plen);
	if (gb2big_savec[inst])
		(*plen)--;
	return s;
}

char *
big2gb(char *s, int *plen, int inst)
{
	if (*plen == 0)
		return s;
	if (big2gb_savec[inst]) {
		*(--s) = big2gb_savec[inst];
		(*plen)++;
	}
	big2gb_savec[inst] = sbig2gb(s, *plen);
	if (big2gb_savec[inst])
		(*plen)--;
	return s;
}
