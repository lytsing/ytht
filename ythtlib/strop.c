#include <string.h>
#include <locale.h>
#include <iconv.h>
#include "ythtlib.h"

void
strsncpy(char *s1, const char *s2, int n)
{
	int l = strnlen(s2, n);
	if (n <= 0) {
		if (n < 0) {
			errlog("n less than zero!!");
		}
		*s1 = 0;
		return;
	}
	if (n > l + 1)
		n = l + 1;
	memcpy(s1, s2, n - 1);
	s1[n - 1] = 0;
}

char *
strltrim(char *s)
{
	char *s2 = s;
	if (s[0] == 0)
		return s;
	while (s2[0] && strchr(" \t\r\n", s2[0]))
		s2++;
	return s2;
}

char *
strrtrim(char *s)
{
	static char t[1024], *t2;
	if (s[0] == 0)
		return s;
	strsncpy(t, s, sizeof (t));
	t2 = t + strlen(s) - 1;
	while (strchr(" \t\r\n", t2[0]) && t2 > t)
		t2--;
	t2[1] = 0;
	return t;
}

void
normalize(char *buf)
{
	int i = 0;
	while (buf[i]) {
		if (buf[i] == '/')
			buf[i] = ':';
		i++;
	}
}

int
myatoi(unsigned char *buf, int len)
{
	int i, k, ret = 0;
	for (i = 0; buf[i] && i < len; i++) {
		if (buf[i] >= 'a')
			k = buf[i] - 'a' + 26;
		else if (buf[i] >= 'A')
			k = buf[i] - 'A';
		else if (buf[i] >= '0')
			k = buf[i] - '0' + 52;
		else
			k = buf[i] - '*' + 62;
		ret = (ret << 6) + k;
	}
	return ret;
}

static const unsigned char mybase64[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789*+";

void
myitoa(int num, unsigned char *buf, int len)
{
	buf[len] = 0;
	while (len--) {
		buf[len] = mybase64[num & 0x3f];
		num = num >> 6;
	}
}

void
filteransi(char *line)
{
	int i, stat, j;
	stat = 0;
	j = 0;
	for (i = 0; line[i]; i++) {
		if (line[i] == '\033')
			stat = 1;
		if (!stat) {
			if (j != i)
				line[j] = line[i];
			j++;
		}
		if (stat && ((line[i] > 'a' && line[i] < 'z')
			     || (line[i] > 'A' && line[i] < 'Z')
			     || line[i] == '@'))
			stat = 0;
	}
	line[j] = 0;
}

void
filteransi2(char *line)
{
	int i, stat, j;
	stat = 0;
	j = 0;
	for (i = 0; line[i]; i++) {
		if (line[i] == '\033') {
			stat = 1;
			if (line[i + 1] == '<')
				stat = 2;
			if (line[i + 1] == '[' && line[i + 2] == '<')
				stat = 2;
		}
		if (!stat) {
			if (j != i)
				line[j] = line[i];
			j++;
		}
		if (stat == 1 && ((line[i] > 'a' && line[i] < 'z')
				  || (line[i] > 'A' && line[i] < 'Z')
				  || line[i] == '@'))
			stat = 0;
		if (stat == 2 && (line[i] == '>' || line[i] == '\n'))
			stat = 0;
	}
	line[j] = 0;
}

