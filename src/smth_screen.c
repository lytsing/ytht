/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

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
#include "edit.h"
#include "smth_screen.h"
#include <sys/param.h>
#include <stdarg.h>

extern void output(const char *s, int len);
extern void ochar(int c);

#define o_clear() {if(tc_color!=7||tc_mode!=0) output("\033[m",3);output("\033[H\033[J",6); tc_mode=0; tc_color=7;  tc_col=0; tc_line=0; }
#define o_cleol() output("\033[K",3)

int scr_lns = 24, scr_cols = 80;
int cur_ln = 0, cur_col = 0;
int roll, scrollcnt;
int tc_col = 0, tc_line = 0;
int tc_mode = 0, tc_color = 7;
int cur_mode = 0, cur_color = 7;
int default_cur_mode = 0, default_cur_color = 7;
int offsetln = 0;
int disable_move = 0;
struct screenline *big_picture = NULL;
static const char nullstr[] = "(null)";
static /*struct screenline old_line; */ char tmpbuffer[LINELEN * 3];
static char printable[256];
static void init_screen(int slns, int scols);
static void rel_move(int was_col, int was_ln, int new_col, int new_ln);

void
setdefaultfcolor(int i, int j)
{
	default_cur_color = i + default_cur_color / 16 * 16;
	if (j)
		default_cur_mode |= SCREEN_BRIGHT;
	else
		default_cur_mode &= ~SCREEN_BRIGHT;
	cur_color = default_cur_color;
	cur_mode = default_cur_mode;
}

void
setdefaultbcolor(int i)
{
	if (i == 0)
		i = 8;
	default_cur_color = default_cur_color % 16 + i * 16;
	cur_color = default_cur_color;
}

void
setfcolor(int i, int j)
{
	cur_color = i + cur_color / 16 * 16;
	if (j)
		cur_mode |= SCREEN_BRIGHT;
	else
		cur_mode &= ~SCREEN_BRIGHT;
}

void
setbcolor(int i)
{
	if (i == 0)
		i = 8;
	cur_color = cur_color % 16 + i * 16;
}

void
resetcolor()
{
	default_cur_color = 7;
	default_cur_mode = 0;
	prints("\033[37;40;0m");
}

int
num_noans_chr(char *str)
{
	int len, i, ansinum, ansi;

	ansinum = 0;
	ansi = 0;
	len = strlen(str);
	for (i = 0; i < len; i++) {
		if (str[i] == KEY_ESC) {
			ansi = 1;
			ansinum++;
			continue;
		}
		if (ansi) {
			if (isalpha(str[i]))
				ansi = 0;
			ansinum++;
			continue;
		}
	}
	return len - ansinum;
}

static void
init_screen(int slns, int scols)
{
	struct screenline *slp, *oslp;
	struct screenline *oldp = big_picture;
	int i;
	for (i = 0; i < 256; i++) {
		if (isgraph(i) || ((char) i) & 0x80 || i == ' ')
			printable[i] = 1;
		else
			printable[i] = 0;
	}
	scr_lns = slns;
	scr_cols = (scols < LINELEN) ? scols : LINELEN;

	big_picture =
	    (struct screenline *) calloc(scr_lns, sizeof (struct screenline));
	for (slns = 0; slns < scr_lns; slns++) {
		slp = &big_picture[slns];
		slp->lchanged = 1;
		memset(slp->data, 32, LINELEN);
		memset(slp->mode, 0, LINELEN);
		memset(slp->color, 7, LINELEN);
		memset(slp->changed, 1, LINELEN);
	}
	roll = 0;
	if (!oldp)
		return;
	//��ֹ��ĻͻȻ��� ���ٻָ�һЩԭ����ʾ�Ķ���
	for (slns = 0; slns < 24; slns++) {
		slp = &big_picture[slns];
		oslp = &oldp[slns];
		memcpy(slp->data, oslp->data, 80);
		memcpy(slp->mode, oslp->mode, 80);
		memcpy(slp->color, oslp->color, 80);
	}
	free(oldp);
	oldp = NULL;
}

