#include <time.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "bbs.h"
#include "winmine.h"

#define WINMINEPATH MY_BBS_HOME "/etc/winmine/"
int a[32][18];			//��
int m[32][18];			//marked
int o[32][18];			//opened
char topID[20][20], topFROM[20][20], prize[20][3];
char userid[20] = "unknown.", fromhost[20] = "unknown.";
int topT[20], gameover = 0;
static char buf[10000];		// output buffer
struct termios oldtty, newtty;
int colorful = 1;

int
main(int n, char *cmd[])
{
	tcgetattr(0, &oldtty);
	cfmakeraw(&newtty);
	tcsetattr(0, TCSANOW, &newtty);
	if (n >= 2)
		strsncpy(userid, cmd[1], sizeof(userid));
	if (n >= 3)
		strsncpy(fromhost, cmd[2], sizeof(fromhost));
	winminelog("ENTER");
	winmine();
	tcsetattr(0, TCSANOW, &oldtty);
	return 0;
}

void
winmine()
{
	int x, y;
	win_showrec();
	clear();
	prints("Enable ANSI color?[Y/N]");
	refresh();
	if (strchr("Nn", egetch()))
		colorful = 0;
	while (1) {
		clear();
		for (x = 0; x <= 31; x++)
			for (y = 0; y <= 17; y++) {
				a[x][y] = 0;
				m[x][y] = 0;
				o[x][y] = 0;
			}
		winrefresh();
		winloop();
		pressanykey();
	}
}

int
num_mine_beside(int x1, int y1)
{
	int dx, dy, s;
	s = 0;
	for (dx = x1 - 1; dx <= x1 + 1; dx++)
		for (dy = y1 - 1; dy <= y1 + 1; dy++)
			if (!(dx == x1 && dy == y1) && a[dx][dy])
				s++;
	return s;
}

int
num_mark_beside(int x1, int y1)
{
	int dx, dy, s;
	s = 0;
	for (dx = x1 - 1; dx <= x1 + 1; dx++)
		for (dy = y1 - 1; dy <= y1 + 1; dy++)
			if (!(dx == x1 && dy == y1) && m[dx][dy])
				s++;
	return s;
}

void
wininit(int x1, int y1)
{
	int n, x, y;
	srand(time(NULL) + getpid());
	for (n = 1; n <= 99; n++) {
		do {
			x = rand() % 30 + 1;
			y = rand() % 16 + 1;
		}
		while (a[x][y] != 0 || (abs(x - x1) < 2 && abs(y - y1) < 2));
		a[x][y] = 1;
	}
}

/* ˫�� */
void
dblclick(int x, int y)
{
	int dx, dy;
	if (x < 1 || x > 30 || y < 1 || y > 16)
		return;
	if (!o[x][y])
		return;
	if (num_mine_beside(x, y) != num_mark_beside(x, y))
		return;
	for (dx = x - 1; dx <= x + 1; dx++)
		for (dy = y - 1; dy <= y + 1; dy++)
			windig(dx, dy);
}

/* ��� */
void
windig(int x, int y)
{
	int dx, dy;
	if (x < 1 || x > 30 || y < 1 || y > 16)
		return;
	if (o[x][y] || m[x][y])
		return;
	o[x][y] = 1;
	winsh(x, y);
	if (a[x][y]) {
		gameover = 1;
		return;
	}
	if (num_mine_beside(x, y) == 0) {
		for (dx = x - 1; dx <= x + 1; dx++)
			for (dy = y - 1; dy <= y + 1; dy++)
				windig(dx, dy);
	}
}

/* ��ʾ[x][y]�� */
void
winsh(int x, int y)
{
	move(x * 2 - 2, y - 1);
	winsh0(x, y);
}

/* ͬ��, �ӿ��ٶ� */
void
winsh0(int x, int y)
{
	int c, d;
	static char word[9][10] = {
		"��", "��", "��", "��", "��", "��", "��", "��", "��"
	};
	static int cc[9] = { 38, 37, 32, 31, 33, 35, 36, 40, 39 };
	char buf[100];
	if (!o[x][y] && !m[x][y]) {
		prints("��");
		return;
	}
	if (m[x][y]) {
		prints("��");
		return;
	}
	if (a[x][y]) {
		prints("[1;31m��[m");
		return;
	}
	c = num_mine_beside(x, y);
	d = 1;
	if (c == 0)
		d = 0;
	if (colorful)
		sprintf(buf, "[%d;%dm%s[m", d, cc[c], word[c]);
	else
		strcpy(buf, word[c]);
	prints(buf);
}

