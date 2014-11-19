 /*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
    Guy Vega, gtvega@seabass.st.usm.edu
    Dominic Tynes, dbtynes@seabass.st.usm.edu
    Copyright (C) 1999, KCN,Zhou Lin, kcn@cic.tsinghua.edu.cn

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  */

#include "bbs.h"
#include "bbstelnet.h"
#include "edit.h"

#define OBUFSIZE  (1024*4)
#define IBUFSIZE  (256)

#define INPUT_ACTIVE 0
#define INPUT_IDLE 1

unsigned char outbuffer[OBUFSIZE + 1];
unsigned char *outbuf = outbuffer + 1;
int obufsize = 0;

unsigned char inbuffer[IBUFSIZE + 1];
unsigned char *inbuf = inbuffer + 1;
int ibufsize = 0;
int icurrchar = 0;
unsigned char inbuffer2[21];
int ibufsize2 = 0;
int icurrchar2 = 0;

int KEY_ESC_arg;

static int i_mode = INPUT_ACTIVE;
time_t now_t, last_utmp_update;

int canProcessSIGALRM = 0;
int occurredSIGALRM = 0;
int canProcessSIGUSR2 = 0;
int occurredSIGUSR2 = 0;
void (*handlerSIGALRM) (int) = NULL;

static void hit_alarm_clock(int sig);
static int telnet_machine(int ch);
static int filter_telnet(unsigned char *s, int *len);
static int igetch2(void);
static int igetch(void);
//static void top_show(char *prompt);

void
oflush()
{
	if (obufsize) {
		if (convcode) {
			char *out;
			out = gb2big(outbuf, &obufsize, 0);
#ifdef SSHBBS
			if (ssh_write(0, out, obufsize) <= 0)
#else
			if (write(0, out, obufsize) <= 0)
#endif
				abort_bbs();
#ifdef SSHBBS
		} else if (ssh_write(0, outbuf, obufsize) <= 0)
#else
		} else if (write(0, outbuf, obufsize) <= 0)
#endif
			abort_bbs();
	}
	obufsize = 0;
}

static void
hit_alarm_clock(int sig)
{
	if (!canProcessSIGALRM && sig) {
		occurredSIGALRM = 1;
		return;
	}
	if (currentuser && USERPERM(currentuser, PERM_NOTIMEOUT))
		return;
	if (i_mode == INPUT_IDLE) {
		clear();
		prints("Idle timeout exceeded! Booting...\n");
		oflush();
		kill(getpid(), SIGHUP);
	}
	i_mode = INPUT_IDLE;
	alarm(IDLE_TIMEOUT);
}

void
init_alarm()
{
	i_mode = INPUT_IDLE;
	signal(SIGALRM, (void *) hit_alarm_clock);
	handlerSIGALRM = hit_alarm_clock;
	alarm(IDLE_TIMEOUT);
}

void inline
ochar(c)
int c;
{
	if (obufsize > OBUFSIZE - 1) {	/* doin a oflush */
		oflush();
	}
	outbuf[obufsize++] = c;
	/* need to change IAC to IAC IAC */
	if (c == IAC) {
		if (obufsize > OBUFSIZE - 1) {	/* doin a oflush */
			oflush();
		}
		outbuf[obufsize++] = c;
	}
}

void inline
output(s, len)
char *s;
int len;
{
	int l, n;
	char *p0, *p1;
	p0 = s;
	l = len;
	while ((p1 = memchr(p0, IAC, l)) != NULL) {
		n = p1 - p0 + 1;
		if (obufsize + n + 1 > OBUFSIZE) {
			oflush();
		}
		memcpy(outbuf + obufsize, p0, n);
		obufsize += n;
		p0 += n;
		l -= n;
		outbuf[obufsize++] = IAC;
	}
	if (obufsize + l > OBUFSIZE) {
		oflush();
	}
	memcpy(outbuf + obufsize, p0, l);
	obufsize += l;
}

void inline
outputstr(s)
char *s;
{
	output(s, strlen(s));
}

int i_newfd = 0;
struct timeval i_to, *i_top = NULL;
void (*flushf) () = NULL;

void
add_io(fd, timeout)
int fd;
int timeout;
{
	i_newfd = fd;
	if (timeout) {
		i_to.tv_sec = timeout;
		i_to.tv_usec = 0;
		i_top = &i_to;
	} else
		i_top = NULL;
}

