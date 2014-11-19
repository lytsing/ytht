#include "ythtlib.h"
static const struct tzoffset {
	int offset20000101;
	int offset20000801;
	const char *name;
} tzoffsetlist[] = {
#include "tzoffsetlist.c"
	{
	0, 0, NULL}
};

const char *
timezoneGuess(int offset20000101, int offset20000801)
{
	int i;
	for (i = 0; tzoffsetlist[i].name; i++) {
		if (offset20000101 == tzoffsetlist[i].offset20000101
		    && offset20000801 == tzoffsetlist[i].offset20000801)
			return tzoffsetlist[i].name;
	}
	return "";
}

char *
Ctime(time_t clock)
{
	char *tmp;
	char *ptr = ctime(&clock);
	tmp = strchr(ptr, '\n');
	if (NULL != tmp)
		*tmp = 0;
	return ptr;
}
