#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <iconv.h>
#include "ythtbbs.h"

char *
utf8cut(char *str, int maxsize)
{
	int len = maxsize - 1;
	setlocale(LC_CTYPE, "zh_CN.utf8");
	str[len] = 0;
	while (len > 0 && mbstowcs(NULL, str, 0) == (size_t) (-1)) {
		len--;
		str[len] = 0;
	}
	setlocale(LC_CTYPE, "");
	return str;
}

char *
gb2utf8(char *to0, int tolen0, char *from0)
{
	static iconv_t cd = (iconv_t) - 1;
	char *to = to0, *from = from0;
	int retv;
	size_t tolen, flen = strlen(from);

	if (cd == (iconv_t) - 1)
		cd = iconv_open("UTF-8", "GB2312");
	if (cd == (iconv_t) - 1) {
		strsncpy(to0, from0, tolen0);
		return to0;
	}
	tolen = tolen0 - 1;
	retv = iconv(cd, &from, &flen, &to, &tolen);
	//iconv_close(cd);
	if (retv != -1) {
		to0[tolen0 - 1 - tolen] = 0;
	} else {
		strsncpy(to0, from0, tolen0);
	}
	return to0;
}

