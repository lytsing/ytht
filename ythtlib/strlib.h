/* strlib.c */
#ifndef __STRLIB_H
#define __STRLIB_H
struct memline {
	char *text;
	int len;
	int size;
};

char *strnstr(const char *haystack, const char *needle, size_t haystacklen);
char *strncasestr(const char *haystack, const char *needle, size_t haystacklen);
void memlineinit(struct memline *mlp, char *ptr, int size);
int memlinenext(struct memline *mlp);
#endif