void
winloop()
{
	int x, y, c, marked, t0, inited;
	char buf[100];
	x = 10;
	y = 8;
	inited = 0;
	marked = 0;
	clearbuf();
	t0 = time(0);
	while (1) {
		c = egetch();
		if (c == 257 && y > 1)
			y--;
		if (c == 258 && y < 16)
			y++;
		if (c == 260 && x > 1)
			x--;
		if (c == 259 && x < 30)
			x++;
		move(0, 20);
		sprintf(buf, "ʱ��: %d ", (int) (time(0) - t0));
		prints(buf);
		move(40, 20);
		sprintf(buf, "���: %d ", marked);
		prints(buf);
		move(0, 22);
		sprintf(buf, "����: %3d, %3d", x, y);
		prints(buf);
		move(x * 2 - 2, y - 1);
		if (c == 'h' || c == 'H')
			winhelp();
		if (c == 'd' || c == 'D')
			winrefresh();
		if (c == 'a' || c == 'A') {
			if (!inited) {
				wininit(x, y);
				inited = 1;
			}
			dig(x, y);
		}
		if ((c == 83 || c == 115) && !o[x][y]) {
			if (m[x][y]) {
				m[x][y] = 0;
				marked--;
			} else {
				m[x][y] = 1;
				marked++;
			}
			winsh(x, y);
		}
		if (checkwin() == 1) {
			move(0, 22);
			prints("ף���㣡��ɹ��ˣ�                    ");
			{
				char buf[100];
				sprintf(buf, "finished in %d s.",
					(int) (time(0) - t0));
				win_checkrec(time(0) - t0);
				winminelog(buf);
			}
			gameover = 0;
			return;
		}
		if (gameover) {
			move(0, 22);
			prints("���ź�����ʧ����... ����һ�ΰɣ�    ");
			{
				char buf[100];
				sprintf(buf, "failed in %d s.",
					(int) (time(0) - t0));
				winminelog(buf);
			}
			gameover = 0;
			return;
		}
		move(x * 2 - 2, y - 1);
		refresh();
	}
}

int
checkwin()
{
	int x, y, s;
	s = 0;
	for (x = 1; x <= 30; x++)
		for (y = 1; y <= 16; y++)
			if (!o[x][y])
				s++;
	if (s == 99)
		return 1;
	return 0;
}

void
dig(int x, int y)
{
	if (!o[x][y])
		windig(x, y);
	else
		dblclick(x, y);
}

void
winrefresh()
{
	int x, y;
	clear();
	move(0, 23);
	prints
	    ("[1;32mɨ��[mfor bbs v1.00 by zhch.nju 00.3, press '[1;32mh[m' to get help, '[1;32m^C[m') to exit.");
	for (y = 1; y <= 16; y++) {
		move(0, y - 1);
		for (x = 1; x <= 30; x++)
			winsh0(x, y);
	}
	refresh();
}

void
winhelp()
{
	clear();
	prints
	    ("==��ӭ��[1;35m" MY_BBS_NAME
	     "[m�����ɨ����Ϸ==\r\n---------------------------------\\r\n\r\n");
	prints("�淨�ܼ򵥣���[1;34mwindows[m�µ����ɨ�ײ��.\r\n");
	prints
	    ("  '[1;32mA[m'���������൱�����������˫�������ã� �������������λ��\r\n");
	prints("  �Զ��ж�Ҫ�������ֲ�����\r\n");
	prints("  '[1;32mS[m'�����൱������Ҽ��Ĺ���, ����������.\r\n");
	prints("  '[1;32mH[m'��������ʾ��������Ϣ.\r\n");
	prints("  '[1;32mQ[m'���˳���Ϸ.\r\n");
	prints("  ����Ļ�ҵ�ʱ������'[1;32mD[m'������ˢ����Ļ��\r\n");
	prints
	    ("������[1;32mNetterm[m����(��ȻnjutermҲ����,:)),[1;32mtelnet[mЧ������̫��\r\n");
	prints("��һ�ε��һ���ῪһƬ��������ɡ�\r\n");
	prints("�������ٶȻ��Ǻܿ�ģ��������Դﵽ���ɨ�׵��ٶ�.\r\n");
	pressanykey();
	winrefresh();
}