void
clear()
{
	int i;
	struct screenline *slp;

	for (i = 0; i < scr_lns; i++) {
		slp = &big_picture[i];
		memset(slp->data, 32, scr_cols);
		memset(slp->mode, 0, scr_cols);
		memset(slp->color, 7, scr_cols);
		memset(slp->changed, 0, scr_cols);
		slp->lchanged = 0;
	}
	cur_color = 7;
	cur_mode = 0;
	o_clear();
	move(0, 0);
}

void
initscr()
{
	//t_columns = WRAPMARGIN;
	init_screen(t_lines, t_columns);
}

static void
rel_move(int was_col, int was_ln, int new_col, int new_ln)
{
	int i;
	struct screenline *bp = big_picture;
	if (new_col == t_columns)
		new_col--;
	if (new_ln >= t_lines || new_col >= t_columns)
		return;
	if (was_col == new_col && was_ln == new_ln)
		return;
	tc_col = new_col;
	tc_line = new_ln;
	if ((new_col == 0) && (new_ln == was_ln + 1)) {
		if (tc_color % 16 != 7)
			tc_color = tc_color / 16 * 16 + 8;
		if (was_col != 0)
			ochar('\r');
		ochar('\n');
		return;
	}
	if ((new_col == 0) && (new_ln == was_ln)) {
		ochar('\r');
		return;
	}
	if (new_col <= was_col - 1 && new_col >= was_col - 5
	    && new_ln == was_ln) {
		for (i = 0; i < was_col - new_col; i++)
			ochar(Ctrl('H'));
		return;
	}
	if (new_ln == was_ln && new_col >= was_col + 1
	    && new_col <= was_col + 5) {
		int p = 1, q = (new_ln + roll) % scr_lns;
		for (i = was_col; i < new_col; i++)
			p = p && (bp[q].color[i] == tc_color)
			    && (bp[q].mode[i] == tc_mode);
		if (p) {
			for (i = was_col; i < new_col; i++)
				if (bp[q].data[i] == 0)
					ochar(32);
				else
					ochar(bp[q].data[i]);
			return;
		}
	}
	if (new_ln == was_ln && new_col >= was_col + 1) {
		char ss[20];
		if (new_col == was_col + 1)
			sprintf(ss, "\033[C");
		else
			sprintf(ss, "\033[%dC", new_col - was_col);
		output(ss, strlen(ss));
		return;
	}
	if (new_ln == was_ln && new_col <= was_col - 1) {
		char ss[20];
		if (new_col == was_col - 1)
			sprintf(ss, "\033[D");
		else
			sprintf(ss, "\033[%dD", was_col - new_col);
		output(ss, strlen(ss));
		return;
	}
	if ((new_col == was_col || new_col == 0) && new_ln >= was_ln + 1) {
		char ss[20];
		if (tc_color % 16 != 7)
			tc_color = tc_color / 16 * 16 + 8;
		if (new_ln == was_ln + 1)
			sprintf(ss, "\033[B");
		else
			sprintf(ss, "\033[%dB", new_ln - was_ln);
		output(ss, strlen(ss));
		if (new_col == 0 && was_col != 0)
			ochar('\r');
		return;
	}
	if ((new_col == was_col || new_col == 0) && new_ln <= was_ln - 1) {
		char ss[20];
		if (tc_color % 16 != 7)
			tc_color = tc_color / 16 * 16 + 8;
		if (new_ln == was_ln - 1)
			sprintf(ss, "\033[A");
		else
			sprintf(ss, "\033[%dA", was_ln - new_ln);
		output(ss, strlen(ss));
		if (new_col == 0 && was_col != 0)
			ochar('\r');
		return;
	}
	if (new_ln == was_ln + 1 && new_col <= 5) {
		int p = 1, q = (new_ln + roll) % scr_lns;
		for (i = 0; i < new_col; i++)
			p = p && (bp[q].color[i] == tc_color)
			    && (bp[q].mode[i] == tc_mode);
		if (p) {
			ochar('\n');
			if (tc_color % 16 != 7)
				tc_color = tc_color / 16 * 16 + 8;
			if (was_col != 0)
				ochar('\r');
			for (i = 0; i < new_col; i++)
				if (bp[q].data[i] == 0)
					ochar(32);
				else
					ochar(bp[q].data[i]);
			return;
		}
	}

	do_move(new_col, new_ln, (void *) ochar);
}

