#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/times.h>
#include "bbsconfig.h"
#include "tt.h"

#define RECORDFILE MY_BBS_HOME "/etc/tt/tt.dat"
char *userid, *from;
char tt_id[20][20], tt_ip[20][20], prize[20][3];
int tt_scr[20];

int
tt_load_record()
{
	int n;
	FILE *fp = fopen(RECORDFILE, "r");
	if (fp == 0) {
		fp = fopen(RECORDFILE, "w");
		for (n = 0; n < 20; n++)
			fprintf(fp, "%s %d %s %s\n", "null.", 0, "unknown.", "NA");
		fclose(fp);
		fp = fopen(RECORDFILE, "r");
	}
	for (n = 0; n < 20; n++)
		fscanf(fp, "%s %d %s %s", tt_id[n], &tt_scr[n], tt_ip[n], prize[n]);
	fclose(fp);
	return 0;
}

int
tt_save_record()
{
	int n, m1, m2;
	char id[20], ip[30];
	int scr;
	FILE *fp = fopen(RECORDFILE, "w");
	for (m1 = 0; m1 < 20; m1++)
		for (m2 = m1 + 1; m2 < 20; m2++)
			if (tt_scr[m1] < tt_scr[m2]) {
				strcpy(id, tt_id[m1]);
				strcpy(ip, tt_ip[m1]);
				scr = tt_scr[m1];
				strcpy(tt_id[m1], tt_id[m2]);
				strcpy(tt_ip[m1], tt_ip[m2]);
				tt_scr[m1] = tt_scr[m2];
				strcpy(tt_id[m2], id);
				strcpy(tt_ip[m2], ip);
				tt_scr[m2] = scr;
				strcpy(id, prize[m1]);
				strcpy(prize[m1], prize[m2]);
				strcpy(prize[m2], id);
			}
	for (n = 0; n < 20; n++)
		fprintf(fp, "%s %d %s %s\n", tt_id[n], tt_scr[n], tt_ip[n], prize[n]);
	fclose(fp);
	return 0;
}

int
tt_check_record(int score)
{
	int n;
	tt_load_record();
	for (n = 0; n < 20; n++)
		if (!strcasecmp(tt_id[n], userid)) {
			if (tt_scr[n] > score)
				return 0;
			tt_scr[n] = score;
			strncpy(tt_ip[n], from, 16);
			tt_ip[n][16] = 0;
			strcpy(prize[n], "δ");
			tt_save_record();
			return 1;
		}
	if (tt_scr[19] < score) {
		tt_scr[19] = score;
		strcpy(tt_id[19], userid);
		strncpy(tt_ip[19], from, 16);
		tt_ip[19][16] = 0;
		strcpy(prize[19], "δ");
		tt_save_record();
		return 1;
	}
	return 0;
}

int
main(int argn, char **argv)
{
	if (argn >= 3) {
		userid = argv[1];
		from = argv[2];
	} else {
		userid = "����";
		from = "δ֪";
	}
	tt_game();
	return 0;
}

int
tt_game()
{
	char c[30], a, genbuf[60];
	char chars[] = "���£ãģţƣǣȣɣʣˣ̣ͣΣϣУѣңӣԣգ֣ףأ٣�";
	int m, n, t, score, retv;
	srand(getpid() + time(NULL));
	//randomize();
	//modify_user_mode(BBSNET);
	//report("tt game");
	tt_load_record();
        printf("\033[2J\033[44;37m                       -" MY_BBS_NAME " BBS ���ָ������а�-                             \r\n\033[m");
	printf("\033[41m ����       ����        �ٶ�(WPM)                 ����                     �齱\033[m\r\n");
	for (n = 0; n <= 19; n++) {
		printf("\033[1;37m%3d\033[32m%13s\033[0;37m%14.2f\033[m%29s\033[33m%18s\r\n", n + 1,
				tt_id[n], tt_scr[n]/10., tt_ip[n], prize[n]);
	}
	printf("\033[41m                                                                               \033[m\r\n"); 
//	printf
//	    ("[2J��վ���ָ������а�\r\n\r\n%4s %12.12s %24.24s %5s(WPMs)\r\n",
//	     "����", "�ʺ�", "��Դ", "�ٶ�");
//	for (n = 0; n < 20; n++)
//		printf("%4d %12.12s %24.24s %5.2f\r\n", n + 1, tt_id[n],
//		       tt_ip[n], tt_scr[n] / 10.);
	fflush(stdout);
	read(0, genbuf, 32);
	printf
	    ("[2J[1;32m�££�[m������ϰ����. (��Сд����, �����һ�ַ�ǰ�ĵȴ�����ʱ. [1;32m^C[m or [1;32m^D[m �˳�.)\r\n\r\n\r\n");

      start:
	for (n = 0; n < 30; n++) {
		c[n] = rand() % 26;
		printf("%c%c", chars[c[n] * 2], chars[c[n] * 2 + 1]);
	}
	printf("\r\n");
	fflush(stdout);
	m = 0;
	t = times(0);
	while (m < 30) {
		while ((retv = read(0, genbuf, 32)) > 1) ;
		if (retv <= 0)
			return 0;
		if (m == 0 && abs(times(0) - t) > 300) {
			printf
			    ("\r\n��ʱ! �������[1;32m3[m�������ڿ�ʼ!\r\n");
			goto start;
		}
		a = genbuf[0];
		if (a == c[m] + 65 || a == c[m] + 97) {
			printf("%c%c", chars[c[m] * 2], chars[c[m] * 2 + 1]);
			if (m == 0) {
				t = times(0);
			}
			m++;
			fflush(stdout);
			usleep(60000);
		}
		if (genbuf[0] == 3 || genbuf[0] == 4)
			return 0;
	}
	score = 360000 / (times(0) - t);
	printf("\r\n\r\nSpeed=%5.2f WPMs\r\n\r\n", score / 10.);
	if (tt_check_record(score))
		printf("[1;33mף�أ���ˢ�����Լ��ļ�¼��[m\r\n\r\n");
	fflush(stdout);
	goto start;
}
