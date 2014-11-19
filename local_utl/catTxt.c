#include "ythtbbs.h"

FILE *fpout = NULL;

int
shouldSkip(char *str, int len)
{
	if (!strncmp(str, ": : ", min(len, 4)) ||
	    !strncmp(str, ": ¡¾ ÔÚ ", min(len, 4)) ||
	    !strncmp(str, "·¢ÐÅÕ¾: ", min(len, 8))) {
		return 1;
	}
	return 0;

}

int
printLines(char *str, int len)
{
	char *ptr, *pend;
	pend = str + len;
	while ((ptr = memchr(str, '\n', len))) {
		if (!shouldSkip(str, len)) {
			fwrite(str, ptr - str + 1, 1, fpout);
		}
		str = ptr + 1;
		len = pend - str;
	}
	if(len && !shouldSkip(str, len)) {
		fwrite(str, ptr - str + 1, 1, fpout);
	}
	return 0;
}

int
main(int argn, char **argv)
{
      struct mmapfile mf = { ptr:NULL };
	int len, attlen;
	char *ptr0, *ptr;
	
	errlog("%d %s %s", argn, argv[1], argv[2]);
	if (argn < 3) {
		errlog("NM:KLDSF:LKSDD");
		return -1;
	}
	if (mmapfile(argv[1], &mf) < 0) {
		//printf("%s\n", argv[1]);
		return -1;
	}
	fpout = fopen(argv[2], "w");
	if(!fpout)
		return -1;

	len = mf.size;

	//Remove head and tail
	ptr0 = NULL;
	//ptr0 = memmem(mf.ptr, len, "\n\n", 2);
	if (!ptr0) {
		ptr0 = mf.ptr;
	} else {
		ptr0 += 2;
		len -= (ptr0 - mf.ptr);
	}
	ptr = memmem(ptr0, len, "\n--\n", 4);
	if (ptr)
		len = ptr - ptr0;

	while (len > 0 && (ptr = memchr(ptr0, 0, len))) {
		printLines(ptr0, ptr-ptr0);
		len -= ptr - ptr0;
		ptr0 = ptr;
		attlen = ntohl(*(unsigned int *) (ptr0 + 1));
		attlen = min(len, (int) (attlen + 5));
		ptr0 += attlen;
		len -= len;
	}
	if (len > 0) {
		printLines(ptr0, len);
	}
	return 0;
}