void
add_flush(flushfunc)
void (*flushfunc) ();
{
	flushf = flushfunc;
}

int
num_in_buf()
{
	return icurrchar - ibufsize;
}

char lastch;
int telnet_state = 0;
int naw_col, naw_ln, naw_changed = 0;
static int
telnet_machine(ch)
unsigned char ch;
{
	switch (telnet_state) {
	case 255:		/* after the first IAC */
		switch (ch) {
		case DO:
		case DONT:
		case WILL:
		case WONT:
			telnet_state = 1;
			break;
		case SB:	/* loop forever looking for the SE */
			telnet_state = 2;
			break;
		case IAC:
			return IAC;
		default:
			telnet_state = 0;	/* other telnet command */
		}
		break;
	case 1:		/* Get the DO,DONT,WILL,WONT */
		telnet_state = 0;	/* the ch is the telnet option */
		break;
	case 2:		/* the telnet suboption */
		if (ch == 31)	/* 改变行列 */
			telnet_state = 4;
		if (ch == IAC)
			telnet_state = 3;	/* wait for SE */
		break;
	case 3:		/* wait for se */
		if (naw_changed) {
			naw_changed = 0;
			do_naws(naw_ln, naw_col);
		}
		if (ch == SE) {
			telnet_state = 0;
			break;
		} else
			telnet_state = 2;
		break;
	case 4:
		if (ch != 0)
			telnet_state = 3;
		else
			telnet_state = 5;
		break;
	case 5:
		naw_col = ch;
		telnet_state = 6;
		break;
	case 6:
		if (ch != 0)
			telnet_state = 3;
		else {
			naw_changed = 1;
			telnet_state = 7;
		}
		break;
	case 7:
		naw_ln = ch;
		telnet_state = 2;
		break;
	}
	return 0;
}

static int
filter_telnet(unsigned char *s, int *len)
{
	unsigned char *p1, *p2, *pend;
	int newlen;
	newlen = 0;
	for (p1 = s, p2 = s, pend = s + (*len); p1 != pend; p1++) {
		if (telnet_state) {
			int ch = 0;
			ch = telnet_machine(*p1);
			if (ch == IAC) {	/* 两个IAC */
				*p2 = IAC;
				p2++;
				newlen++;
			}
		} else {
			if (*p1 == IAC)
				telnet_state = 255;
			else {
				*p2 = *p1;
				p2++;
				newlen++;
			}
		}
	}
	return (*len = newlen);
}

