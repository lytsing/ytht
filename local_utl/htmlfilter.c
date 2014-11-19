#include <stdio.h>
#include <stdlib.h>
#include "bbs.h"

int blog2board = 0;

char *(keepTag0[]) = {
"i", "b", "u", "em", "small", "strong", "tt", "dd", "dl", "dt", "pre",
	    "span", "font", "br", "p", "li", "ul", "ol", "table", "th",
	    "tr", "td", "hr", "h1", "h2", "h3", "h4", "h5", "h6", "a",
	    "img", "center", "blockquote", NULL};

char *(keepAtt0[][10]) = { {
"hr", "width", "color"}, {
"table", "border"}, {
"p", "align"}, {
"td", "align", "valign", "colspan", "rowspan"}, {
"a", "href"}, {
"img", "src", "width", "height", "align"}, {
"font", "size", "color"}, {
NULL}};

char *(deleteTag0[]) = {
"tbody", "form", "input", "div", NULL};

char *(keepTag1[]) = {
"i", "b", "u", "em", "small", "strong", "tt", "dd", "dl", "dt", "pre",
	    "span", "font", "br", "p", "li", "ul", "ol", "a",
	    "img", "center", "blockquote", NULL};

char *(keepAtt1[][10]) = { {
"p", "align"}, {
"a", "href"}, {
"img", "src", "width", "height"}, {
"font", "size", "color"}, {
NULL}};

char *(deleteTag1[]) = {
"table", "th", "tr", "td", "tbody", "form", "input", "div", "hr", "h1",
	    "h2", "h3", "h4", "h5", "h6", NULL};

struct Setting {
	char **keepTag;
	char *((*keepAtt)[10]);
	char **deleteTag;
} *setting;

struct Setting setting0 = { keepTag0, keepAtt0, deleteTag0 };
struct Setting setting1 = { keepTag1, keepAtt1, deleteTag1 };

void
skipHeader(char **pptr)
{
	char *p = strstr(*pptr, "<body>");
	if (p)
		*pptr = p + 6;
}

void
removeFooter(char *ptr)
{
	char *p = strstr(ptr, "</body>");
	if (p)
		*p = 0;
}

int
removeBlock(char *ptr, char *start, char *end)
{
	char *p0, *p1;
	int len;
	p0 = strcasestr(ptr, start);
	if (!p0)
		return -1;
	p1 = strcasestr(p0, end);
	if (!p1) {
		*p0 = 0;
		return -1;
	}
	p1 += strlen(end);
	len = strlen(p1) + 1;
	memmove(p0, p1, len);
	return 1;
}

char *
getNextElement(char **pptr)
{
	char *s, *p, *ptr = *pptr;
	int len;
	if (!*ptr)
		return NULL;
	if (*ptr == '<') {
		p = strchr(ptr, '>');
		if (!p)
			return NULL;
		p++;
		len = p - ptr;
	} else {
		p = strchr(ptr, '<');
		if (!p)
			len = strlen(ptr);
		else
			len = p - ptr;
	}
	s = malloc(len + 1);
	if (!s)
		return NULL;
	memcpy(s, ptr, len);
	s[len] = 0;
	*pptr = ptr + len;
	return s;
}

int
inList(char *(list[]), char *tag)
{
	int i;
	for (i = 0; list[i]; i++)
		if (!strcasecmp(list[i], tag))
			return 1;
	return 0;
}

void
printEndTag(char *el)
{
	char *p;
	p = strchr(el, '>');
	if (p)
		*p = 0;
	p = el + 2;
	if (inList(setting->keepTag, p)) {
		if (blog2board)
			printf("\033[");
		printf("</%s>", p);
	} else if (inList(setting->deleteTag, p))
		return;
	else
		printf("&lt;/%s&gt;", p);
}

int
getRightAtt(char *attStr, char att[30], char **val)
{
	char *ptr, *ptrStart;
	*val = NULL;
	ptr = attStr + strlen(attStr) - 1;
	while (ptr >= attStr && strchr(" \t\r\n", *ptr)) {
		*ptr = 0;
		ptr--;
	}
	if (!*attStr)
		return -1;
	if (attStr[1] == 0) {
		strsncpy(att, attStr, 30);
		return 1;
	}
	ptr = attStr + strlen(attStr) - 1;
	ptrStart = ptr;
	if (*ptr == '\"' || *ptr == '\'') {
		ptrStart--;
		while (ptrStart > attStr && *ptrStart != *ptr)
			ptrStart--;
	}
	while (ptrStart > attStr && !strchr(" \t\r\n", ptrStart[-1])) {
		//printf("=====%s====%c=====\n", attStr, *(ptrStart -1));
		ptrStart--;
	}
	if ((ptr = strchr(ptrStart, '='))) {
		*ptr = 0;
		*val = ptr + 1;
	}
	strsncpy(att, ptrStart, 30);
	*ptrStart = 0;
	return 1;
}

void
printAttributes(char *tag, char *attStr)
{
	char list[5][30], *(vallist[5]);
	char *val;
	char *(*thisKeepAtt) = NULL;
	int i;
	for (i = 0; setting->keepAtt[i][0]; i++) {
		if (!strcasecmp(tag, setting->keepAtt[i][0])) {
			thisKeepAtt = &setting->keepAtt[i][1];
			break;
		}
	}
	i = 0;
	while (i < 5 && getRightAtt(attStr, list[i], &val) >= 0) {
		if (i == 0 && !strcmp(list[i], "/")) {
			vallist[i] = NULL;
			i++;
			continue;
		}
		//check src、href...
		//...

		if (!thisKeepAtt)
			break;
		if (inList(thisKeepAtt, list[i])) {
			vallist[i] = val;
			i++;
		}
	}
	while (i--) {
		printf(" %s", list[i]);
		if (vallist[i])
			printf("=%s", vallist[i]);
	}
}

