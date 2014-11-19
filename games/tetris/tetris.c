#include <termios.h>
#include <sys/times.h>
#include "bbs.h"
#include "tetris.h"
#define TETRISPATH MY_BBS_HOME "/etc/tetris"
#define stty(fd, data) tcsetattr( fd, TCSANOW, data )
#define gtty(fd, data) tcgetattr( fd, data )
struct termios tty_state, tty_new;
static int oy, ox, ok, on = -1;
int a[21][12] = {
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8},
	{8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8}
};
unsigned char buf[4000], userid[30] = "null.", userip[60] = "unknown.";
int bufip = 0;
int dx[7][4][4], dy[7][4][4];
int d[7][4][4] = {
	{{0, 1, 4, 5}, {0, 1, 4, 5}, {0, 1, 4, 5}, {0, 1, 4, 5}},
	{{4, 5, 6, 7}, {1, 5, 9, 13}, {4, 5, 6, 7}, {1, 5, 9, 13}},
	{{0, 1, 5, 6}, {1, 4, 5, 8}, {0, 1, 5, 6}, {1, 4, 5, 8}},
	{{1, 2, 4, 5}, {0, 4, 5, 9}, {1, 2, 4, 5}, {0, 4, 5, 9}},
	{{0, 1, 2, 4}, {0, 1, 5, 9}, {2, 4, 5, 6}, {0, 4, 8, 9}},
	{{0, 1, 2, 6}, {1, 5, 8, 9}, {0, 4, 5, 6}, {0, 1, 4, 8}},
	{{0, 1, 2, 5}, {1, 4, 5, 9}, {1, 4, 5, 6}, {0, 4, 5, 8}}
};
int k, n, y, x, e;
int newk = 0;
int lines = 0;
char tmp[200];
int delay, level;
char topID[20][20], topFROM[20][20], prize[20][3];
int topT[20];
int starttime;

int
get_tty()
{
	if (gtty(1, &tty_state) < 0)
		return 0;
	return 1;
}

void
init_tty()
{
	long vdisable;

	memcpy(&tty_new, &tty_state, sizeof (tty_new));
	tty_new.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ISIG);
	tty_new.c_cflag &= ~CSIZE;
	tty_new.c_cflag |= CS8;
	tty_new.c_cc[VMIN] = 1;
	tty_new.c_cc[VTIME] = 0;
	if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) >= 0) {
		tty_new.c_cc[VSTART] = vdisable;
		tty_new.c_cc[VSTOP] = vdisable;
		tty_new.c_cc[VLNEXT] = vdisable;
	}
	tcsetattr(1, TCSANOW, &tty_new);
}

void
reset_tty()
{
	stty(1, &tty_state);
}

int
getch()
{
	int c, d, e;
	c = getch0();
	if (c == 3 || c == 4 || c == -1)
		quit();
	if (c != 27)
		return c;
	d = getch0();
	e = getch0();
	if (d == 0)
		return c;
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
	char ch;
	fd_set rfds;
	struct timeval tv;
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 50000;
	if (select(1, &rfds, NULL, NULL, &tv)) {
		if (read(0, &ch, 1) <= 0)
			exit(-1);
		return ch;
	} else
		return 0;
}

void
prints(char *s)
{
	sprintf(buf + bufip, s);
	bufip += strlen(s);
}

void
oflush()
{
	write(1, buf, bufip);
	bufip = 0;
}

void
move(int y1, int x1)
{
	sprintf(tmp, "[%d;%dH", y1 + 3, x1 + 1);
	prints(tmp);
}

void
color(int c)
{
	static int lastc = -1;
	if (c == lastc)
		return;
	lastc = c;
	if (c == 4)
		c = 12;
	sprintf(tmp, "[%d;%dm", c / 8, c % 8 + 30);
	prints(tmp);
}

void
clear()
{
	sprintf(tmp, "[H[J");
	prints(tmp);
}

