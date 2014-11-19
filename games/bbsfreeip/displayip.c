#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "bbs.h"
#include "displayip.h"
#include "ipnums.h"
static unsigned int free_addr[2000], free_mask[2000], free_num = 0;

struct ip_s {
	unsigned int left;
	unsigned int right;
	char ds[40];
};
struct ip_s *ipa;
int
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
		printf("\n错误的地址\n");
		return -1;
	}
	/*
	if (is_free(addr)) {
		printf
		    ("\n\n根据CERNET 2002年4月免费IP列表\n%s(%s)是一个免费ip.\n\n",
		     ip, inet_ntoa(i));
	} else {
		printf
		    ("\n\n根据CERNET 2002年4月免费IP列表\n%s(%s)不是免费ip.\n\n",
		     ip, inet_ntoa(i));
	}*/
	if (mmapfile(MY_BBS_HOME "/etc/ip_arrange_sort.txt", &mf) < 0) {
		printf("\n内部错误\n");
		return -1;
	}
	ipa =( struct ip_s *) mf.ptr;
	search_ip(addr);
	return 0;
}
int
search_ip(unsigned int addr)
{
	unsigned int i1 = addr / (256 * 256 * 256), i2 = (addr / 65536) % 256;
	unsigned int i3 = (addr / 256) % 256, i4 = (addr % 256);
	unsigned int v =
	    i4 * 65536 * 256 + i3 * 256 * 256 + i2 * 256 + i1;
	int low = 0, high = NUM_IP - 1, mid;

	while (low < high) {
		mid = (low + high) / 2;
		if (ipa[mid].left <= v && v <= ipa[mid + 1].left) {
			if (v <= ipa[mid].right) 
				printf("%s", ipa[mid].ds);
			return 0;
		}
		if (v < ipa[mid].left) 
			high = mid;
		else
			low = mid;
	}
	return 0;
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
