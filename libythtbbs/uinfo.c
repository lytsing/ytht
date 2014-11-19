#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ythtbbs.h"
int
isreject(struct user_info *uentp, struct user_info *reader)
{
	int i;

	if (USERPERM(reader, PERM_SYSOP))
		return 0;
	if (uentp->uid == reader->uid)
		return 0;
	for (i = 0; i < MAXREJECTS && uentp->reject[i]; i++) {
		if (uentp->reject[i] == reader->uid)
			return 1;	/* 被设为黑名单 */
	}
	for (i = 0; i < MAXREJECTS && reader->reject[i]; i++) {
		if (uentp->uid == reader->reject[i])
			return 1;	/* 被设为黑名单 */
	}
	return 0;
}

unsigned int
utmp_iphash(char *fromhost)
{
	struct in_addr addr;
	inet_aton(fromhost, &addr);
	return (addr.s_addr % UTMPHASH_SIZE) + 1;
}