void
clear2()
{
	sprintf(tmp, "[H[J");
	prints(tmp);
	color(8);
	move(3, 0);
	prints("                            [33m©°©¤©´     \r\n");
	prints("                            ©¦£Ô©¦     \r\n");
	prints("                            ©¸©¤©¼     \r\n");
	prints("                            [34m©°©¤©´     \r\n");
	prints("                            ©¦£Å©¦     \r\n");
	prints("                            ©¸©¤©¼     \r\n");
	prints("                            [33m©°©¤©´     \r\n");
	prints("                            ©¦£Ô©¦     \r\n");
	prints("                            ©¸©¤©¼     \r\n");
	prints("                            [35m©°©¤©´     \r\n");
	prints("                            ©¦£Ò©¦     \r\n");
	prints("                            ©¸©¤©¼     \r\n");
	prints("                            [31m©°©¤©´     \r\n");
	prints("                            ©¦£É©¦     \r\n");
	prints("                            ©¸©¤©¼     \r\n");
	prints("                            [32m©°©¤©´     \r\n");
	prints("                            ©¦£Ó©¦     \r\n");
	prints("                            ©¸©¤©¼[m     \r\n");

	prints("Press '^C', if you want to quit.");
	move(-2, 0);
}

void
sh2()
{
	if (oy == y && ox == x && ok == k && on == n)
		return;
	sh(oy, ox, ok, on, 0);
	oy = y;
	ox = x;
	ok = k;
	on = n;
	sh(oy, ox, ok, on, ok + 1);
	oflush();
}

void
sh(int y, int x, int k, int n, int c)
{
	if (n == -1)
		return;
	for (e = 0; e <= 3; e++) {
		move(y + dy[k][n][e], 2 * (x + dx[k][n][e]));
		color(c);
		if (c)
			prints("¡ö");
		else
			prints("  ");
	}
	move(-2, 0);
}

void
show0()
{
	int y, x;
	for (y = 0; y <= 20; y++) {
		move(y, 0);
		for (x = 0; x <= 11; x++) {
			color(a[y][x]);
			if (a[y][x])
				prints("¡ö");
			else
				prints("  ");
		}
	}
	oflush();
}

int
main(int argc, char *argv[])
{
	if (argc >= 2)
		strcpy(userid, argv[1]);
	if (argc >= 3)
		strcpy(userip, argv[2]);
	starttime = time(0);
	get_tty();
	init_tty();
	intr();
	start();
	quit();
	return 0;
}

void
quit()
{
	reset_tty();
	color(7);
	sprintf(tmp, "Quit. Stay time: %d", (int) (time(0) - starttime));
	tetrislog(tmp);
	printf("[H[J»¶Ó­³£À´, ÔÙ¼û!\r\n");
	sleep(1);
	fflush(stdout);
	usleep(500000);
	oflush();
	exit(0);
}

void
init_data()
{
	for (k = 0; k <= 6; k++)
		for (n = 0; n <= 3; n++)
			for (e = 0; e <= 3; e++) {
				dx[k][n][e] = d[k][n][e] % 4;
				dy[k][n][e] = d[k][n][e] / 4;
			}
	for (y = 0; y <= 19; y++)
		for (x = 1; x <= 10; x++) {
			a[y][x] = 0;
		}
	srand(time(0));
	newk = rand() % 7;
	level = 0;
	delay = 20;
	lines = 0;
}

int
crash2(int x, int y, int k, int n)
{
	for (e = 0; e <= 3; e++)
		if (a[y + dy[k][n][e]][x + dx[k][n][e]])
			return 1;
	return 0;
}

void
bell()
{
	char c = 7;
	write(0, &c, 1);
}