static int
igetch2()
{
	char c;

      igetagain2:
	while ((icurrchar2 < ibufsize2) && (inbuf[icurrchar2] == 0))
		icurrchar2++;

	if (ibufsize2 <= icurrchar2) {
		fd_set readfds;
		struct timeval to;
		int sr, hifd;
		if (i_top)
			to = *i_top;
		else {
			to.tv_sec = IDLE_TIMEOUT * 60 * 3;
			to.tv_usec = 0;;
		}
		hifd = 1;
		FD_ZERO(&readfds);

		while (1) {
			switchSIGALRM(1);
			switchSIGUSR2(1);
			FD_SET(0, &readfds);
#ifdef SSHBBS
			sr = ssh_select(hifd, &readfds, NULL, NULL, &to);
#else
			sr = select(hifd, &readfds, NULL, NULL, &to);
#endif
			switchSIGALRM(0);
			switchSIGUSR2(0);
			if (sr >= 0)
				break;
			if (errno == EINTR)
				continue;
			else
				abort_bbs();
		}
		if ((sr == 0) && (!i_top))
			abort_bbs();
		if (sr == 0)
			return I_TIMEOUT;
		now_t = time(0);
		uinfo.lasttime = now_t;
		if ((unsigned long) now_t - (unsigned long) last_utmp_update > 60) {
			update_utmp();
		}
#ifdef SSHBBS
		while ((ibufsize2 = ssh_read(0, inbuffer2 + 1, 20)) <= 0) {
#else
		while ((ibufsize2 = read(0, inbuffer2 + 1, 20)) <= 0) {
#endif
			if (ibufsize2 == 0)
				longjmp(byebye, -1);
			if (ibufsize2 < 0 && errno != EINTR)
				longjmp(byebye, -1);
		}
		if (!filter_telnet(inbuffer2 + 1, &ibufsize2)) {
			icurrchar2 = 0;
			ibufsize2 = 0;
			goto igetagain2;
		}
		/* add by KCN for GB/BIG5 encode */
		if (convcode) {
			inbuf = big2gb(inbuffer2 + 1, &ibufsize2, 0);
			if (ibufsize2 == 0) {
				icurrchar2 = 0;
				goto igetagain2;
			}
		} else
			inbuf = inbuffer2 + 1;
		/* end */
		icurrchar2 = 0;
	}

	if (icurrchar2 >= ibufsize2)
		goto igetagain2;

	if (((inbuf[icurrchar2] == '\n') && (lastch == '\r'))
	    || ((inbuf[icurrchar2] == '\r') && (lastch == '\n'))) {
		lastch = 0;
		icurrchar2++;
		goto igetagain2;
	}
	lastch = inbuf[icurrchar2];

	i_mode = INPUT_ACTIVE;
	c = inbuf[icurrchar2];
	switch (c) {
	case Ctrl('L'):
		redoscr();
		icurrchar2++;
		goto igetagain2;
	default:
		break;
	}
	icurrchar2++;
	while ((icurrchar2 != ibufsize2) && (inbuf[icurrchar2] == 0))
		icurrchar2++;
	return c;
}

static int
igetch()
{
	char c;
	extern int RMSG;

	if (RMSG == YEA && ((uinfo.mode == CHAT1) || (uinfo.mode == CHAT2)
			    || (uinfo.mode == CHAT3)
			    || (uinfo.mode == TALK) || (uinfo.mode == PAGE)))
		return igetch2();

      igetagain:
	while ((icurrchar < ibufsize) && (inbuf[icurrchar] == 0))
		icurrchar++;
	if (ibufsize <= icurrchar) {
		fd_set readfds;
		struct timeval to;
		int sr, hifd = 1;

		if (flushf && RMSG == NA)
			(*flushf) ();
		refresh();
		hifd = 1;
		if (i_newfd && RMSG == NA) {
			hifd = i_newfd + 1;
		}
		if (i_top && RMSG == NA)
			to = *i_top;
		else {
			to.tv_sec = IDLE_TIMEOUT * 60 * 3;
			to.tv_usec = 0;
		}
		FD_ZERO(&readfds);

		while (1) {
			switchSIGALRM(1);
			switchSIGUSR2(1);
			FD_SET(0, &readfds);
			if (i_newfd) {
				FD_SET(i_newfd, &readfds);
			}
#ifdef SSHBBS
			sr = ssh_select(hifd, &readfds, NULL, NULL, &to);
#else
			sr = select(hifd, &readfds, NULL, NULL, &to);
#endif
			switchSIGALRM(0);
			switchSIGUSR2(0);
			if (sr >= 0)
				break;
			if (errno == EINTR)
				continue;
			else
				abort_bbs();
		}
		if ((sr == 0) && (!i_top))
			abort_bbs();
		if (sr == 0)
			return I_TIMEOUT;
		if (i_newfd && FD_ISSET(i_newfd, &readfds))
			return I_OTHERDATA;
		now_t = time(0);
		uinfo.lasttime = now_t;
		if ((unsigned long) now_t - (unsigned long) last_utmp_update > 60) {
			update_utmp();
		}
#ifdef SSHBBS
		while ((ibufsize = ssh_read(0, inbuffer + 1, IBUFSIZE)) <= 0) {
#else
		while ((ibufsize = read(0, inbuffer + 1, IBUFSIZE)) <= 0) {
#endif
			if (ibufsize == 0)
				longjmp(byebye, -1);
			if (ibufsize < 0 && errno != EINTR)
				longjmp(byebye, -1);
		}
		if (!filter_telnet(inbuffer + 1, &ibufsize)) {
			icurrchar = 0;
			ibufsize = 0;
			goto igetagain;
		}
		/* add by KCN for GB/BIG5 encode */
		if (!convcode) {
			inbuf = inbuffer + 1;
		} else {
			inbuf = big2gb(inbuffer + 1, &ibufsize, 0);
			if (ibufsize == 0) {
				icurrchar = 0;
				goto igetagain;
			}
		}
		/* end */
		icurrchar = 0;
	}

	if (icurrchar >= ibufsize)
		goto igetagain;

	if (((inbuf[icurrchar] == '\n') && (lastch == '\r'))
	    || ((inbuf[icurrchar] == '\r') && (lastch == '\n'))) {
		lastch = 0;
		icurrchar++;
		goto igetagain;
	}

	lastch = inbuf[icurrchar];

	i_mode = INPUT_ACTIVE;
	c = inbuf[icurrchar];
	switch (c) {
	case Ctrl('L'):
		redoscr();
		icurrchar++;
		goto igetagain;
	default:
		break;
	}
	icurrchar++;
	while ((icurrchar != ibufsize) && (inbuf[icurrchar] == 0))
		icurrchar++;
	return c;
}

int
igetkey()
{
	int mode;
	int ch, last;
	int oldblock;
	extern int msg_blocked;
	sigset_t set, oldset;
	mode = last = 0;
	while (1) {
		ch = igetch();
		if ((ch == Ctrl('Z')) && (RMSG == NA)
		    && (uinfo.mode != LOCKSCREEN)) {
			if (have_msg_unread) {
				oldblock = msg_blocked;
				msg_blocked = 0;
				sigemptyset(&set);
				sigaddset(&set, SIGUSR2);
				sigprocmask(SIG_BLOCK, &set, &oldset);
				r_msg(0);
				sigprocmask(SIG_SETMASK, &oldset, NULL);
				msg_blocked = oldblock;
			} else
				r_msg2();
			return 0;
		}
		if (mode == 0) {
			if (ch == KEY_ESC)
				mode = 1;
			else
				return ch;	/* Normal Key */
		} else if (mode == 1) {	/* Escape sequence */
			if (ch == '[' || ch == 'O')
				mode = 2;
			else if (ch == '1' || ch == '4')
				mode = 3;
			else {
				KEY_ESC_arg = ch;
				return KEY_ESC;
			}
		} else if (mode == 2) {	/* Cursor key */
			if (ch >= 'A' && ch <= 'D')
				return KEY_UP + (ch - 'A');
			else if (ch >= '1' && ch <= '6')
				mode = 3;
			else
				return ch;
		} else if (mode == 3) {	/* Ins Del Home End PgUp PgDn */
			if (ch == '~')
				return KEY_HOME + (last - '1');
			else
				return ch;
		}
		last = ch;
	}
}

char
askone(line, col, prompt, menu, dft)
int line, col;
char *prompt, *menu;
char dft;
{
	char ans, ch;

	move(line, col);
	if (prompt) {
		prints("%s", prompt);
	}
	getyx(&line, &col);
      AGAIN:ans = '\0';
	while ((ch = igetkey()) != '\r') {
		if (ch == '\n')
			break;
		if (!isprint2(ch)) {
			ans = ' ';
			outc(' ');
			move(line, col);
			continue;
		}
		outc(ch);
		move(line, col);
		ans = ch;
	}
	ans = toupper(ans);
	if (!ans || !strchr(menu, ans)) {
		if (!dft)
			goto AGAIN;
		return dft;
	}
	return ans;
}

int
getdata(line, col, prompt, buf, len, echo, clearlabel)
int line, col, len, echo, clearlabel;
char *prompt, *buf;
{
	int ch, clen = 0, curr = 0, x, y;
	char tmp[STRLEN];
	int dbchar, i;
	extern int scr_cols;
	extern int RMSG;
	extern int have_msg_unread;
	extern int k_getdata_val;

	if (clearlabel == YEA) {
		buf[0] = 0;
	}
	move(line, col);
	if (prompt) {
		prints("%s", prompt);
	}
	getyx(&y, &x);
	clen = strlen(buf);
	curr = (clen >= len) ? len - 1 : clen;
	buf[curr] = '\0';
	prints("%s", buf);
	if (echo == NA) {
		while ((ch = igetkey()) != '\r') {
			if (RMSG == YEA && have_msg_unread == 0) {
				if (ch == Ctrl('Z') || ch == KEY_UP) {
					buf[0] = Ctrl('Z');
					clen = 1;
					break;
				}
				if (ch == Ctrl('A') || ch == KEY_DOWN) {
					buf[0] = Ctrl('A');
					clen = 1;
					break;
				}
			}
			if (ch == '\n')
				break;
			if (ch == '\177' || ch == Ctrl('H')) {
				if (clen == 0) {
					continue;
				}
				clen--;
				getyx(&y, &x);
				move(y, x - 1);
				outc(' ');
				move(y, x - 1);
				continue;
			}
			if (!isprint2(ch)) {
				continue;
			}
			if (clen >= len - 1) {
				continue;
			}
			buf[clen++] = ch;
			if (echo)
				outc(ch);
			else
				outc('*');
		}
		buf[clen] = '\0';
		outc('\n');
		return clen;
	}
	clrtoeol();
	while (1) {
		ch = igetkey();
		if ((RMSG == YEA) && have_msg_unread == 0) {
			if (ch == Ctrl('Z') || ch == KEY_UP) {
				buf[0] = Ctrl('Z');
				clen = 1;
				break;
			}
			if (ch == Ctrl('A') || ch == KEY_DOWN) {
				buf[0] = Ctrl('A');
				clen = 1;
				break;
			}
		}
		if (uinfo.mode == KILLER && RMSG != YEA
		    && (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_PGUP
			|| ch == KEY_PGDN || ch == Ctrl('S')
			|| ch == Ctrl('T'))) {
			k_getdata_val = ch;
			break;
		}
		if (ch == '\n' || ch == '\r')
			break;
		if (ch == Ctrl('R')) {
			enabledbchar = ~enabledbchar & 1;
			continue;
		}
		if (ch == '\177' || ch == Ctrl('H')) {
			if (curr == 0) {
				continue;
			}
			strcpy(tmp, &buf[curr]);
			if (enabledbchar) {
				dbchar = 0;
				for (i = 0; i < curr - 1; i++)
					if (dbchar)
						dbchar = 0;
					else if (buf[i] & 0x80)
						dbchar = 1;
				if (dbchar) {
					curr--;
					clen--;
				}
			}
			buf[--curr] = '\0';
			(void) strcat(buf, tmp);
			clen--;
			move(y, x);
			prints("%s", buf);
			clrtoeol();
			move(y, x + curr);
			continue;
		}
		if (ch == KEY_DEL) {
			int pos = curr + 1;
			if (curr >= clen) {
				curr = clen;
				continue;
			}
			if (enabledbchar) {
				dbchar = 0;
				for (i = clen - 1; i >= curr + 1; i--)
					if (dbchar)
						dbchar = 0;
					else if (buf[i] & 0x80)
						dbchar = 1;
				if (dbchar) {
					pos = curr + 2;
					clen--;
				}
			}
			strcpy(tmp, &buf[pos]);
			buf[curr] = '\0';
			(void) strcat(buf, tmp);
			clen--;
			move(y, x);
			prints("%s", buf);
			clrtoeol();
			move(y, x + curr);
			continue;
		}
		if (ch == KEY_LEFT) {
			if (curr == 0) {
				continue;
			}
			curr--;
			if (enabledbchar) {
				int i, j = 0;
				for (i = 0; i < curr; i++)
					if (j)
						j = 0;
					else if (buf[i] < 0)
						j = 1;
				if (j) {
					curr--;
				}
			}
			move(y, x + curr);
			continue;
		}
		if (ch == Ctrl('E') || ch == KEY_END) {
			curr = clen;
			move(y, x + curr);
			continue;
		}
		if (ch == Ctrl('A') || ch == KEY_HOME) {
			curr = 0;
			move(y, x + curr);
			continue;
		}
		if (ch == KEY_RIGHT) {
			if (curr >= clen) {
				curr = clen;
				continue;
			}
			curr++;
			if (enabledbchar) {
				int i, j = 0;
				for (i = 0; i < curr; i++)
					if (j)
						j = 0;
					else if (buf[i] < 0)
						j = 1;
				if (j) {
					curr++;
				}
			}

			move(y, x + curr);
			continue;
		}
		if (!isprint2(ch)) {
			continue;
		}

		if (x + clen >= scr_cols || clen >= len - 1) {
			continue;
		}

		if (!buf[curr]) {
			buf[curr + 1] = '\0';
			buf[curr] = ch;
		} else {
			strncpy(tmp, &buf[curr], len);
			buf[curr] = ch;
			buf[curr + 1] = '\0';
			strncat(buf, tmp, len - curr);
		}
		curr++;
		clen++;
		move(y, x);
		prints("%s", buf);
		move(y, x + curr);
	}
	buf[clen] = '\0';
	if (echo) {
		move(y, x);
		prints("%s", buf);
	}
	outc('\n');
	return clen;
}

int ZmodemRateLimit = 0;
int
raw_write(int fd, char *buf, int len)
{
	static int lastcounter = 0;
	int nowcounter;
#ifndef SSHBBS
	int i;
	int retlen = 0;
	int mylen;
	char *wb;
#endif
	static int bufcounter;
	if (ZmodemRateLimit) {
		nowcounter = time(0);
		if (lastcounter == nowcounter) {
			if (bufcounter >= ZMODEM_RATE) {
				sleep(1);
				nowcounter = time(0);
				bufcounter = len;
			} else
				bufcounter += len;
		} else {
			/*
			 * time clocked, clear bufcounter
			 */
			bufcounter = len;
		}
		lastcounter = nowcounter;
	}
#ifdef SSHBBS
	return ssh_write(fd, buf, len);
#else
	wb = malloc(len * 2);
	if (wb == NULL)
		return -1;
	for (i = 0; i < len; i++) {
		if ((unsigned char) buf[i] == 0xff) {
			wb[retlen++] = '\xff';
			wb[retlen++] = '\xff';
		} else if (buf[i] == 13) {
			wb[retlen++] = '\x1d';
			wb[retlen++] = '\x00';
		} else
			wb[retlen] = buf[i];
	}
	mylen = write(fd, wb, retlen);
	free(wb);
	return mylen;

#endif
}

int
raw_read(int fd, char *buf, int len)
{
#ifdef SSHBBS
	return ssh_read(fd, buf, len);
#else
	return read(fd, buf, len);
#endif
}

int
multi_getdata(int line, int col, int maxcol, char *prompt, char *buf, int len,
	      int maxline, int clearlabel)
{
	int ch, x, y, startx, starty, now, i, j, k, i0, chk, cursorx =
	    0, cursory = 0;
	char savebuffer[25][LINELEN * 3];
	extern int RMSG;

	if (clearlabel == 1) {
		buf[0] = 0;
	}
	move(line, col);
	if (prompt)
		prints("%s", prompt);
	getyx(&starty, &startx);
	now = strlen(buf);
	for (i = 0; i < 24; i++)
		saveline(i, 0, savebuffer[i]);

	while (1) {
		y = starty;
		x = startx;
		move(y, x);
		chk = 0;
		if (now == 0) {
			cursory = y;
			cursorx = x;
		}
		for (i = 0; i < strlen(buf); i++) {
			if (chk)
				chk = 0;
			else if (buf[i] < 0)
				chk = 1;
			if (chk && x >= maxcol)
				x++;
			if (buf[i] != 13 && buf[i] != 10) {
				if (x > maxcol) {
					clrtoeol();
					x = col;
					y++;
					move(y, x);
				}
				prints("%c", buf[i]);
				x++;
			} else {
				clrtoeol();
				x = col;
				y++;
				move(y, x);
			}
			if (i == now - 1) {
				cursory = y;
				cursorx = x;
			}
		}
		clrtoeol();
		move(cursory, cursorx);
		ch = igetkey();
		if ((ch == '\n' || ch == '\r'))	// && num_in_buf()==0)
			break;
		for (i = starty; i <= y; i++)
			saveline(i, 1, savebuffer[i]);
		if (1 == RMSG
		    && (KEY_UP == ch || KEY_DOWN == ch || Ctrl('Z') == ch
			|| Ctrl('A') == ch)
		    && (!buf[0])) {
			return -ch;
		}
		switch (ch) {
		case Ctrl('Q'):
//              case '\n':
//              case '\r':
			if (y - starty + 1 < maxline) {
				for (i = strlen(buf) + 1; i > now; i--)
					buf[i] = buf[i - 1];
				buf[now++] = '\n';
			}
			break;
		case KEY_UP:
			if (cursory > starty) {
				y = starty;
				x = startx;
				chk = 0;
				if (y == cursory - 1 && x <= cursorx)
					now = 0;
				for (i = 0; i < strlen(buf); i++) {
					if (chk)
						chk = 0;
					else if (buf[i] < 0)
						chk = 1;
					if (chk && x >= maxcol)
						x++;
					if (buf[i] != 13 && buf[i] != 10) {
						if (x > maxcol) {
							x = col;
							y++;
						}
						x++;
					} else {
						x = col;
						y++;
					}
					if (!enabledbchar || !chk)
						if (y == cursory - 1
						    && x <= cursorx)
							now = i + 1;
				}
			}
			break;
		case KEY_DOWN:
			if (cursory < y) {
				y = starty;
				x = startx;
				chk = 0;
				if (y == cursory + 1 && x <= cursorx)
					now = 0;
				for (i = 0; i < strlen(buf); i++) {
					if (chk)
						chk = 0;
					else if (buf[i] < 0)
						chk = 1;
					if (chk && x >= maxcol)
						x++;
					if (buf[i] != 13 && buf[i] != 10) {
						if (x > maxcol) {
							x = col;
							y++;
						}
						x++;
					} else {
						x = col;
						y++;
					}
					if (!enabledbchar || !chk)
						if (y == cursory + 1
						    && x <= cursorx)
							now = i + 1;
				}
			}
			break;
		case '\177':
		case Ctrl('H'):
			if (now > 0) {
				for (i = now - 1; i < strlen(buf); i++)
					buf[i] = buf[i + 1];
				now--;
				if (enabledbchar) {
					chk = 0;
					for (i = 0; i < now; i++) {
						if (chk)
							chk = 0;
						else if (buf[i] < 0)
							chk = 1;
					}
					if (chk) {
						for (i = now - 1;
						     i < strlen(buf); i++)
							buf[i] = buf[i + 1];
						now--;
					}
				}
			}
			break;
		case KEY_DEL:
			if (now < strlen(buf)) {
				if (enabledbchar) {
					chk = 0;
					for (i = 0; i < now + 1; i++) {
						if (chk)
							chk = 0;
						else if (buf[i] < 0)
							chk = 1;
					}
					if (chk)
						for (i = now; i < strlen(buf);
						     i++)
							buf[i] = buf[i + 1];
				}
				for (i = now; i < strlen(buf); i++)
					buf[i] = buf[i + 1];
			}
			break;
		case KEY_LEFT:
			if (now > 0) {
				now--;
				if (enabledbchar) {
					chk = 0;
					for (i = 0; i < now; i++) {
						if (chk)
							chk = 0;
						else if (buf[i] < 0)
							chk = 1;
					}
					if (chk)
						now--;
				}
			}
			break;
		case KEY_RIGHT:
			if (now < strlen(buf)) {
				now++;
				if (enabledbchar) {
					chk = 0;
					for (i = 0; i < now; i++) {
						if (chk)
							chk = 0;
						else if (buf[i] < 0)
							chk = 1;
					}
					if (chk)
						now++;
				}
			}
			break;
		case KEY_HOME:
		case Ctrl('A'):
			now--;
			while (now >= 0 && buf[now] != '\n' && buf[now] != '\r')
				now--;
			now++;
			break;
		case KEY_END:
		case Ctrl('E'):
			while (now < strlen(buf) && buf[now] != '\n'
			       && buf[now] != '\r')
				now++;
			break;
		case KEY_PGUP:
			now = 0;
			break;
		case KEY_PGDN:
			now = strlen(buf);
			break;
		case Ctrl('Y'):
			i0 = strlen(buf);
			i = now - 1;
			while (i >= 0 && buf[i] != '\n' && buf[i] != '\r')
				i--;
			i++;
			if (!buf[i])
				break;
			j = now;
			while (j < i0 - 1 && buf[j] != '\n' && buf[j] != '\r')
				j++;
			if (j >= i0 - 1)
				j = i0 - 1;
			j = j - i + 1;
			if (j < 0)
				j = 0;
			for (k = 0; k < i0 - i - j + 1; k++)
				buf[i + k] = buf[i + j + k];

			y = starty;
			x = startx;
			chk = 0;
			if (y == cursory && x <= cursorx)
				now = 0;
			for (i = 0; i < strlen(buf); i++) {
				if (chk)
					chk = 0;
				else if (buf[i] < 0)
					chk = 1;
				if (chk && x >= maxcol)
					x++;
				if (buf[i] != 13 && buf[i] != 10) {
					if (x > maxcol) {
						x = col;
						y++;
					}
					x++;
				} else {
					x = col;
					y++;
				}
				if (!enabledbchar || !chk)
					if (y == cursory && x <= cursorx)
						now = i + 1;
			}

			if (now > strlen(buf))
				now = strlen(buf);
			break;
		default:
			if (isprint2(ch) && strlen(buf) < len - 1) {
				for (i = strlen(buf) + 1; i > now; i--)
					buf[i] = buf[i - 1];
				buf[now++] = ch;
				y = starty;
				x = startx;
				chk = 0;
				for (i = 0; i < strlen(buf); i++) {
					if (chk)
						chk = 0;
					else if (buf[i] < 0)
						chk = 1;
					if (chk && x >= maxcol)
						x++;
					if (buf[i] != 13 && buf[i] != 10) {
						if (x > maxcol) {
							x = col;
							y++;
						}
						x++;
					} else {
						x = col;
						y++;
					}
				}
				if (y - starty + 1 > maxline) {
					for (i = now - 1; i < strlen(buf); i++)
						buf[i] = buf[i + 1];
					now--;
				}
			}
			break;
		}
	}

	return y - starty + 1;
}

void
switchSIGALRM(int on)
{
	if (!on) {
		canProcessSIGALRM = 0;
		return;
	}
	canProcessSIGALRM = 1;
	if (occurredSIGALRM) {
		sigset_t set, oldset;
		struct sigaction oldact;
		sigemptyset(&set);
		sigaddset(&set, SIGALRM);
		sigprocmask(SIG_BLOCK, &set, &oldset);
		sigaction(SIGALRM, NULL, &oldact);
		if (handlerSIGALRM && handlerSIGALRM == oldact.sa_handler)
			handlerSIGALRM(0);
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		occurredSIGALRM = 0;
	}
}

void
switchSIGUSR2(int on)
{
	if (!on) {
		canProcessSIGUSR2 = 0;
		return;
	}
	canProcessSIGUSR2 = 1;
	if (occurredSIGUSR2) {
		sigset_t set, oldset;
		sigemptyset(&set);
		sigaddset(&set, SIGUSR2);
		sigprocmask(SIG_BLOCK, &set, &oldset);
		r_msg(0);
		sigprocmask(SIG_SETMASK, &oldset, NULL);
		occurredSIGUSR2 = 0;
	}
}

void
show_message(msg)
char msg[];
{

	move(t_lines - 1, 0);
	clrtoeol();
	if (msg != NULL)
		prints("\033[1m%s\033[m", msg);
}

void
bell()
{
	char sound;
	if (USERDEFINE(currentuser, DEF_SOUNDMSG)) {
		sound = Ctrl('G');
		output(&sound, 1);
	}
}

int
pressanykey()
{
	extern int showansi;

	showansi = 1;
	move(t_lines - 1, 0);
	clrtoeol();
	prints
	    ("\033[m                                \033[5;1;33m按任何键继续...\033[m");
	egetch();
	move(t_lines - 1, 0);
	clrtoeol();
	return 0;
}

int
pressreturn()
{
	extern int showansi;
	char buf[3];

	showansi = 1;
	move(t_lines - 1, 0);
	clrtoeol();
	getdata(t_lines - 1, 0,
		"                              \033[1;33m请按 ◆\033[5;36mEnter\033[m\033[1;33m◆ 继续\033[m",
		buf, 2, NOECHO, YEA);
	move(t_lines - 1, 0);
	clrtoeol();
	return 0;
}

int
askyn(str, defa, gobottom)
char str[STRLEN];
int defa, gobottom;
{
	int x, y;
	char realstr[280];
	char ans;

	snprintf(realstr, sizeof (realstr), "%s (Y/N)? [%c]: ", str,
		 (defa) ? 'Y' : 'N');
	if (gobottom)
		move(t_lines - 1, 0);
	getyx(&x, &y);
	clrtoeol();
	ans = askone(x, y, realstr, "YN", (defa) ? 'Y' : 'N');
	outc('\n');
	if (ans == 'Y')
		return 1;
	else
		return 0;
}

void
printdash(mesg)
char *mesg;
{
	char buf[80], *ptr;
	int len;

	memset(buf, '=', 79);
	buf[79] = '\0';
	if (mesg != NULL) {
		len = strlen(mesg);
		if (len > 76)
			len = 76;
		ptr = &buf[40 - len / 2];
		ptr[-1] = ' ';
		ptr[len] = ' ';
		strncpy(ptr, mesg, len);
	}
	prints("%s\n", buf);
}