void
fixTag(char *s)
{
	char *ptr;
	while ((ptr = strchr(s, '\t')))
		*ptr = ' ';
	while ((ptr = strchr(s, '\r'))) {
		memmove(ptr, ptr + 1, strlen(ptr));
	}
	while ((ptr = strstr(s, "=\n"))) {
		ptr++;
		memmove(ptr, ptr + 1, strlen(ptr));
	}
	while ((ptr = strchr(s, '\n')))
		*ptr = ' ';
}

void
printStartTag(char *el)
{
	char *p, *p1, *p2, *p3;
	fixTag(el);
	p = strchr(el, '>');
	if (p)
		*p = 0;
	p = el + 1;
	p1 = strchr(p, ' ');

	if (p1) {
		*p1 = 0;
		p1++;
	} else
		p1 = "";

	if (inList(setting->deleteTag, p))
		return;
	if (!inList(setting->keepTag, p)) {
		printf("&lt;%s %s&gt;", p, p1);
		return;
	}

	if (blog2board) {
		if (!strcasecmp(p, "img")) {
			if ((p2 = strstr(p1, "src"))) {
				p2 += 5;
				if ((p3 = strchr(p2, '\"')))
					*p3 = 0;
				printf("\033[<div style=\"display:"
					" none;\">\033[0;32m"
					"外部图片: \033[m%s\033[</div>", p2);
				if (p3)
					*p3 = '\"';
			}
		}
		printf("\033[");
	}
	printf("<%s", p);
	printAttributes(p, p1);
	printf(">");
}

char *
readFile(char *fn)
{
      struct mmapfile mf = { ptr:NULL };
	char *src;
	if (mmapfile(fn, &mf) < 0)
		return NULL;
	src = malloc(mf.size + 1);
	if (!src)
		return NULL;
	memcpy(src, mf.ptr, mf.size);
	src[mf.size] = 0;
	mmapfile(NULL, &mf);
	return src;
}

char *
readStdin()
{
	int size = 10240;
	int n = 0, nleft, lenread;
	char *ptr, *src = malloc(10240);
	if (!src)
		return NULL;
	while (1) {
		nleft = size - 1 - n;
		if (nleft <= 0) {
			ptr = realloc(src, size + 10240);
			if (!ptr)
				break;
			src = ptr;
			size += 10240;
			continue;
		}
		lenread = fread(src + n, 1, nleft, stdin);
		if (lenread <= 0)
			break;
		n += lenread;
	}
	src[n] = 0;
	return src;
}

int print_filter_html (char *el) {
	char *ptr, *str, *tmpptr;

	if (!el)
		return 0;
	str = el;
	if (!(ptr = strchr(str, '&'))) {
		printf("%s", el);
		return 0;
	}
	while (ptr) {
		*ptr++ = 0;
		printf("%s", str);
		str = ptr;
		if (!(ptr = strchr(str, ';'))) {
			printf("%s", str);
			break;
		} else if ((tmpptr = strchr(str, '&')) && tmpptr < ptr) {
			ptr = tmpptr;
			printf("&");
			continue;
		}
		*ptr++ = 0;
		if (!strcasecmp(str, "nbsp") || !strcasecmp(str, "#160"))
			printf(" ");
		else if (!strcasecmp(str, "lt") || !strcasecmp(str, "#60"))
			printf("<");
		else if (!strcasecmp(str, "gt") || !strcasecmp(str, "#62"))
			printf(">");
		else if (!strcasecmp(str, "amp") || !strcasecmp(str, "#38"))
			printf("&");
		else if (!strcasecmp(str, "quot") || !strcasecmp(str, "#34"))
			printf("\"");
		else if (!strcasecmp(str, "apos") || !strcasecmp(str, "#39"))
			printf("'");
		else	//暂时处理这些，好象有些符号本身gb2312就不支持
			printf("&%s;", str);
		str = ptr;
		if (!(ptr = strchr(str, '&')) && str)
			printf("%s", str);
	}
	return 1;
}

int
main(int argn, char **argv)
{
	char *src, *ptr, *el;

	setting = &setting0;

	if (argn >= 2) {
		if (atoi(argv[1]) == 1)
			setting = &setting1;
		else if (atoi(argv[1]) == 3)
			blog2board = 1;
	}

	if (argn >= 3) {
		src = readFile(argv[2]);
	} else {
		src = readStdin();
	}
	if (!src)
		return 0;

	ptr = src;
	skipHeader(&ptr);
	removeFooter(ptr);
	while (removeBlock(ptr, "<!--", "-->") >= 0) ;
	while (removeBlock(ptr, "<script", "</script>") >= 0) ;
	while (removeBlock(ptr, "<noscript", "</noscript>") >= 0) ;
	while (removeBlock(ptr, "<object", "/object>") >= 0) ;
	while (removeBlock(ptr, "<embed", "/embed>") >= 0) ;
	while (removeBlock(ptr, "<iframe", "/iframe>") >= 0) ;
	while ((el = getNextElement(&ptr))) {
		if (el[0] != '<') {
			if (blog2board)
				print_filter_html(el);
			else
				printf("%s", el);
		} else if (el[1] == '/') {
			printEndTag(el);
		} else {
			printStartTag(el);
		}
		free(el);
	}
	free(src);
	return 0;
}