void
start()
{
	int c, t, first;
	win_showrec();
	while (1) {
		init_data();
		clear2();
		show0();
		first = 1;
		while (1) {
			k = newk;
			newk = rand() % 7;
			n = 0;
			color(0);
			move(0, 25);
			prints("                ");
			move(1, 25);
			prints("                ");
			sh(0, 14, newk, 0, newk + 1);
			n = 0;
			x = 3;
			y = 0;
			sh2();
			if (first) {
				presskey();
				first = 0;
			}
			if (crash2(x, y, k, n)) {
				sprintf(tmp, "%d", lines);
				tetrislog(tmp);
				win_checkrec(lines);
				break;
			}
			t = times(0);
			while (1) {
				c = getch();
				if (c == 27)
					presskey();
				if (c == 260 || c == 'a' || c == 'A')
					if (!crash2(x - 1, y, k, n)) {
						x--;
						sh2();
					}
				if (c == 259 || c == 's' || c == 'S')
					if (!crash2(x + 1, y, k, n)) {
						x++;
						sh2();
					}
				if (c == 'b' || c == 'B' || c == 10)
					if (!crash2(x, y, k, (n + 1) % 4)) {
						n = (n + 1) % 4;
						sh2();
					}
				if (c == 'h' || c == 'H' || c == 257)
					if (!crash2(x, y, k, (n + 3) % 4)) {
						n = (n + 3) % 4;
						sh2();
					}
				if (c == 'J' || c == 'j')
					if (!crash2(x, y, k, (n + 2) % 4)) {
						n = (n + 2) % 4;
						sh2();
					}
				if (c == ' ') {
					while (!crash2(x, y + 1, k, n))
						y++;
					sh2();
					down();
					break;
				}
				if (times(0) - t > delay || c == 258 || c == 'z'
				    || c == 'Z') {
					t = times(0);
					if (crash2(x, y + 1, k, n)) {
						down();
						break;
					} else {
						y++;
						sh2();
					}
				}
			}
		}
	}
}

void
down()
{
	for (e = 0; e <= 3; e++)
		a[y + dy[k][n][e]][x + dx[k][n][e]] = k + 1;
	checklines();
	on = -1;
}

void
checklines()
{
	int y1, x1;
	int y2;
	int s;
	s = 0;
	for (y1 = 0; y1 <= 19; y1++) {
		for (x1 = 1; x1 <= 10; x1++)
			if (a[y1][x1] == 0)
				break;
		if (x1 <= 10)
			continue;
		s = 1;
		printf("[37m[24;1HLines =[32m%3d", lines + 1);
		if (lines == 0)
			printf("                         ");
		for (y2 = y1; y2 >= 1; y2--)
			for (x1 = 1; x1 <= 10; x1++)
				a[y2][x1] = a[y2 - 1][x1];
		for (x1 = 1; x1 <= 10; x1++)
			a[0][x1] = 0;
		if ((++lines) % 30 == 0) {
			delay *= .9;
			level++;
			bell();
			printf("[37m[24;15HLevel =[32m%3d", level);
		}
	}
	if (s) {
		show0();
		fflush(stdout);
	}
}

void
tetrislog(char *cc)
{
	FILE *fp;
	time_t t;
	t = time(0);
	fp = fopen(TETRISPATH "/tetris.log", "a");
	fprintf(fp, "%s did %s on %s", userid, cc, ctime(&t));
	fclose(fp);
}

void
intr()
{
	clear();
	oflush();
	sprintf(tmp,
		"»¶Ó­¹âÁÙ %s. ÄúÊÇ×Ô2001Äê2ÔÂ25ÈÕ±¾ÓÎÏ·µÄµÚ [1;33m%d[m Î»·ÃÎÊÕß.\r\n\r\n",
		userid, count());
	prints(tmp);
	prints("¼üÅÌÉèÖÃ: \r\n");
	prints("×ó: '[1;32ma[m', '[1;32m¡û[m';\r\n");
	prints("ÓÒ: '[1;32ms[m', '[1;32m¡ú[m';\r\n");
	prints("ÏÂ: '[1;32mz[m', '[1;32m¡ý[m';\r\n");
	prints
	    ("Ë³Ê±Õë×ª¶¯: '[1;32mb[m', <[1;32mCR[m>; ÄæÊ±Õë×ª¶¯: '[1;32mh[m', '[1;32m¡ü[m'; 180¶È×ª¶¯: '[1;32mj[m';\r\n");
	prints("¿ì½µ: ' ', ÔÝÍ£: <[1;32mESC[m>.\r\n");
	prints("ÍË³ö: '[1;32m^C[m', '[1;32m^D[m'.\r\n\r\n");
	prints("Ã¿Ïû [1;33m30[m ÐÐÉýÒ»¼¶. \r\n");
	oflush();
	getchar();
	clear();
	oflush();
}