static void
rel_changemodecolor(int mode, int color)
{
	int stackt = 0, i;
	int stack[100];
	if (((!(mode & SCREEN_BRIGHT) && tc_mode & SCREEN_BRIGHT)
	     || (!(mode & SCREEN_LINE) && tc_mode & SCREEN_LINE)
	     || (!(mode & SCREEN_BLINK) && tc_mode & SCREEN_BLINK)
	     || (!(mode & SCREEN_BACK) && tc_mode & SCREEN_BACK))
	    || ((tc_color & 0xf0) && !(color & 0xf0))) {
		tc_mode = 0;
		tc_color = 7;
		output("\033[m", 3);
	}
	if (!(tc_mode & SCREEN_BRIGHT)
	    && mode & SCREEN_BRIGHT) {
		tc_mode |= SCREEN_BRIGHT;
		stack[stackt++] = 1;
	}
	if (!(tc_mode & SCREEN_LINE) && mode & SCREEN_LINE) {
		tc_mode |= SCREEN_LINE;
		stack[stackt++] = 4;
	}
	if (!(tc_mode & SCREEN_BLINK) && mode & SCREEN_BLINK) {
		tc_mode |= SCREEN_BLINK;
		stack[stackt++] = 5;
	}
	if (!(tc_mode & SCREEN_BACK) && mode & SCREEN_BACK) {
		tc_mode |= SCREEN_BACK;
		stack[stackt++] = 7;
	}
	if ((tc_color & 0x0f) != (color & 0x0f)) {
		tc_color = (tc_color & 0xf0) + (color & 0x0f);
		if (!currentuser || USERDEFINE(currentuser, DEF_COLOR))
			stack[stackt++] = 30 + (color & 0x0f);
	}
	if ((tc_color & 0xf0) != (color & 0xf0)) {
		tc_color = (color & 0xf0) + (tc_color & 0x0f);
		if (!currentuser || USERDEFINE(currentuser, DEF_COLOR)) {
			if ((color & 0xf0) == 0x80)
				stack[stackt++] = 40;
			else
				stack[stackt++] = 40 + (color >> 4);
		}
	}
	if (stackt > 0) {
		char buf[200], *p;
		int pos = 0;
		strcpy(buf, "\033[");
		p = buf + 2;
		pos = 2;
		if (stackt != 1 || stack[0] != 0)
			for (i = 0; i < stackt; i++) {
				pos++;
				if (i == 0)
					sprintf(p, "%d", stack[i]);
				else {
					sprintf(p, ";%d", stack[i]);
					pos++;
				}
				if (stack[i] > 9)
					pos++;
				if (stack[i] > 99)
					pos++;
				p = buf + pos;
			}
		strcpy(p, "m");
		pos++;
		output(buf, pos);
	}
}

#define ndiff(i,j) (bp[i].changed[j]==0)

