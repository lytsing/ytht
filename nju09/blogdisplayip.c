#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "bbs.h"
#include "blogdisplayip.h"

// a temp value, make it from game/bbsfreeip
#define	NUM_IP	350901

static unsigned int free_addr[2000], free_mask[2000], free_num = 0;

struct ip_s {
	unsigned int left;
	unsigned int right;
	char ds[40];
};
struct ip_s *ipa;
char*
display_ip(char *ip)
{
	struct hostent *he;
	struct in_addr i;
	int addr;
	struct mmapfile mf = {ptr:NULL};

	if ((he = gethostbyname(ip)) != 0) {
		addr = *(int *) he->h_addr;
	} else {
		addr = inet_addr(ip);
	}
	i.s_addr = addr;
	if (addr == -1) {
		return "";
	}

	if (mmapfile(MY_BBS_HOME "/etc/ip_arrange_sort.txt", &mf) < 0) {
		return "";
	}
	ipa =( struct ip_s *) mf.ptr;

	return search_ip(addr);
}

char*
search_ip(unsigned int addr)
{
	char str[32];
	char *p = malloc(64);
	memset(p, 0, 64);

	unsigned int i1 = addr / (256 * 256 * 256), i2 = (addr / 65536) % 256;
	unsigned int i3 = (addr / 256) % 256, i4 = (addr % 256);
	unsigned int v =
	    i4 * 65536 * 256 + i3 * 256 * 256 + i2 * 256 + i1;
	int low = 0, high = NUM_IP - 1, mid;

	while (low < high) {
		mid = (low + high) / 2;
		if (ipa[mid].left <= v && v <= ipa[mid + 1].left) {
			if (v <= ipa[mid].right) 
				strncpy(p, ipa[mid].ds, 64);
			else
				strncpy(p, "世外桃源", 64);
			return p;
		}
		if (v < ipa[mid].left) 
			high = mid;
		else
			low = mid;
	}

	strncpy(p, "世外桃源", 64);

	return p;
}


int
get_free_list()
{
	FILE *fp;
	char buf1[100], buf2[100], buf3[100], buf[100];
	static int inited = 0, r;
	if (inited)
		return;
	inited = 1;
	fp = fopen(MY_BBS_HOME "/etc/free.txt", "r");
	if (fp == 0)
		return;
	while (1) {
		if (!fgets(buf, sizeof (buf), fp))
			break;
		r = sscanf(buf, "%s%s%s", buf1, buf2, buf3);
		if (r <= 0)
			break;
		if (r != 3)
			continue;
		free_addr[free_num] = inet_addr(buf1);
		free_mask[free_num] = inet_addr(buf2);
		free_num++;
		if (free_num >= 2000)
			break;
	}
	fclose(fp);
}

int
is_free(unsigned int x)
{
	int n;
	get_free_list();
	for (n = 0; n < free_num; n++)
		if (((x ^ free_addr[n]) | free_mask[n]) == free_mask[n])
			return 1;
	return 0;
}