void
win_loadrec()
{
	FILE *fp;
	int n;
	for (n = 0; n <= 19; n++) {
		strcpy(topID[n], "null.");
		topT[n] = 999;
		strcpy(topFROM[n], "unknown.");
		strcpy(prize[n], "NA");
	}
	fp = fopen(WINMINEPATH "mine2.rec", "r");
	if (fp == NULL) {
		win_saverec();
		return;
	}
	for (n = 0; n <= 19; n++)
		fscanf(fp, "%s %d %s %s\n", topID[n], &topT[n], topFROM[n], prize[n]);
	fclose(fp);
}

void
win_saverec()
{
	FILE *fp;
	int n;
	fp = fopen(WINMINEPATH "mine2.rec", "w");
	if (!fp)
		return;
	for (n = 0; n <= 19; n++) {
		fprintf(fp, "%s %d %s %s\n", topID[n], topT[n], topFROM[n], prize[n]);
	}
	fclose(fp);
}

void
win_showrec()
{
	char buf[200];
	int n;
	win_loadrec();
	clear();
	prints
	    ("[44;37m                         -" MY_BBS_NAME " BBS ɨ�����а�-                               \r\n[m");
	prints
	    ("[41m ����       ����        ��ʱ                      ����                     �齱[m\r\n");
	for (n = 0; n <= 19; n++) {
		sprintf(buf, "[1;37m%3d[32m%13s[0;37m%12d[m%29s\033[33m%20s\r\n", n + 1,
			topID[n], topT[n], topFROM[n], prize[n]);
		prints(buf);
	}
	sprintf(buf,
		"[41m                                                                               [m\r\n");
	prints(buf);
	pressanykey();
}

void
win_checkrec(int dt)
{
	int n;
	win_loadrec();
	for (n = 0; n <= 19; n++)
		if (!strcmp(topID[n], userid)) {
			if (dt < topT[n]) {
				topT[n] = dt;
				strsncpy(topFROM[n], fromhost, sizeof(topFROM[0]));
				strcpy(prize[n], "δ");
				win_sort();
			}
			return;
		}
	if (dt < topT[19]) {
		strsncpy(topID[19], userid, sizeof(topID[0]));
		topT[19] = dt;
		strsncpy(topFROM[19], fromhost, sizeof(topFROM[0]));
		strcpy(prize[19], "δ");
		win_sort();
		return;
	}
}

void
win_sort()
{
	int n, n2, tmp;
	char tmpID[sizeof(topID[0])];
	for (n = 0; n <= 18; n++) {
		for (n2 = n + 1; n2 <= 19; n2++) {
			if (topT[n] > topT[n2]) {
				tmp = topT[n];
				topT[n] = topT[n2];
				topT[n2] = tmp;
				strcpy(tmpID, topID[n]);
				strcpy(topID[n], topID[n2]);
				strcpy(topID[n2], tmpID);
				strcpy(tmpID, topFROM[n]);
				strcpy(topFROM[n], topFROM[n2]);
				strcpy(topFROM[n2], tmpID);
				strcpy(tmpID, prize[n]);
				strcpy(prize[n], prize[n2]);
				strcpy(prize[n2], tmpID);
			}
		}
	}
	win_saverec();
	clear();
	prints("ף��! ��ˢ�����Լ��ļ�¼!\r\n");
	pressanykey();
	win_showrec();
}

void
clear()
{
	prints("[H[J");
}

void
refresh()
{
	write(0, buf, strlen(buf));
	buf[0] = 0;
}

void
prints(char *b)
{
	strcat(buf, b);
}

void
move(int x, int y)
{
	char c[100];
	sprintf(c, "[%d;%dH", y + 1, x + 1);
	prints(c);
}

int
egetch()
{
	int c, d, e;
	c = getch0();
	if (c == 3 || c == 4 || c == -1)
		quit();
	if (c != 27)
		return c;
	d = getch0();
	e = getch0();
	if (e == 'A')
		return 257;
	if (e == 'B')
		return 258;
	if (e == 'C')
		return 259;
	if (e == 'D')
		return 260;
	return 0;
}

int
getch0()
{
	char c;
	if (read(0, &c, 1) <= 0)
		quit();
	return c;
}

void
quit()
{
	tcsetattr(0, TCSANOW, &oldtty);
	clear();
	refresh();
	winminelog("QUIT");
	exit(0);
}

void
pressanykey()
{
	refresh();
	clearbuf();
}

void
clearbuf()
{
	char buf[128];
	refresh();
	read(0, buf, 100);
}

void
winminelog(char *cc)
{
	FILE *fp;
	time_t t;
	t = time(0);
	fp = fopen(WINMINEPATH "winmine.log", "a");
	fprintf(fp, "%s did %s on %s", userid, cc, ctime(&t));
	fclose(fp);
}