void
refresh()
{
	int i, j, k, ii, p, tailcolor, tailmode;
	struct screenline *bp = big_picture;
	int count = 0;

	if (num_in_buf() != 0)
		return;
	if (scrollcnt < 0) {
		char buf[10];
		rel_move(tc_col, tc_line, 0, 0);
		sprintf(buf, "\033[%dL", -scrollcnt);
		output(buf, strlen(buf));
		scrollcnt = 0;
	}
	if (scrollcnt > 0) {
		do_move(0, t_lines - 1, (void *) ochar);
		while (scrollcnt > 0) {
			ochar('\n');
			scrollcnt--;
		}
		tc_col = 0;
		tc_line = t_lines - 1;
	}

	for (i = 0; i < scr_lns; i++) {
		j = (i + roll) % scr_lns;
		if (!bp[j].lchanged)
			continue;
		bp[j].lchanged = 0;

		ii = scr_cols - 1;
		tailcolor = bp[j].color[scr_cols - 1] & 0xf0;
		tailmode = bp[j].mode[scr_cols - 1] & ~SCREEN_BRIGHT;

		while (ii >= 0 && bp[j].data[ii] == 32
		       && (bp[j].color[ii] & 0xf0) == tailcolor
		       && (bp[j].mode[ii] & ~SCREEN_BRIGHT) == tailmode)
			ii--;
		p = ii + 1;
		count = 0;
		for (ii = p; ii < scr_cols; ii++)
			if (!ndiff(j, ii))
				count++;

		rel_move(tc_col, tc_line, 0, i);
		for (k = 0; k < scr_cols; k++) {
			if ((ndiff(j, k)) && (k < p || count < 5))
				continue;
			if (tc_col != k)
				rel_move(tc_col, tc_line, k, i);
			if (bp[j].mode[k] != tc_mode
			    || bp[j].color[k] != tc_color)
				rel_changemodecolor(bp[j].mode[k],
						    bp[j].color[k]);
			if (k >= p && p <= scr_cols - 5) {
				if (k < scr_cols)
					memset(&(bp[j].changed[k]), 0,
					       scr_cols - k);
				o_cleol();
				break;
			}
			ochar(bp[j].data[k]);
			bp[j].changed[k] = 0;
			tc_col++;
		}
	}
	rel_move(tc_col, tc_line, cur_col, cur_ln);
	oflush();
}

void
redoscr()
{
	int i, j;
	struct screenline *bp = big_picture;

	for (i = 0; i < scr_lns; i++) {
		j = (i + roll) % scr_lns;
		bp[j].lchanged = 1;
		memset(bp[j].changed, 1, scr_cols);
	}
	do_move(tc_col, tc_line, (void *) ochar);
	refresh();
}

int
fakefunc()
{
	int i = 0;
	i = 1 / i;
	return 1;
}

void
move(int y, int x)
{
	if (x < 0 || y < 0)
		fakefunc();
	cur_col = x /*+c_shift(y,x) */ ;
	cur_ln = y;
}

void
getyx(int *y, int *x)
{
	*y = cur_ln;
	*x = cur_col /*-c_shift(y,x)*/ ;
}

void
clear_whole_line(int i)
{
	struct screenline *slp = &big_picture[(i + roll) % scr_lns];
	memset(slp->data, 32, scr_cols);
	memset(slp->mode, cur_mode, scr_cols);
	memset(slp->color, cur_color, scr_cols);
	memset(slp->changed, 1, scr_cols);
	slp->lchanged = 1;
}

void
clrtoeol()
{
	struct screenline *slp;
	int ln;

	ln = (cur_ln + roll) % scr_lns;
	slp = &big_picture[ln];
	if (t_columns > cur_col) {
		memset(slp->data + cur_col, 32, t_columns - cur_col);
		memset(slp->mode + cur_col, cur_mode, t_columns - cur_col);
		memset(slp->color + cur_col, cur_color, t_columns - cur_col);
		memset(slp->changed + cur_col, 1, t_columns - cur_col);
		slp->lchanged = 1;
	}
}

void
clrtobot()
{
	int i;
	clrtoeol();
	for (i = cur_ln + 1; i < scr_lns; i++)
		clear_whole_line(i);
}

void
outc(unsigned char c)
{
	struct screenline *slp;

	if (!isprint2(c)) {
		if (c == '\n') {	/* do the newline thing */
			clrtoeol();
			cur_col = 0;
			if (cur_ln < scr_lns - 1)
				cur_ln++;
		}
		return;
	}
	slp = &big_picture[(cur_ln + roll) % scr_lns];
	if (cur_col < scr_cols) {
		slp->mode[cur_col] = cur_mode;
		slp->color[cur_col] = cur_color;
		slp->data[cur_col] = c;
		slp->changed[cur_col] = 1;
		slp->lchanged = 1;
		cur_col++;
	}
}

