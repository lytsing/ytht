#include "ythtbbs.h"

static int
fileIP(char *filename, char *ip)
{
      struct mmapfile mf = { ptr:NULL };
	int retv = 0, IPlen, len;
	char *ptr, *ptrend;
	IPlen = strlen(ip);
	if (mmapfile(filename, &mf) < 0)
		return 0;
	ptr = mf.ptr;
	ptrend = ptr + mf.size;
	len = mf.size;
	while (len > 0) {
		ptr = memmem(ptr, len, ip, IPlen);
		if (!ptr)
			break;
		if ((ptr == mf.ptr || !isdigit(ptr[-1]))
		    && (len <= IPlen || !isdigit(ptr[IPlen]))) {
			retv = 1;
			break;
		}
		ptr += IPlen;
		len = ptrend - ptr;
	}
	mmapfile(NULL, &mf);
	return retv;
}

int
invalid_voteIP(char *ip)
{
	int i;
	char *(files[4]) = {
	MY_BBS_HOME "/etc/untrust",
	MY_BBS_HOME "/etc/untrust.bbsfulllist",
		    MY_BBS_HOME "/etc/bbsnetA.ini",
		    MY_BBS_HOME "/etc/bbsnet.ini"};
	for (i = 0; i < 4; i++) {
		if (fileIP(files[i], ip))
			return -1;
	}
	return 0;
}
