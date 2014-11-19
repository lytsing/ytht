#include <stdio.h>
#include <string.h>
#include "htmlfunc.h"
//ֻ�滻�����ź�'\\'���۳�ĩβ�İ�����֡������ֱ�����������'\\'���滻
//�ַ����е�><֮�������ַ������ɽű��Լ����������ڷ������˴���
//
//����ǰ�����ּ���', ��֪����ʲôЧ��������������ٸİ�
char *
scriptstr(const unsigned char *s)
{
	static char buf[1024];
	int i = 0, db = 0;
	for (; *s && i < 1017; s++) {
		if (db == 1) {
			db = 0;
			if (*s < 32)
				buf[i - 1] = 32;
			buf[i] = *s;
			i++;
			continue;
		}
		if (*s == '\'') {
			strcpy(buf + i, "\\\'");
			i += 2;
		} else if (*s == '/') {	// Bug of browsers: '</script>'
			strcpy(buf + i, "\\/");
			i += 2;
		} else if (*s == '\\') {
			strcpy(buf + i, "\\\\");
			i += 2;
		} else if (*s == '\r') {
			continue;
		} else if (*s == '\n') {
			strcpy(buf + i, "\\n");
			i += 2;
		} else {
			if (*s >= 128)
				db = 1;
			buf[i] = *s;
			i++;
		}
	}
	if (db)
		buf[i - 1] = 0;
	buf[i] = 0;
	return buf;
}

char *
void1(char *s0)
{
	int i;
	int flag = 0;
	unsigned char *s = (unsigned char *) s0;
	for (i = 0; s[i]; i++) {
		if (flag == 0) {
			if (s[i] >= 128)
				flag = 1;
			continue;
		}
		flag = 0;
		if (s[i] < 32)
			s[i - 1] = 32;
	}
	if (flag)
		s[i - 1] = 0;
	//s[strlen(s) - 1] = 0;
	return s;
}

/*
 * "����.jpg" --> "%BA%BA%D7%D6.jpg"
 *    RFC 1738
 */
char *
urlencode(char *str)
{
	static char buf[256];
	char *pstr = str, *pbuf = buf, c;
	while (*pstr && pbuf - buf < sizeof (buf) - 3) {
		c = *pstr;
		if (('a' <= c && c <= 'z')
		    || ('A' <= c && c <= 'Z')
		    || ('0' <= c && c <= '9')
		    || c == '-' || c == '_' || c == '.'
		    || c == '\r' || c == '\n') {
			*pbuf = c;
			pstr++;
			pbuf++;
		} else {
			snprintf(pbuf, 4, "%%%02X", c & 0xff);
			pstr++;
			pbuf += 3;
		}
	}
	*pbuf = '\0';
	return buf;
}