int savey = -1, savex = -1;

void
outns(const char *str, int n)
{
	int i, j, k;
	char ch;
	struct screenline *slp;
	const char *endstr = str + n;
	int esc_quote, esc_quote_quote;

	while (str < endstr && *str) {
		slp = &big_picture[(cur_ln + roll) % scr_lns];
		if (printable[(unsigned char) *str]) {
			slp->lchanged = 1;
			do {
				if (cur_col < scr_cols) {
					slp->data[cur_col] = *str;
					slp->mode[cur_col] = cur_mode;
					slp->color[cur_col] = cur_color;
					slp->changed[cur_col] = 1;
					cur_col++;
				}
				str++;
			} while (str < endstr
				 && printable[(unsigned char) *str]);
			continue;
		}
		if (*str == '\r') {
			str++;
			continue;
		} else if (*str == '\033') {
			if (*(str + 1) != '[') {
				str++;
				continue;
			}
			if (*(str + 2) == '<') {
				i = 2;
				esc_quote = 0;
				esc_quote_quote = 0;
				while (str + i < endstr && str[i]
				       && str[i] != '\033' && (str[i] != '>' || esc_quote || esc_quote_quote)) {
					if (str[i] == '\'' && !esc_quote_quote)
						esc_quote = 1 - esc_quote;
					if (str[i] == '\"' && !esc_quote)
						esc_quote_quote = 1 - esc_quote_quote;
					i++;
				}
				if (str[i] == '>') {
					str += i + 1;
					continue;
				}
			}
			i = 1;
			while (!isalpha(*(str + i)) && (*(str + i) != '\033')
			       && *(str + i))
				i++;
			if (*(str + i) == 'H') {
				j = 0;
				while (j < i && *(str + j) != ';')
					j++;
				if (*(str + j) == ';' && j <= 4 && j >= 3
				    && i - j >= 2 && i - j <= 4) {
					char s1[5], s2[5], x, y;
					memcpy(s1, str + 2, j - 2);
					s1[j - 2] = 0;
					memcpy(s2, str + j + 1, i - j - 1);
					s2[i - j - 1] = 0;
					y = atoi(s1) - 1 + offsetln;
					x = atoi(s2) - 1;
					if (y >= 0 && y < scr_lns && x >= 0
					    && x < scr_cols) {
						cur_col = x;
						cur_ln = y;
					}
					str += i + 1;
					continue;
				} else if ((*str + j) != ';') {
					if (offsetln == 0)
						clear();
					str += i + 1;
					continue;
				}
			} else
			    if ((*(str + i) == 'A' || *(str + i) == 'B'
				 || *(str + i) == 'C' || *(str + i) == 'D')
				&& i <= 5) {
				char s1[5];
				s1[i - 2] = 0;
				memcpy(s1, str + 2, i - 2);
				if (s1[0])
					k = atoi(s1);
				else
					k = 1;
				if (!disable_move) {
					if (*(str + i) == 'A') {
						if (cur_ln >= k)
							cur_ln -= k;
						else
							cur_ln = 0;
					} else if (*(str + i) == 'B') {
						if (cur_ln < scr_lns - k)
							cur_ln += k;
						else
							cur_ln = scr_lns - 1;
					} else if (*(str + i) == 'C') {
						if (cur_col < scr_cols - k)
							cur_col += k;
						else
							cur_col = scr_cols;
					} else if (*(str + i) == 'D') {
						if (cur_col >= k)
							cur_col -= k;
						else
							cur_col = 0;
					}

					if (cur_col < 0)
						cur_col = 0;
					if (cur_col >= scr_cols)
						cur_col = scr_cols;
					if (cur_ln < offsetln)
						cur_ln = offsetln;
					if (cur_ln >= scr_lns)
						cur_ln = scr_lns - 1;
				}
				str += i + 1;
				continue;
			} else if (*(str + i) == 's' && i == 2) {
				str += 3;
				savey = cur_ln;
				savex = cur_col;
				continue;
			} else if (*(str + i) == 'u' && i == 2) {
				str += 3;
				if (savey != -1 && savex != -1 && !disable_move) {
					cur_ln = savey;
					cur_col = savex;
					continue;
				}
			} else if (*(str + i) == 'J') {
				str += i + 1;
				if (!disable_move)
					clear();
				continue;
			} else if (*(str + i) == 'm') {
				j = 1;
				while (*(str + j) != 'm') {
					int m;
					char s[100];
					j++;
					k = j;
					while (*(str + j) != 'm'
					       && *(str + j) != ';'
					       && *(str + j) >= '0'
					       && *(str + j) <= '9')
						j++;
					if (*(str + j) != 'm'
					    && *(str + j) != ';')
						break;
					memcpy(s, str + k, j - k);
					s[j - k] = 0;
					if (s[0]) {
						m = atoi(s);
						if (m == 0) {
							cur_mode =
							    default_cur_mode;
							cur_color =
							    default_cur_color;
						} else if (m == 1)
							cur_mode |=
							    SCREEN_BRIGHT;
						else if (m == 4)
							cur_mode |= SCREEN_LINE;
						else if (m == 5)
							cur_mode |=
							    SCREEN_BLINK;
						else if (m == 7)
							cur_mode |= SCREEN_BACK;
						else if (m >= 30 && m <= 37)
							cur_color =
							    m - 30 +
							    (cur_color & 0xf0);
						else if (m >= 40 && m <= 47) {
							if (m == 40)
								m = 48;
							cur_color =
							    ((m - 40) << 4) +
							    (cur_color & 0x0f);
						}
					}
				}
				if (i == 2) {
					cur_mode = default_cur_mode;
					cur_color = default_cur_color;
				}
				str += i + 1;
				continue;
			} else if (*(str + i) == 'M') {
				k = 1;
				for (j = 2; j < i; j++)
					k = k && (*(str + j) >= '0'
						  && *(str + j) <= '9');
				if (k) {
					refresh();
					output(str, i + 1);
					oflush();
				}
				str += i + 1;
				continue;
			} else if (isalpha(*(str + i))) {
				str += i + 1;
				continue;
			}
			ch = '*';
		} else if (*str == '\n') {
			if (cur_ln < scr_lns - 1)
				cur_ln++;
			cur_col = 0;
			str++;
			continue;
		} else if (*str == 9) {	//����tab
			j = (cur_col / 8 + 1) * 8 - cur_col;
			for (i = 0; i < j; i++) {
				if (cur_col < scr_cols) {
					slp->data[cur_col] = 32;
					slp->mode[cur_col] = cur_mode;
					slp->color[cur_col] = cur_color;
					slp->changed[cur_col] = 1;
					slp->lchanged = 1;
					cur_col++;
				}
			}
			str++;
			continue;
		} else if (isprint(*str)) {
			ch = *str;
		} else
			ch = '*';

		if (cur_col < scr_cols) {
			slp->data[cur_col] = ch;
			slp->mode[cur_col] = cur_mode;
			slp->color[cur_col] = cur_color;
			slp->changed[cur_col] = 1;
			slp->lchanged = 1;
			cur_col++;
		}
		str++;
	}
}

