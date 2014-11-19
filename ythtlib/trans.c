//从原始的convcode转换出新的次序的码表, 以便可以更快地运算
#include <stdio.h>
#include "convcode.h"
#include "convcode.c"
struct {
	int gbch1;
	int gbch2[2];
	int bigch1;
	int bigch2[2];
} extratab[]={
	{164, {161,184}, 198, {231, 254}}, //日文, 下列, 到TEL以上, mozilla支持, 但是IE不支持; 不过比什么都不是强点
	{164, {185,243}, 199, {64, 112}}, 
	{165, {161,164}, 199, {123, 126}},
	{165, {165,246}, 199, {161, 242}},
	{167, {161, 172}, 199,{243,254}}, //俄文
	{167, {173, 193}, 200, {64, 84}}, 
	{167, {209, 241}, 200, {85, 117}}, 
	{168, {89, 89}, 200, {211, 211}}, //TEL
	{231, {219, 219}, 163, {177, 177}}, //幺
	{0}
};

//clist: {简体, 繁体}
int clist[][4]= {
	{161, 169, 198, 224}, //々, mozilla is happy
	{214, 187, 165, 117}, //只
	{0}
};

int
correctb2g(unsigned char c[2], unsigned char cc[2])
{
	int i;
	for(i=0;clist[i][0];i++){
		if(c[0]==clist[i][2]&&c[1]==clist[i][3]) {
			cc[0]=clist[i][0];
			cc[1]=clist[i][1];
			return 0;
		}
	}
	return -1;
}

int
correctg2b(unsigned char c[2], unsigned char cc[2])
{
	int i;
	for(i=0;clist[i][0];i++){
		if(c[0]==clist[i][0]&&c[1]==clist[i][1]) {
			cc[0]=clist[i][2];
			cc[1]=clist[i][3];
			return 0;
		}
	}
	return -1;
}

int
extrab2g(unsigned char ch[2])
{
	int i;
	for(i=0;extratab[i].gbch1;i++) {
		if(ch[0]!=extratab[i].bigch1)
			continue;
		if(ch[1]>=extratab[i].bigch2[0]&&
				ch[1]<=extratab[i].bigch2[1]) {
			ch[0]=extratab[i].gbch1;
			ch[1]=extratab[i].gbch2[0]+ch[1]-extratab[i].bigch2[0];
			return 0;
		}
	}
	ch[0]=BtoG_bad1;
	ch[1]=BtoG_bad2;
	return -1;
}

int
extrag2b(unsigned char ch[2])
{
	int i;
	for(i=0;extratab[i].gbch1;i++) {
		if(ch[0]!=extratab[i].gbch1)
			continue;
		if(ch[1]>=extratab[i].gbch2[0]&&
				ch[1]<=extratab[i].gbch2[1]) {
			ch[0]=extratab[i].bigch1;
			ch[1]=extratab[i].bigch2[0]+ch[1]-extratab[i].gbch2[0];
			return 0;
		}
	}
	ch[0]=GtoB_bad1;
	ch[1]=GtoB_bad2;
	return -1;
}

int
main()
{
	unsigned char c[2], cc[2];
	int i;
	printf("//g2btab[c2-0x40][(c1-0x80)*2], g2btab[c2-0x40][(c1-0x80)*2+1]\n");
	printf("//c1: 0x80-0xff, c2: 0x40-0xfe\n");
	printf("static const char g2btab[192][256]={\n");
	for(c[1] = 0x40; /*c[1]<= 0xff &&*/c[1]; c[1]++) {
		printf("\t{");
		for(i=0, c[0]=0x80; /*c[0]<=0xff&&*/c[0]; c[0]++, i++) {
			cc[0]=c[0];
			cc[1]=c[1];
			g2b(cc);
			correctg2b(c, cc);
			if(cc[0]==GtoB_bad1&&cc[1]==GtoB_bad2){
				cc[0]=c[0];
				cc[1]=c[1];
				extrag2b(cc);
			}
			printf("%3d,%3d%s", (int)cc[0], (int)cc[1], c[0]==0xff?"":",");
			if(i%10==9) printf("\n\t ");
		}
		printf("}%s\n", c[1]==0xff?"":",");
	}
	printf("};\n");
	printf("//b2gtab[c2-0x40][(c1-0x80)*2], b2gtab[c2-0x40][(c1-0x80)*2+1]\n");
	printf("//c1: 0x80-0xff, c2: 0x40-0xfe\n");
	printf("static const char b2gtab[192][256]={\n");
	for(c[1] = 0x40; c[1]<= 0xff&&c[1]; c[1]++) {
		printf("\t{");
		for(i=0, c[0]=0x80; c[0]<=0xff&&c[0]; c[0]++, i++) {
			cc[0]=c[0];
			cc[1]=c[1];
			b2g(cc);
			correctg2b(c, cc);
			if(cc[0]==BtoG_bad1&&cc[1]==BtoG_bad2) {
				cc[0]=c[0];
				cc[1]=c[1];
				extrab2g(cc);
			}
			printf("%3d,%3d%s", (int)cc[0], (int)cc[1], c[0]==0xff?"":",");
                        if(i%10==9) printf("\n\t ");
		}
		printf("}%s\n", c[1]==0xff?"":",");
	}
	printf("};\n");
	return 0;
}
