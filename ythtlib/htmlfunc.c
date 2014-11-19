#include <stdio.h>
#include <string.h>
#include "htmlfunc.h"
//只替换单引号和'\\'，扣除末尾的半个汉字。繁体字编码中遇到的'\\'不替换
//字符串中的><之类特殊字符可以由脚本自己来处理，不在服务器端处理
//
//如果是半个汉字加上', 不知道是什么效果。如果有问题再改吧
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
 * "汉字.jpg" --> "%BA%BA%D7%D6.jpg"
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
