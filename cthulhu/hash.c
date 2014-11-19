#include "misc.h"

long
hasch (str)
	char *str;
{
	long res = 0xdeadbeef;
	size_t i;

	while (*str) {
		long l = 0;
		for (i=0;i<sizeof (long)&&*str;i++)
			l = l<<8|*str++;
		res ^= l;
	}
	return (res);
}