int
count()
{
	int c;
	FILE *fp;
	fp = fopen(TETRISPATH "/tetris.count", "r");
	if (fp == NULL) {
		system("echo 1 > " TETRISPATH "/tetris.count");
		return 1;
	}
	fscanf(fp, "%d", &c);
	fclose(fp);
	fp = fopen(TETRISPATH "/tetris.count", "w");
	fprintf(fp, "%d", c + 1);
	fclose(fp);
	return c + 1;
}

void
win_loadrec()
{
	FILE *fp;
	int n, T;
	char ID[20], FROM[20], PRIZ[3];
	for (n = 0; n <= 19; n++) {
		strcpy(topID[n], "null.");
		topT[n] = 0;
		strcpy(topFROM[n], "unknown.");
		strcpy(prize[n], "NA");
	}
	fp = fopen(TETRISPATH "/tetris.rec", "r");
	if (fp == NULL) {
		win_saverec();
		return;
	}
	for (n = 0; n <= 19; n++) {
		if (4 != fscanf(fp, "%s %d %s %s", ID, &T, FROM, PRIZ))
			break;
		strcpy(topID[n], ID);
		topT[n] = T;
		strcpy(topFROM[n], FROM);
		strcpy(prize[n], PRIZ);
	}
	fclose(fp);
}

void
win_saverec()
{
	FILE *fp;
	int n;
	fp = fopen(TETRISPATH "/tetris.rec", "w");
	flock(fileno(fp), LOCK_EX);
	for (n = 0; n <= 19; n++) {
		fprintf(fp, "%s %d %s %s\n", topID[n], topT[n], topFROM[n], prize[n]);
	}
	flock(fileno(fp), LOCK_UN);
	fclose(fp);
}

void
win_showrec()
{
	int n;
	win_loadrec();
	clear();
	prints
	    ("[44;37m                        --== " MY_BBS_NAME
	     " TETRIS ÅÅÐÐ°ñ ==--                         \r\n[m");
	prints
	    ("[41m Ãû´Î       Ãû×Ö        ÐÐÊý                      À´×Ô                     ³é½±[m\n\r");
	for (n = 0; n <= 19; n++) {
		sprintf(tmp, "[1;37m%3d[32m%13s[0;37m%12d[m%29s\033[33m%20s\n\r", n + 1,
			topID[n], topT[n], topFROM[n], prize[n]);
		prints(tmp);
	}
	prints
	    ("[41m                                                                               [m\n\r");
	oflush();
	presskey();
}

void
win_checkrec(int dt)
{
	char id[30];
	int n;
	win_loadrec();
	strcpy(id, userid);
	for (n = 0; n <= 19; n++)
		if (!strcmp(topID[n], id)) {
			if (dt > topT[n]) {
				topT[n] = dt;
				strcpy(topFROM[n], userip);
				strcpy(prize[n], "Î´");
				win_sort();
				win_saverec();
				win_showrec();
			}
			return;
		}
	if (dt > topT[19]) {
		strcpy(topID[19], id);
		topT[19] = dt;
		strcpy(topFROM[19], userip);
		strcpy(prize[19], "Î´");
		win_sort();
		win_saverec();
		win_showrec();
		return;
	}
}

void
win_sort()
{
	int n, n2, tmp;
	char tmpID[30];
	clear();
	prints("×£ºØ! ÄúË¢ÐÂÁË×Ô¼ºµÄ¼ÍÂ¼!\r\n");
	oflush();
	presskey();
	for (n = 0; n <= 18; n++)
		for (n2 = n + 1; n2 <= 19; n2++)
			if (topT[n] < topT[n2]) {
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

void
presskey()
{
	char c;
	if (read(0, &c, 1) <= 0)
		exit(-1);
}
