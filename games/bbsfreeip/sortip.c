#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define NUM_IP 360000
struct ippp {
	unsigned int left;
	unsigned int right;
	char ds[40];
};
struct ippp ipa[ NUM_IP ];

int
cmm(struct ippp *a, struct ippp *b) {
	if (a->left < b->left)
		return -1;
	if (a->left > b->left)
		return 1;
	return 0;
}
int
main()
{
	FILE *fp;
	char buf[512], buf2[80];
	int b1, b2, b3, b4, c1, c2, c3, c4;
	int v1, v2;
	int kk=0;
	fp = fopen("ip_arrange.txt", "r");
	if (fp == 0)
		return 0;
	while (1) {
		buf[0] = 0;
		if (fgets(buf, 500, fp) <= 0)
			break;
		if (strlen(buf) < 10)
			continue;
		sscanf(buf, "%d.%d.%d.%d %d.%d.%d.%d ",
		       &b1, &b2, &b3, &b4, &c1, &c2, &c3, &c4 );
		strncpy(buf2,buf+32,39);
		v1 = b1 * 65536 * 256 + b2 * 256 * 256 + b3 * 256 + b4;
		v2 = c1 * 65536 * 256 + c2 * 256 * 256 + c3 * 256 + c4;
		ipa[kk].left = v1;
		ipa[kk].right = v2;
		strcpy(ipa[kk].ds, buf2);
		kk++;
	}
	fclose(fp);
	qsort(ipa, kk, sizeof(struct ippp), &cmm); 
	fp = fopen("ip_arrange_sort.txt", "w");
	fwrite(ipa, sizeof(struct ippp), kk, fp);
	fclose(fp);
	printf("#define\tNUM_IP\t%d\n",kk);
	return 0;
}