void
outs(const char *str)
{
	outns(str, 4096);
}

int dec[] =
    { 1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10,
	1
};

void
prints(char *format, ...)
{
	va_list ap;
	char *fmt;
	const char *bp;
	int i, count, hd, indx;
	char *begin;

	va_start(ap, format);
	begin = fmt = format;
	while (*fmt) {
		if (*fmt == '%') {
			int sgn = 1;
			int val = 0;
			int len, negi;

			if (fmt - begin)
				outns(begin, fmt - begin);
			fmt++;
			while (*fmt == '-') {
				sgn *= -1;
				fmt++;
			}
			while (isdigit(*fmt)) {
				val *= 10;
				val += *fmt - '0';
				fmt++;
			}
			switch (*fmt) {
			case 's':
				bp = va_arg(ap, char *);

				if (bp == NULL)
					bp = nullstr;
				if (val) {
					int slen = strlen(bp);

					if (val <= slen)
						outns(bp, val);
					else if (sgn > 0) {
						int len;
						for (len = val - slen;
						     len > 0; len--)
							outc(' ');
						outns(bp, slen);
					} else {
						outns(bp, slen);
						for (slen = val - slen;
						     slen > 0; slen--)
							outc(' ');
					}
				} else
					outs(bp);
				break;
			case 'd':
				i = va_arg(ap, int);

				negi = 0;
				if (i < 0) {
					negi = 1;
					i *= -1;
				}
				for (indx = 0; indx < 10; indx++)
					if (i >= dec[indx])
						break;
				if (i == 0)
					len = 1;
				else
					len = 10 - indx;
				if (negi)
					len++;
				if (val >= len && sgn > 0) {
					int slen;
					for (slen = val - len; slen > 0; slen--)
						outc(' ');
				}
				if (negi)
					outc('-');
				hd = 1, indx = 0;
				while (indx < 10) {
					count = 0;
					while (i >= dec[indx]) {
						count++;
						i -= dec[indx];
					}
					indx++;
					if (indx == 10)
						hd = 0;
					if (hd && !count)
						continue;
					hd = 0;
					outc('0' + count);
				}
				if (val >= len && sgn < 0) {
					int slen;
					for (slen = val - len; slen > 0; slen--)
						outc(' ');
				}
				break;
			case 'c':
				i = va_arg(ap, int);

				outc(i);
				break;
			case '\0':
				goto endprint;
			default:
				outc(*fmt);
				break;
			}
			fmt++;
			begin = fmt;
			continue;
		}

		fmt++;
	}
	if (*begin)
		outs(begin);
      endprint:
	va_end(ap);
	return;
}

