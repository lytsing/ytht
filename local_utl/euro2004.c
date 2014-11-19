#include "ythtbbs.h"
#define NUM1 8
#define NUM2 5
#define NUM3 10
int main() {
	struct mmapfile mf={ptr:NULL};
	int i;
	int total;
	struct fileheader *fh;
	struct fileheader newthread[NUM1], tiegan[NUM2], pinglun[NUM3];
	int nt = 0, n1 = 0, n2 = 0;
	FILE *fp;
	char *ptr;
	chdir(MY_BBS_HOME);
	MMAP_TRY {
		if (mmapfile("boards/Euro2004/.DIR", &mf) < 0)
			exit(-1);
		if (mf.size == 0)
			exit(-1);
		total = mf.size / sizeof(struct fileheader);
		fh = (struct fileheader *)(mf.ptr + (total - 1) * sizeof(struct fileheader));
		for (i = total; i > 0 && nt+n1+n2  < NUM1 + NUM2 + NUM3; i--, fh--) {
			if (fh->filetime != fh->thread)
				continue;
			if (nt < NUM1) {
				memcpy(&(newthread[nt]), fh, sizeof(struct fileheader));
				nt++;
			}
			if (fh->accessed & FH_MARKED) {
				if (strstr(fh->title, "Ìú¸Ë") && strchr(fh->title, ']')) {
					if (n1 < NUM2) {
						memcpy(&(tiegan[n1]), fh, sizeof(struct fileheader));
						n1++;
					}
				} else {
					if (n2 < NUM3) {
						memcpy(&(pinglun[n2]), fh, sizeof(struct fileheader));
						n2++;
					}
				}
			}
		}
	}
	MMAP_CATCH {
		exit(-1);
	}
	MMAP_END mmapfile(NULL, &mf);
	fp = fopen("ftphome/root/boards/Euro2004/html/newthread.js.new", "w");	
	if (fp == NULL)
		exit(-1);
	fprintf(fp, "<!--\n");
	fprintf(fp, "document.write('");
	for (i = 0; i < nt; i++) {
		fprintf(fp, "<tr>");
		fprintf(fp, "<td width=\"17\" height=\"20\"><img src=\"ball.gif\" width=\"15\" height=\"15\"></td>");
		fprintf(fp, "<td height=\"20\" colspan=\"3\">");
		fprintf(fp, "<a href=../../../../bbsnt?B=Euro2004&th=%d>", newthread[i].thread);
		fprintf(fp, "<span class=\"style7\">");
		fprintf(fp, "%s", scriptstr(newthread[i].title));
		fprintf(fp, "</span></a></td></tr>");
	}
	fprintf(fp, "')\n-->");
	fclose(fp);
	rename("ftphome/root/boards/Euro2004/html/newthread.js.new", "ftphome/root/boards/Euro2004/html/newthread.js");
	fp = fopen("ftphome/root/boards/Euro2004/html/tiegan.js.new", "w");	
	if (fp == NULL)
		exit(-1);
	fprintf(fp, "<!--\n");
	fprintf(fp, "document.write('");
	for (i = 0; i < n1; i++) {
		fprintf(fp, "<tr>");
		fprintf(fp, "<td width=\"19\">&nbsp;</td>");
		fprintf(fp, "<td width=\"15\" height=\"20\"><img src=\"ball2.gif\" width=\"15\" height=\"15\"></td>");
		fprintf(fp, "<td width=\"305\" height=\"20\" colspan=\"2\" align=\"left\">");
		fprintf(fp, "<a href=../../../../bbsnt?B=Euro2004&th=%d><span class=\"style8\">", tiegan[i].thread);
		ptr = strchr(tiegan[i].title, ']');
		fprintf(fp, "%s", scriptstr(ptr+1));
		fprintf(fp, "</span></a></td></tr>");
	}
	fprintf(fp, "')\n-->");
	fclose(fp);
	rename("ftphome/root/boards/Euro2004/html/tiegan.js.new", "ftphome/root/boards/Euro2004/html/tiegan.js");
	fp = fopen("ftphome/root/boards/Euro2004/html/pinglun.js.new", "w");	
	if (fp == NULL)
		exit(-1);
	fprintf(fp, "<!--\n");
	fprintf(fp, "document.write('");
	for (i = 0; i < n2; i++) {
		fprintf(fp, "<tr>");
		fprintf(fp, "<td width=\"18\" height=\"20\"><img src=\"ball.gif\" width=\"15\" height=\"15\"></td>");
		fprintf(fp, "<td width=\"327\" height=\"20\" colspan=\"2\">");
		fprintf(fp, "<a href=../../../../bbsnt?B=Euro2004&th=%d><span class=\"style7\">", pinglun[i].thread);
		fprintf(fp, "%s", scriptstr(pinglun[i].title));
		fprintf(fp, "</span></a></td></tr>");
	}
	fprintf(fp, "')\n-->");
	fclose(fp);
	rename("ftphome/root/boards/Euro2004/html/pinglun.js.new", "ftphome/root/boards/Euro2004/html/pinglun.js");
	return 0;
}
