#include "an.h"
char tmp[256], buf[256], board[256];
struct boardheader brds[1000];
int brd_num;
int brd_level(char *);

int
main() {
	FILE *fp;
	chdir(MY_BBS_HOME);
	fp=fopen(".BOARDS", "r");
	brd_num=fread(&brds, sizeof(brds[0]), 1000, fp);
	fclose(fp);
	system("rm -rf " dl_path "/tmp/an.tmp; mkdir -p " dl_path "/tmp/an.tmp");
	fp=fopen("0Announce/.Search", "r");
	while(fscanf(fp, "%s %s", board, buf)>0) {
		int level;
		if(strchr(board, '*')) continue;
		board[strlen(board)-1]=0;
		level=brd_level(board);
		if(level==-1) {
			printf("skip %s\n",board);
			continue;
		}
		if(level!=0 && (level & PERM_NOZAP)==0 && (level & PERM_POSTMASK)==0){
			printf("skip2 %s\n",board);
			continue;
		}
		printf("[%d %d %d]\n", level, level & PERM_NOZAP, level & PERM_POSTMASK);
		sprintf(tmp, "%s/0Announce/%s", MY_BBS_HOME, buf);
		if(chdir(tmp)==-1) continue;
		sprintf(buf, "%s %s", exec_path, board);
		system(buf);
	}
	fclose(fp);
	return 0;
}

int brd_level(char *brd) {
	int i;
	if (!strcasecmp(board, "Personal_Corpus"))
		return -1;
	for(i=0; i<brd_num; i++) 
		if(!strcasecmp(brd, brds[i].filename)) {
			if (brds[i].flag & CLOSECLUB_FLAG) 
				return -1;
			return brds[i].level;
		}
	return -1;
}