void
scroll()
{
	int ln;
	struct screenline *slp;
	scrollcnt++;
	roll++;
	if (roll >= scr_lns)
		roll -= scr_lns;
	move(scr_lns - 1, 0);
	ln = (cur_ln + roll) % scr_lns;
	slp = &big_picture[ln];
	memset(slp->changed, 1, t_columns);
	slp->lchanged = 1;
}

void
rscroll()
{
	int ln;
	struct screenline *slp;
	scrollcnt--;
	if (roll > 0)
		roll--;
	else
		roll = scr_lns - 1;
	move(0, 0);
	ln = (cur_ln + roll) % scr_lns;
	slp = &big_picture[ln];
	memset(slp->changed, 1, t_columns);
	slp->lchanged = 1;
}

void
noscroll()
{
	int i;
	struct screenline bp[100];
	for (i = 0; i < scr_lns; i++)
		memcpy(bp + i, big_picture + (i + roll) % scr_lns,
		       sizeof (struct screenline));
	for (i = 0; i < scr_lns; i++)
		memcpy(big_picture + i, bp + i, sizeof (struct screenline));
	roll = 0;
}

void
saveline(int line, int mode, char *buffer)
{				/* 0 : save, 1 : restore */
	struct screenline *slp = &big_picture[(line + roll) % scr_lns];
	char *tmp = tmpbuffer;

	if (buffer)
		tmp = buffer;
	switch (mode) {
	case 0:
		memcpy(tmp, slp->data, LINELEN);
		memcpy(tmp + LINELEN, slp->mode, LINELEN);
		memcpy(tmp + LINELEN * 2, slp->color, LINELEN);
		break;
	case 1:
		memcpy(slp->data, tmp, LINELEN);
		memcpy(slp->mode, tmp + LINELEN, LINELEN);
		memcpy(slp->color, tmp + LINELEN * 2, LINELEN);
		memset(slp->changed, 1, LINELEN);
		slp->lchanged = 1;
		break;
	}
}

void
do_naws(int ln, int col)
{
	if (ln < 24 || ln > MAXT_LINES)
		ln = 24;
	if (col < 80) {
		col = 80;
	}

	if (col > 240) {
		col = 240;
	}
	if (t_lines == ln && t_columns == col)
		return;
	t_lines = ln;
	t_columns = col;
	initscr();
}

