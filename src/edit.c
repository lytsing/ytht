/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu
    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
                        Peng Piaw Foong, ppfoong@csie.ncu.edu.tw
    
    Copyright (C) 1999, KCN,Zhou Lin, kcn@cic.tsinghua.edu.cn
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "bbs.h"
#include "bbstelnet.h"
#include "edit.h"

struct edit *currentedit = NULL;
int enabledbchar;
char savedOriginLine[250];
int editview = 0;
static char searchtext[80];

static void insertChar(struct edit *edit, const char ch);
static void insertStr(struct edit *edit, char *str);
static void editMsgLine(struct edit *edit);
static void redraw(struct edit *edit);
static void preview(struct edit *edit);
static void editSelect(struct edit *edit);
static void keepSelect(struct textLine *ln);
static int doEditESCKey(struct edit *edit, int arg);
static int doEditKey(struct edit *edit, int key);
static int doEdit(struct edit *edit);
static struct edit *initEdit();
static void freeEdit(struct edit *edit);
static void parseEdit(struct edit *edit, char *p0, int size);
static void indigestion(int i);
static void input_tools(struct edit *edit);
static void valid_article(struct edit *edit, char *pmt, char *abort);
static int write_file(struct edit *edit, char *filename, int saveheader,
		      int modifyheader);
static int editOrigin(struct textLine *ln);
static void searchStr(struct edit *edit);
static void searchNextStr(struct edit *edit);
static void gotoline(struct edit *edit);

struct textLine *
allocNextLine(struct textLine *ln)
{
	struct textLine *newln;
	newln = malloc(sizeof (*newln));
	newln->prev = ln;
	if (ln) {
		newln->next = ln->next;
		ln->next = newln;
		if (newln->next)
			newln->next->prev = newln;
		if (ln->selected)
			newln->selected = 2;
		else
			newln->selected = 0;
	} else {
		newln->selected = 0;
		newln->next = NULL;
	}
	newln->len = 0;
	newln->changed = 1;
	newln->br = 0;
	return newln;
}

int
indb(char *ptr, int n)
{
	int db = 0, i;
	if (!enabledbchar)
		return 0;
	for (i = 0; i < n; i++, ptr++) {
		if (!db) {
			if ((unsigned char) *ptr >= 128)
				db = 1;
		} else
			db = 0;
	}
	return db;
}

//measureLine 取得一个显示行的内容, 最大显示宽度 maxwidth, 返回值为字节数
//p0 所指向内容若包含 '\n', '\r' 字符, 则 measureLine 不会跨过这些字符.
//若 p0 所指向字符串包含 '\t', 会对 '\t' 进行宽度解释.
//cutmode = 0: 切断到上一词首, 但是如果词首在 0, 则不回到词首
//cutmode = 1: 切断回到上一词首, 不管词首是否在 0
//cutmode = 2: 不按照词切断
int
measureLine(char *p0, int size, int maxwidth, int cutmode, int *width)
{
	int i, w, db = 0, lastspace = 0, lastwidth = 0, w1;
	char *p = p0;
	if (size == 0 || maxwidth <= 0) {
		*width = 0;
		return 0;
	}
	for (i = 0, w = 0; i < size; i++, p++) {
		if (*p == '\n' || *p == '\r')
			return i;
		if (*p == '\t') {
			w1 = (w + 8) / 8 * 8;
			if (w1 > maxwidth) {
				*width = w;
				return i;
			} else if (w1 == maxwidth) {
				*width = w1;
				return i + 1;
			}
			lastspace = i + 1;
			lastwidth = w1;
			w = w1;
			continue;
		}
		w++;
		if (!db) {
			if ((unsigned char) *p >= 128)
				db = 1;
			else if (isblank(*p)) {
				lastwidth = w;
				lastspace = i + 1;
			}
		} else {
			db = 0;
			lastspace = i + 1;
		}
		//这里应该只有 w == maxwidth, 不会发生 w > maxwidth
		if (w >= maxwidth) {
			if (db) {
				*width = w - 1;
				return i;
			} else if ((cutmode == 0 && lastspace > 0)
				   || cutmode == 1) {
				*width = lastwidth;
				return lastspace;
			} else {
				*width = w;
				return i + 1;
			}
		}
	}
	*width = w;
	return size;
}

int
measureWidth(char *ptr, int size)
{
	int i, w;
	for (i = 0, w = 0; i < size; i++) {
		if (ptr[i] == '\t')
			w = (w + 8) / 8 * 8;
		else
			w++;
	}
	return w;
}

static void
insertChar(struct edit *edit, const char ch)
{
	struct textLine *ln = edit->curline, *newln;
	int len, width;
	if (!edit->overwrite)
		memmove(ln->text + edit->col + 1, ln->text + edit->col,
			ln->len - edit->col);
	ln->text[edit->col] = ch;
	edit->col++;
	edit->colcur = measureWidth(ln->text, edit->col);
	if (!edit->overwrite || edit->col > ln->len)
		ln->len++;
	ln->changed = 1;
	len = measureLine(ln->text, ln->len, MAXLINELEN, 0, &width);
	if (len == ln->len && width < MAXLINELEN) {
		edit->position = edit->colcur;
		return;
	}
	newln = allocNextLine(ln);
	newln->br = ln->br;
	ln->br = 0;
	memcpy(newln->text, ln->text + len, ln->len - len);
	newln->len = ln->len - len;
	ln->len = len;
	edit->redrawall = 1;
	reflow(newln, 0);
	if (edit->col > len || edit->colcur >= MAXLINELEN) {
		edit->curline = ln->next;
		edit->col = edit->col - len;
		edit->colcur = measureWidth(edit->curline->text, edit->col);
		edit->line++;
		edit->linecur++;
	}
	edit->position = edit->colcur;
}

static void
insertStr(struct edit *edit, char *str)
{
	while (*str) {
		insertChar(edit, *str);
		str++;
	}
}

int
reflow(struct textLine *ln, int tillend)
{
	int len, i;
	struct textLine *nextln;
	while (ln && ln->next && !ln->br) {
		nextln = ln->next;
		if ((ln->selected && !nextln->selected)
		    || (!ln->selected && nextln->selected)) {
			ln = nextln;
			continue;
		}
		if (!ln->len) {
			nextln->prev = ln->prev;
			if (nextln->next)
				nextln->next->prev = ln;
			keepSelect(ln);
			*ln = *nextln;	//curline 可能正指向 ln, 所以释放 nextln 而不是 ln
			free(nextln);
			ln->changed = 1;
			continue;
		}
		len = min(MAXLINELEN - ln->len, nextln->len);
		if (len) {
			memcpy(ln->text + ln->len, nextln->text, len);
			//从下一行抓取的字符数
			len =
			    measureLine(ln->text, ln->len + len, MAXLINELEN, 0,
					&i) - ln->len;
		}
		if (nextln->len && len <= 0)
			break;
		ln->len += len;
		nextln->len -= len;
		memmove(nextln->text, nextln->text + len, nextln->len);
		ln->changed = 1;
		nextln->changed = 1;
		if (!nextln->len
		    && measureWidth(ln->text, ln->len) < MAXLINELEN) {
			ln->next = nextln->next;
			if (nextln->next)
				nextln->next->prev = ln;
			if (nextln->br)
				ln->br = 1;
			keepSelect(nextln);
			free(nextln);
			if (ln->br)
				break;
			nextln = ln->next;
		}
		ln = nextln;
	}
	return 0;
}

void
printTextLine(struct textLine *ln)
{
	int i, y, x;
	if (ln->selected)
		prints("\033[7m");
	for (i = 0; i < ln->len; i++) {
		if (ln->text[i] == '\033')
			prints("\033[33m*\033[37m");
		else if (ln->text[i] == '\t')
			prints("\t");
		else
			outc(ln->text[i]);
	}
	if (!editview) {
		getyx(&y, &x);
		if (ln->br)
			prints("\033[33m~\033[37m");
		else if (x < MAXLINELEN - 1)
			prints("\033[32m~\033[37m");
	}
	clrtoeol();
	prints("\033[0m");
}

#if 0
static int
docheck(struct edit *edit)
{
	struct textLine *l1;
	for (l1 = edit->firstline; l1; l1 = l1->next) {
		if (l1 == edit->curline)
			break;
	}
	if (!l1)
		return -1;
	for (l1 = edit->firstline; l1->next; l1 = l1->next) ;
	for (; l1; l1 = l1->prev)
		if (l1 == edit->curline)
			break;
	if (!l1)
		return -2;
	return 0;
}
#endif

static void
editMsgLine(struct edit *edit)
{
	static int ck, havenewmail;
	if (!edit) {
		ck = 0;
		havenewmail = chkmail();
		return;
	}
	if (talkrequest) {
		talkreply();
		clear();
		edit->redrawall = 1;
	}
	if (++ck >= 10) {
		havenewmail = chkmail();
		ck = 0;
	}
	move(t_lines - 1, 0);
	prints("\033[1;33;44m【%s】",
	       havenewmail ? "\033[5;32m信\033[m\033[1;33;44m" : "  ");
	prints("【%s】",
	       have_msg_unread ? "\033[5;32mMSG\033[m\033[1;33;44m" : "   ");
	prints(" \033[31mCtrl-Q\033[33m 求救 ");
	prints
	    ("状态 [\033[32m%s\033[33m][\033[32m%d\033[33m,\033[32m%d\033[33m][\033[32m%s\033[33m][\033[32m%s\033[33m] ",
	     edit->overwrite ? "Rep" : "Ins", edit->line + 1,
	     edit->col + 1, "X", editview ? " " : "~");
	prints("时间\033[1;33;44m【\033[1;32m%16s\033[33m】     \033[m",
	       Ctime(now_t));
}

static void
redraw(struct edit *edit)
{
	static struct textLine *(lns[MAXT_LINES - 1]);
	static int lastlen[MAXT_LINES - 1];
	struct textLine *ln;
	int i;
      AGAIN:
#if 0
	i = docheck(edit);
	if (i) {
		move(1, 1);
		prints("Error %d", i);
		igetkey();
		sleep(20);
	}
#endif
	if (edit->linecur >= t_lines - 1) {
		edit->redrawall = 1;
		edit->linecur = t_lines - 2;
	}
	if (edit->linecur < 0) {
		edit->redrawall = 1;
		edit->linecur = 0;
	}

	for (i = edit->linecur, ln = edit->curline; i; i--) {
		//if (ln != NULL) 
		ln = ln->prev;
	}

	for (i = 0; i < t_lines - 1; i++, ln = ln ? ln->next : NULL) {
		if (!(edit->redrawall || ln != lns[i] || (ln && ln->changed)))
			continue;
		move(i, 0);
		if (ln) {
			ln->changed = 0;
			printTextLine(ln);
			lastlen[i] = ln->len + (ln->br || !ln->next);
		} else if (edit->redrawall || lastlen[i]) {
			lastlen[i] = 0;
			if (!editview)
				prints("\033[44m");
			clrtoeol();
			if (!editview)
				prints("\033[0m");
		}
		lns[i] = ln;
	}
	edit->redrawall = 0;

	editMsgLine(edit);
	move(edit->linecur, edit->colcur);
	if (edit->redrawall)
		goto AGAIN;
}

static void
preview(struct edit *edit)
{
	struct textLine *ln;
	int len = 0, i;
	char *buf = malloc(1024 * 16);
	if (!buf)
		return;
	for (i = edit->linecur, ln = edit->curline; i; i--)
		ln = ln->prev;
	while (ln && len + ln->len < 1024 * 16 - 1) {
		memcpy(buf + len, ln->text, ln->len);
		len += ln->len;
		if (ln->br) {
			buf[len] = '\n';
			len++;
		}
		ln = ln->next;
	}
	mem_show(buf, len, 0, t_lines, "");
	free(buf);
	edit->redrawall = 1;
	move(t_lines - 1, 0);
	prints
	    ("\033[1;33;44m已显示彩色编辑成果，请按任意键返回编辑画面...\033[0m");
	igetkey();
}

static void
editSelect(struct edit *edit)
{
	int passed = 0, hasmark = 0;
	struct textLine *ln = edit->firstline, *ln2;
	while (ln) {
		if (ln->selected == 1)
			break;
		if (ln == edit->curline)
			passed = 1;
		if (ln->selected) {
			if (!passed) {
				ln->selected = 0;
				ln->changed = 1;
			} else
				hasmark = 1;
		}
		ln = ln->next;
	}
	if (!ln) {
#if 0
		//这里先处理一下或许的代码错误, 原则上, 如果有被选择的区域, 
		//就应该有 selected = 1 的行存在
		if (hasmark) {
			errlog("错误的 selected 标记!");
			ln = edit->firstline;
			while (ln) {
				if (ln->selected) {
					ln->selected = 0;
					ln->changed = 1;
				}
				ln = ln->next;
			}
		}
#endif
		//选择当前行
		edit->curline->selected = 1;
		edit->curline->changed = 1;
		return;
	}
	//如果在选择区域的领头行进行操作, 则取消选择区
	if (ln == edit->curline) {
		while (ln && ln->selected) {
			ln->selected = 0;
			ln->changed = 1;
			ln = ln->next;
		}
		return;
	}
	//如果领头行在 curline 之后             
	if (passed) {
		ln2 = edit->curline;
		while (!ln2->selected) {
			ln2->selected = 2;	//不是领头行, 赋值为 2
			ln2->changed = 1;
			ln2 = ln2->next;
		}
		ln2 = ln->next;
		while (ln2 && ln2->selected) {
			ln2->selected = 0;
			ln2->changed = 1;
			ln2 = ln2->next;
		}
		return;
	}
	//如果当前行在领头行之后
	ln = ln->next;
	while (1) {
		if (!ln->selected) {
			ln->selected = 2;
			ln->changed = 1;
		}
		if (ln == edit->curline)
			break;
		ln = ln->next;
	}
	ln = ln->next;
	while (ln && ln->selected) {
		ln->selected = 0;
		ln->changed = 1;
		ln = ln->next;
	}
}

static void
clearSelect(struct edit *edit)
{
	struct textLine *ln = edit->firstline;
	while (ln && !ln->selected)
		ln = ln->next;
	if (!ln)
		return;
	while (ln && ln->selected) {
		ln->changed = 1;
		ln->selected = 0;
		ln = ln->next;
	}
}

static void
deleteSelect(struct edit *edit)
{
	struct textLine *ln = edit->firstline;
	int passed = 0;
	while (ln && !ln->selected) {
		if (ln == edit->curline)
			passed = 1;
		ln = ln->next;
	}
	if (!ln)
		return;
	//将 curline 移过来, 否则 reflow 会有问题得
	if (!passed) {
		while (edit->curline != ln) {
			edit->curline = edit->curline->prev;
			edit->line--;
			edit->linecur--;
		}
	} else {
		while (edit->curline != ln) {
			edit->curline = edit->curline->next;
			edit->line++;
			edit->linecur++;
		}
	}
	edit->col = edit->colcur = 0;
	while (ln && ln->selected) {
		ln->changed = 1;
		ln->br = 0;
		ln->selected = 0;
		ln->len = 0;
		ln = ln->next;
	}
	reflow(edit->curline, 0);
}

static void
copySelect(struct edit *edit)
{
	struct textLine *ln;
	if (edit->curline->selected)
		return;
	ln = edit->firstline;
	while (ln && !ln->selected)
		ln = ln->next;
	if (!ln)
		return;
	while (ln && ln->selected) {
		ln->text[ln->len] = 0;
		insertStr(edit, ln->text);
		if (ln->br)
			doEditKey(edit, '\n');
		ln = ln->next;
	}
}

//对即将删除的行，如果该行为被选择区的首行，则要将首行标记转移
static void
keepSelect(struct textLine *ln)
{
	if (ln->selected != 1)
		return;
	if (ln->prev && ln->prev->selected)
		ln->prev->selected = 1;
	else if (ln->next && ln->next->selected)
		ln->next->selected = 1;
}

static void
insertToFile(struct edit *edit)
{
	struct textLine *ln;
	char ans[3], *ptr, filename[STRLEN];
	int n, full = 0;
	FILE *fp;
	for (ln = edit->firstline; ln && !ln->selected; ln = ln->next) ;
	if (!ln) {
		ln = edit->firstline;
		full = 1;
		ptr = "把整篇文章写入剪贴簿第几页? (0-7) (Q 取消) [0]: ";
	} else
		ptr = "把标记块写入剪贴簿第几页? (0-7) (Q 取消) [0]: ";
	getdata(t_lines - 1, 0, ptr, ans, 2, DOECHO, YEA);
	if (ans[0] == 'Q')
		return;
	n = atoi(ans);
	if (n < 0 || n > 7)
		return;
	ans[0] = n + '0';
	ans[1] = 0;
	sethomefile(filename, currentuser->userid, "clip_");
	strcat(filename, ans);
	fp = fopen(filename, "w");
	if (!fp) {
		move(t_lines - 1, 0);
		prints("无法打开剪贴簿第 %s 页！\t\t\t", ans);
		igetkey();
		return;
	}
	while (ln && (full || ln->selected)) {
		fwrite(ln->text, 1, ln->len, fp);
		if (ln->br)
			fputc('\n', fp);
		ln = ln->next;
	}
	fclose(fp);
	move(t_lines - 1, 0);
	prints("已写入剪贴簿第 %s 页。\t\t\t", ans);
	igetkey();
}

static void
insertFromFile(struct edit *edit)
{
	char ans[3], ch, filename[STRLEN];
      struct mmapfile mf = { ptr:NULL };
	int n;
	getdata(t_lines - 1, 0, "导入剪贴簿第几页? (0-7) (Q 取消) [0]: ", ans,
		2, DOECHO, YEA);
	if (ans[0] == 'Q')
		return;
	n = atoi(ans);
	if (n < 0 || n > 7)
		return;
	ans[0] = n + '0';
	ans[1] = 0;
	sethomefile(filename, currentuser->userid, "clip_");
	strcat(filename, ans);
	MMAP_TRY {
		if (mmapfile(filename, &mf) < 0)
			MMAP_RETURN_VOID;
		for (n = 0; n < mf.size; n++) {
			ch = mf.ptr[n];
			if (isprint2(ch) || ch == '\t' || ch == KEY_ESC) {
				insertChar(edit, ch);
				continue;
			}
			if (ch == '\n')
				doEditKey(edit, ch);
		}
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	redraw(edit);
	move(t_lines - 1, 0);
	prints("已导入剪贴簿第 %s 页。\t\t\t", ans);
	igetkey();
}

static int
doEditESCKey(struct edit *edit, int arg)
{
	char buf[32];
	int i;
	switch (toupper(arg)) {
	case 'C':
		preview(edit);
		break;
	case 'B':
		move(t_lines - 1, 0);
		prints
		    ("\033[1;33;44m背景颜色? 0)黑 1)红 2)绿 3)黄 4)深蓝 5)粉红 6)浅蓝 7)白\033[0m");
		i = igetkey() - '0';
		if (i < 0 || i > 7)
			break;
		sprintf(buf, "\033[4%dm", i);
		insertStr(edit, buf);
		break;
	case 'F':
		move(t_lines - 1, 0);
		prints
		    ("\033[1;33;44m前景颜色? 0)黑 1)红 2)绿 3)黄 4)深蓝 5)粉红 6)浅蓝 7)白\033[0m");
		i = igetkey() - '0';
		if (i < 0 || i > 7)
			break;
		sprintf(buf, "\033[3%dm", i);
		insertStr(edit, buf);
		break;
	case 'R':
		insertStr(edit, "\033[0m");
		break;
	case 'Q':
		clearSelect(edit);
		break;
	case 'D':
		deleteSelect(edit);
		break;
	case 'E':
		insertToFile(edit);
		break;
	case 'I':
		insertFromFile(edit);
		break;
	case 'S':
		searchStr(edit);
		break;
	case 'N':
		searchNextStr(edit);
		break;
	case 'G':
		gotoline(edit);
		break;
	default:
		break;
	}
	return 0;
}

static int
doEditKey(struct edit *edit, int key)
{
	struct textLine *newln;
	int updownkey = 0, i;

	switch (key) {
	case '\n':
	case '\r':
		newln = allocNextLine(edit->curline);
		newln->len = edit->curline->len - edit->col;
		memcpy(newln->text, edit->curline->text + edit->col,
		       newln->len);
		newln->br = edit->curline->br;
		edit->curline->changed = 1;
		edit->curline->br = 1;
		edit->curline->len = edit->col;
		edit->col = edit->colcur = 0;
		edit->curline = newln;
		edit->line++;
		edit->linecur++;
		reflow(edit->curline, 0);
		break;
	case KEY_LEFT:
		if (edit->col) {
			edit->col--;
			if (indb(edit->curline->text, edit->col))
				edit->col--;
			edit->colcur =
			    measureWidth(edit->curline->text, edit->col);
		} else if (edit->curline->prev) {
			edit->line--;
			edit->linecur--;
			edit->curline = edit->curline->prev;
			edit->col = edit->curline->len;
			if (!edit->curline->br)
				doEditKey(edit, KEY_LEFT);
			else
				edit->colcur =
				    measureWidth(edit->curline->text,
						 edit->col);
		} else
			bell();
		break;
	case KEY_RIGHT:
		if (edit->col < edit->curline->len) {
			edit->col++;
			if (edit->col < edit->curline->len
			    && indb(edit->curline->text, edit->col))
				edit->col++;
			edit->colcur =
			    measureWidth(edit->curline->text, edit->col);
			if (edit->colcur >= MAXLINELEN) {
				if (!edit->curline->next)
					edit->curline->next =
					    allocNextLine(edit->curline);
				edit->line++;
				edit->linecur++;
				edit->curline = edit->curline->next;
				edit->col = edit->colcur = 0;
			}
		} else if (edit->curline->next) {
			edit->line++;
			edit->linecur++;
			edit->curline = edit->curline->next;
			edit->col = edit->colcur = 0;
			if (!edit->curline->prev->br)
				doEditKey(edit, KEY_RIGHT);
		} else
			bell();
		break;
	case Ctrl('P'):
	case KEY_UP:
		if (edit->col == 0 && edit->position == MAXLINELEN
		    && edit->curline->prev
		    && measureWidth(edit->curline->prev->text,
				    edit->curline->prev->len) == MAXLINELEN) {
			edit->curline = edit->curline->prev;
			edit->line--;
			edit->linecur--;
		}
		if (edit->curline->prev) {
			updownkey = 1;
			edit->line--;
			edit->linecur--;
			edit->curline = edit->curline->prev;
		} else {
			updownkey = 2;
			bell();
		}
		break;
	case Ctrl('N'):
	case KEY_DOWN:
		if (edit->col == 0 && edit->curline->len != 0
		    && edit->position == MAXLINELEN && edit->curline->prev
		    && measureWidth(edit->curline->prev->text,
				    edit->curline->prev->len) == MAXLINELEN) {
			edit->curline = edit->curline->prev;
			edit->line--;
			edit->linecur--;
		}
		if (edit->curline->next) {
			updownkey = 1;
			edit->line++;
			edit->linecur++;
			edit->curline = edit->curline->next;
		} else {
			updownkey = 2;
			bell();
		}
		break;
	case Ctrl('B'):
	case KEY_PGUP:
		if (!edit->curline->prev) {
			bell();
			break;
		}
		for (i = t_lines / 2; i && edit->curline->prev; i--) {
			edit->line--;
			edit->linecur--;
			edit->curline = edit->curline->prev;
		}
		updownkey = 1;
		break;
	case Ctrl('F'):
	case KEY_PGDN:
		if (!edit->curline->next) {
			bell();
			break;
		}
		for (i = t_lines / 2; i && edit->curline->next; i--) {
			edit->line++;
			edit->linecur++;
			edit->curline = edit->curline->next;
		}
		updownkey = 1;
		break;
	case Ctrl('A'):
	case KEY_HOME:
		edit->col = edit->colcur = 0;
		break;
	case Ctrl('E'):
	case KEY_END:
		edit->col = edit->curline->len;
		edit->colcur = measureWidth(edit->curline->text, edit->col);
		if (edit->colcur >= MAXLINELEN) {
			if (!edit->curline->next)
				edit->curline->next =
				    allocNextLine(edit->curline);
			edit->line++;
			edit->linecur++;
			edit->curline = edit->curline->next;
			edit->col = edit->colcur = 0;
		}
		edit->position = MAXLINELEN;
		updownkey = 2;
		break;
	case Ctrl('S'):
		edit->col = edit->colcur = edit->line = edit->linecur = 0;
		edit->curline = edit->firstline;
		break;
	case Ctrl('T'):
		edit->col = edit->colcur = edit->line = edit->linecur = 0;
		edit->curline = edit->firstline;
		while (edit->curline->next) {
			edit->curline = edit->curline->next;
			edit->line++;
			edit->linecur++;
		}
		break;
	case Ctrl('H'):
	case '\177':		/* backspace */
		if (!edit->col && !edit->curline->prev) {
			bell();
			updownkey = 2;
			break;
		}
		doEditKey(edit, KEY_LEFT);
		//继续执行 KEY_DEL
	case Ctrl('D'):
	case KEY_DEL:
		edit->curline->changed = 1;
		if (edit->col < edit->curline->len) {
			char *ptr = edit->curline->text;
			if (edit->col + 1 < edit->curline->len
			    && indb(ptr, edit->col + 1)) {
				memmove(ptr + edit->col, ptr + edit->col + 2,
					edit->curline->len - edit->col - 2);
				edit->curline->len -= 2;
			} else {
				memmove(ptr + edit->col, ptr + edit->col + 1,
					edit->curline->len - edit->col - 1);
				edit->curline->len--;
			}
		} else if (edit->curline->br) {
			edit->curline->br = 0;
		} else if (edit->curline->next) {
			struct textLine *ln;
			char *ptr;
			ln = edit->curline->next;
			ptr = ln->text;
			if (ln->len) {
				if (indb(ptr, 1) && ln->len > 1) {
					memmove(ptr, ptr + 2, ln->len - 2);
					ln->len -= 2;
				} else {
					memmove(ptr, ptr + 1, ln->len - 1);
					ln->len--;
				}
				ln->changed = 1;
			} else {
				ln->br = 0;
			}
		}
		reflow(edit->curline, 0);
		break;
	case Ctrl('Y'):
		updownkey = 0;
		edit->col = edit->colcur = 0;
		if (!edit->curline->prev && !edit->curline->next) {
			edit->curline->changed = 1;
			edit->curline->len = 0;
			edit->curline->br = 0;
		} else {
			struct textLine *ln = edit->curline;
			if (ln->prev) {
				ln->prev->next = ln->next;
				if (ln->br)
					ln->prev->br = 1;
			} else
				edit->firstline = ln->next;

			if (ln->next) {
				ln->next->prev = ln->prev;
				edit->curline = ln->next;
			} else {
				edit->curline = ln->prev;
				edit->line--;
				edit->linecur--;
			}
			keepSelect(ln);
			free(ln);
		}
		break;
	case Ctrl('K'):	/* delete to end of line */
		edit->curline->len = edit->col;
		edit->curline->changed = 1;
		reflow(edit->curline, 0);
		break;
	case Ctrl('O'):
		input_tools(edit);
		break;
	case Ctrl('G'):
	case KEY_INS:
		edit->overwrite = !edit->overwrite;
		break;
	case Ctrl('Q'):
		show_help("help/edithelp");
		edit->redrawall = 1;
		break;
	case Ctrl('R'):
		editview = 1 - editview;
		edit->redrawall = 1;
		break;
	case Ctrl('U'):
		editSelect(edit);
		break;
	case Ctrl('C'):
		copySelect(edit);
		break;
	default:
		bell();
		updownkey = 2;
		break;
	}
	if (updownkey == 1) {
		if (edit->position == 0) {
			edit->col = 0;
			edit->colcur = 0;
		} else {
			edit->col =
			    measureLine(edit->curline->text, edit->curline->len,
					edit->position, 2, &edit->colcur);
			if (edit->colcur >= MAXLINELEN) {
				if (!edit->curline->next)
					edit->curline->next =
					    allocNextLine(edit->curline);
				edit->line++;
				edit->linecur++;
				edit->curline = edit->curline->next;
				edit->col = edit->colcur = 0;
			}
		}
	} else if (!updownkey) {
		edit->position = edit->colcur;
	}

	return 1;
}

static int
doEdit(struct edit *edit)
{
	int ch;
	edit->redrawall = 1;
	while (1) {
		//if (ibufsize == icurrchar) {
		redraw(edit);
		//}
		ch = igetkey();
		if (ch == Ctrl('X') || ch == Ctrl('W'))
			return 1;
		if (isprint2(ch) || ch == '\t'
		    || (ch == KEY_ESC && KEY_ESC_arg == KEY_ESC)) {
			insertChar(edit, ch);
			continue;
		}
		if (ch == KEY_ESC) {
			doEditESCKey(edit, KEY_ESC_arg);
			continue;
		}
		doEditKey(edit, ch);
	}
}

static struct edit *
initEdit()
{
	struct edit *edit = malloc(sizeof (struct edit));
	edit->firstline = allocNextLine(NULL);
	edit->curline = edit->firstline;
	edit->line = edit->linecur = edit->col = edit->colcur = edit->position =
	    0;
	edit->overwrite = 0;
	currentedit = edit;
	return edit;
}

static void
freeEdit(struct edit *edit)
{
	struct textLine *ln;
	currentedit = NULL;
	while (edit->firstline) {
		ln = edit->firstline->next;
		free(edit->firstline);
		edit->firstline = ln;
	}
	free(edit);
}

static void
parseEdit(struct edit *edit, char *p0, int size)
{
	struct textLine *ln;
	char *ptr, *p1;
	int w;

	ln = edit->firstline;
	while (ln->next)
		ln = ln->next;
	for (ptr = p0, p1 = p0 + size; ptr < p1;) {
		if (*ptr == '\r') {
			ptr++;
			continue;
		}
		if (*ptr == '\n') {
			ptr++;
			if (editOrigin(ln)) {
				strsncpy(savedOriginLine, ln->text,
					 min(ln->len + 1,
					     sizeof (savedOriginLine)));
				ln->br = 0;
				ln->len = 0;
				continue;
			}
			ln->br = 1;
			ln = allocNextLine(ln);
			continue;
		}
		if (ln->len)
			ln = allocNextLine(ln);
		ln->len = measureLine(ptr, p1 - ptr, MAXLINELEN, 0, &w);
		memcpy(ln->text, ptr, ln->len);
		ptr += ln->len;
	}
}

static int
saveEdit(struct edit *edit, FILE * fp)
{
	struct textLine *ln = edit->firstline;
	int br = 0;
	while (ln) {
		fwrite(ln->text, 1, ln->len, fp);
		if (ln->br)
			fputc('\n', fp);
		br = ln->br;
		ln = ln->next;
	}
	if (savedOriginLine[0])
		fprintf(fp, "%s%s\n", br ? "" : "\n", savedOriginLine);
	return 0;
}

static void
searchStr(struct edit *edit)
{
	char tmp[STRLEN];
	tmp[0] = '\0';
	getdata(t_lines - 1, 0, "搜寻字串: ", tmp, 65, DOECHO, 0);
	if (tmp[0] == '\0')
		return;
	else
		strcpy(searchtext, tmp);
	searchNextStr(edit);
	return;
}

static void
searchNextStr(struct edit *edit)
{
	struct textLine *tmpline;
	int line, linecur;
	char *pos;
	char *ptr;
	if (0 == searchtext[0]) {
		return;
	}
	tmpline = edit->curline;
	linecur = edit->linecur;
	line = edit->line;
	while (tmpline) {
		tmpline->text[tmpline->len] = 0;
		if (tmpline == edit->curline) {
			ptr = tmpline->text + edit->col;
		} else {
			ptr = tmpline->text;
		}
		if ((pos = strstr(ptr, searchtext))) {
			edit->curline = tmpline;
			edit->line = line;
			edit->linecur = linecur;
			edit->position = edit->col =
			    (pos - tmpline->text + strlen(searchtext));
			edit->colcur = measureWidth(tmpline->text, edit->col);
			break;
		}
		tmpline = tmpline->next;
		line++;
		linecur++;
	}
	return;
}

static void
gotoline(struct edit *edit)
{
	char tmp[10];
	struct textLine *tmpline, *lastline = NULL;
	int line, linecur, gline;
	int i;
	tmp[0] = '\0';
	getdata(t_lines - 1, 0, "跳转到哪行: ", tmp, 10, DOECHO, 0);
	if (tmp[0] == '\0')
		return;
	gline = atoi(tmp);
	tmpline = edit->firstline;
	linecur = 0;
	line = 0;
	for (i = 1; i < gline; i++) {
		if (tmpline == NULL) {
			tmpline = lastline;
			line--;
			linecur--;
			break;
		}
		lastline = tmpline;
		tmpline = tmpline->next;
		line++;
		linecur++;
	}
	edit->curline = tmpline;
	edit->line = line;
	edit->linecur = linecur;
	edit->position = edit->col = edit->colcur = 0;
	return;
}

int
editor(char *filename, int saveheader, int modifyheader)
{
	struct edit *edit;
      struct mmapfile mf = { ptr:NULL };
	FILE *fp;
	int offset = 0, retv;

	editMsgLine(NULL);
	if (USERDEFINE(currentuser, DEF_POSTNOMSG))
		block_msg();
	edit = initEdit();
	savedOriginLine[0] = 0;
	if (filename) {
		if (!saveheader && !modifyheader) {
			fp = fopen(filename, "r");
			if (!fp)
				return -1;
			keepoldheader(fp, KEEPHEADER);
			offset = ftell(fp);
			fclose(fp);
		}
		MMAP_TRY {
			if (mmapfile(filename, &mf) >= 0)
				parseEdit(edit, mf.ptr + offset,
					  mf.size - offset);
		}
		MMAP_CATCH {
		}
		MMAP_END mmapfile(NULL, &mf);
	}
	while (1) {
		doEdit(edit);
		retv = write_file(edit, filename, saveheader, modifyheader);
		if (retv != KEEP_EDITING)
			break;
	}
	freeEdit(edit);
	if (USERDEFINE(currentuser, DEF_POSTNOMSG))
		unblock_msg();
	return retv;
}

static void
indigestion(i)
int i;
{
	prints("SERIOUS INTERNAL INDIGESTION CLASS %d\n", i);
}

/* ----------------------------------------------------- */
/* 编辑处理：主程式、键盘处理				 */
/* ----------------------------------------------------- */

/* input tool */
static void
input_tools(struct edit *edit)
{
	const char *msg =
	    "内码输入工具: 1.加减乘除  2.一二三四  3.ＡＢＣＤ  4.横竖斜弧 ？[Q]";
	const char *ansi1[7][10] = {
		{"＋", "－", "×", "÷", "±", "∵", "∴", "∈", "≡", "∝"},
		{"∑", "∏", "∪", "∩", "∫", "∮", "∶", "∧", "∨", "∷"},
		{"≌", "≈", "∽", "≠", "≮", "≯", "≤", "≥", "∞", "∠"},
		{"〔", "〕", "（", "）", "〈", "〉", "《", "》", "「", "」"},
		{"『", "』", "〖", "〗", "【", "】", "［", "］", "｛", "｝"},
		{"︵", "︶", "︹", "︺", "︿", "﹀", "︽", "︾", "﹁", "﹂"},
		{"﹃", "﹄", "︻", "︼", "︷", "︸", "‘", "’", "“", "”"}
	};

	const char *ansi2[7][10] = {
		{"⒈", "⒉", "⒊", "⒋", "⒌", "⒍", "⒎", "⒏", "⒐", "⒑"},
		{"⒒", "⒓", "⒔", "⒕", "⒖", "⒗", "⒘", "⒙", "⒚", "⒛"},
		{"⑴", "⑵", "⑶", "⑷", "⑸", "⑹", "⑺", "⑻", "⑼", "⑽"},
		{"①", "②", "③", "④", "⑤", "⑥", "⑦", "⑧", "⑨", "⑩"},
		{"㈠", "㈡", "㈢", "㈣", "㈤", "㈥", "㈦", "㈧", "㈨", "㈩"},
		{"ⅰ", "ⅱ", "ⅲ", "ⅳ", "ⅴ", "ⅵ", "ⅶ", "ⅷ", "ⅸ", "ⅹ"},
		{"Ⅰ", "Ⅱ", "Ⅲ", "Ⅳ", "Ⅴ", "Ⅵ", "Ⅶ", "Ⅷ", "Ⅸ", "Ⅹ"},
	};

	const char *ansi3[7][10] = {
		{"Α", "Β", "Γ", "Δ", "Ε", "Ζ", "Η", "Θ", "Ι", "Κ"},
		{"Λ", "Μ", "Ν", "Ξ", "Ο", "Π", "Ρ", "Σ", "Τ", "Υ"},
		{"Φ", "Χ", "Ψ", "Ω", "α", "β", "γ", "δ", "ε", "ζ"},
		{"η", "θ", "ι", "κ", "λ", "μ", "ν", "ξ", "ο", "π"},
		{"ρ", "σ", "τ", "υ", "φ", "χ", "ψ", "ψ", "ω", "㎎"},
		{"℡", "㎏", "㎜", "㎝", "㎞", "㎡", "㏄", "〾", "⿰", "⿱"},
		{"⿲", "⿳", "⿴", "⿵", "⿶", "⿷", "⿸", "⿹", "⿺", "⿻"}
	};

	const char *ansi4[7][10] = {
		{"▁", "▂", "▃", "▄", "▅", "▆", "▇", "█", "▉", "▊"},
		{"▋", "▌", "▍", "▎", "▏", "▓", "╱", "╲", "╳", "※"},
		{"─", "│", "┌", "┐", "└", "┘", "├", "┤", "┬", "┴"},
		{"┼", "↖", "↗", "↘", "↙", "→", "←", "↑", "↓", "√"},
		{"▼", "▽", "◢", "◣", "◥", "◤", "╭", "╮", "╯", "╰"},
		{"♂", "♀", "☉", "⊕", "〇", "◎", "〓", "℉", "℃", "㊣"},
		{"☆", "★", "◇", "◆", "□", "■", "△", "▲", "○", "●"}
	};

	char buf[128], tmp[5];
	const char *(*show)[10];
	int ch, i, page;

	move(t_lines - 1, 0);
	clrtoeol();
	prints("%s", msg);
	ch = igetkey();

	if (ch < '1' || ch > '4')
		return;

	switch (ch) {
	case '1':
		show = ansi1;
		break;
	case '2':
		show = ansi2;
		break;
	case '3':
		show = ansi3;
		break;
	case '4':
	default:
		show = ansi4;
		break;
	}

	page = 0;
	for (;;) {
		buf[0] = '\0';

		sprintf(buf, "第%d页:", page + 1);
		for (i = 0; i < 10; i++) {
			sprintf(tmp, "%d%s%s ", i, ".", show[page][i]);
			strcat(buf, tmp);
		}
		strcat(buf, "(P:上  N:下)[Q]\0");
		move(t_lines - 1, 0);
		clrtoeol();
		prints("%s", buf);
		ch = igetkey();

		if (ch == 'p') {
			if (page)
				page -= 1;
		} else if (ch == 'n') {
			if (page != 6)
				page += 1;
		} else if (ch < '0' || ch > '9') {
			buf[0] = '\0';
			break;
		} else if (edit) {
			const char *ptr = show[page][ch - '0'];
			while (*ptr) {
				insertChar(edit, *ptr);
				ptr++;
			}
			break;
		} else {
			break;
		}
		buf[0] = '\0';
	}
}

char save_title[STRLEN];
int in_mail;

int
write_header(fp, deliver)
FILE *fp;
int deliver;
{
	int noname = 0;
	extern char fromhost[];
	extern struct postheader header;
	struct boardmem *bp;
	char uid[20];
	char uname[NAMELEN];
	time_t now;

	now = time(0);
	if (deliver) {
		sprintf(uname, "deliver");
		sprintf(uid, "deliver");
	} else {
		strsncpy(uname, currentuser->username, sizeof(uname));
		strsncpy(uid, currentuser->userid, sizeof(uid));
	}
	save_title[STRLEN - 10] = '\0';
	bp = getbcache(currboard);
	//这里bp会不存在么？fp还在那个目录里呢啊
	//有时候就不会存在啊,比如我删除了版面 但是目录还在...
	if (bp)
		noname = bp->header.flag & ANONY_FLAG;
	if (in_mail)
		fprintf(fp, "寄信人: %s (%s)\n", uid, uname);
	else {
		fprintf(fp, "发信人: %s (%s), 信区: %s\n",
			(noname
			 && header.chk_anony) ? "Anonymous" : uid, (noname
								    &&
								    header.
								    chk_anony) ?
			"我是匿名天使" : uname, currboard);
	}
	fprintf(fp, "标  题: %s\n", save_title);
	fprintf(fp, "发信站: %s (%24.24s), %s", MY_BBS_NAME, ctime(&now),
		local_article ? "本站(" MY_BBS_DOMAIN ")" : "转信("
		MY_BBS_DOMAIN ")");
	if (in_mail)
		fprintf(fp, "\n来  源: %s", fromhost);
	fprintf(fp, "\n\n");
	return 0;
}

void
addsignature(fp, blank)
FILE *fp;
int blank;
{
	FILE *sigfile;
	int i, valid_ln = 0;
	int sig;
	char tmpsig[MAXSIGLINES][256];
	char inbuf[256];
	char fname[STRLEN];

	if (!ftell(fp) || blank)
		fputs("\n", fp);
	fputs("--\n", fp);
	if (USERPERM(currentuser, PERM_DENYSIG))
		return;
	setuserfile(fname, "signatures");
	if ((sigfile = fopen(fname, "r")) == NULL) {
		return;
	}
	sig = (uinfo.signature == -1) ? (numofsig ? (rand() % numofsig + 1) : 0) : uinfo.signature;
	for (i = 1; i <= (sig - 1) * MAXSIGLINES && sig != 1; i++) {
		if (!fgets(inbuf, sizeof (inbuf), sigfile)) {
			fclose(sigfile);
			return;
		}
	}
	for (i = 1; i <= MAXSIGLINES; i++) {
		if (fgets(inbuf, sizeof (inbuf), sigfile)) {
			if (inbuf[0] != '\n')
				valid_ln = i;
			strcpy(tmpsig[i - 1], inbuf);
		} else
			break;
	}
	fclose(sigfile);
	for (i = 1; i <= valid_ln; i++)
		fputs(tmpsig[i - 1], fp);
}

static void
valid_article(struct edit *edit, char *pmt, char *abort)
{
	struct textLine *p = edit->firstline;
	char ch;
	int total, lines, len, sig, y;

	if (uinfo.mode == POSTING) {
		total = lines = len = sig = 0;
		while (p != NULL) {
			p->text[p->len] = 0;
			if (!sig) {
				ch = p->text[0];
				if (strcmp(p->text, "--") == 0)
					sig = 1;
				else if (ch != ':' && ch != '>' && ch != '=' &&
					 !strstr(p->text, "的大作中提到: 】")) {
					lines++;
					len += p->len;
				}
			}
			total++;
			p = p->next;
		}
		y = 2;
		if (lines < total * 0.35 - MAXSIGLINES) {
			move(y, 0);
			prints
			    ("注意：本篇文章的引言过长, 建议您删掉一些不必要的引言.\n");
			y += 3;
		}
		if (len < 40 || lines < 1) {
			move(y, 0);
			prints("注意：本篇文章过于简短, 系统认为是灌水文章.\n");
			y += 3;
		}
	}

	getdata(0, 0, pmt, abort, 3, DOECHO, YEA);

}

static int
write_file(struct edit *edit, char *filename, int saveheader, int modifyheader)
{
	FILE *fp = NULL;
	char abort[6];
	int retv = 0;
	int save_stat;
	char p_buf[100];
	signal(SIGALRM, SIG_IGN);
	clear();

	save_stat = local_article;
	//local_article: -1 不转信版面  0 转信版面默认不转信  1 转信
	if (uinfo.mode != CCUGOPHER) {
		if (uinfo.mode == POSTING) {
			if (local_article == 1)
				strcpy(p_buf,
				       "(L)不转信, (S)发表, (R)不可回复, (A)取消, (T)更改标题 or (E)再编辑? [L]: ");
			else {
				if (local_article == -1)
					strcpy(p_buf,
					       "(L)不转信, (R)不可回复, (A)取消, (T)更改标题 or (E)再编辑? [L]: ");
				else
					strcpy(p_buf,
					       "(S)发表, (L)不转信，(R)不可回复, (A)取消, (T)更改标题 or (E)再编辑? [S]: ");
			}
		} else if (uinfo.mode == SMAIL)
			strcpy(p_buf,
			       "(S)寄出, (A)取消, or (E)再编辑? [S]: ");
		else
			strcpy(p_buf,
			       "(S)储存档案, (A)放弃编辑, (E)继续编辑? [S]: ");
		valid_article(edit, p_buf, abort);

		if ((local_article == 0)
		    && ((abort[0] == 'l')
			|| (abort[0] == 'L')))
			local_article = 1;
		if ((local_article == 1)
		    && ((abort[0] == 's')
			|| (abort[0] == 'S')))
			local_article = 0;
		if (local_article == -1)
			local_article = 1;
		if (abort[0] == 0)
			abort[0] = 's';

	}
	if (abort[0] == 'a' || abort[0] == 'A') {
//		struct stat stbuf;
		clear();
		if (uinfo.mode != CCUGOPHER) {
			prints("取消...\n");
			refresh();
			sleep(1);
		}
//		if (stat(filename, &stbuf) || stbuf.st_size == 0)
//			unlink(filename);
		retv = REAL_ABORT;
	} else if (abort[0] == 'e' || abort[0] == 'E') {
		//msg();
		local_article = save_stat;
		return KEEP_EDITING;
	} else if (uinfo.mode == POSTING
		   && (abort[0] == 't' || abort[0] == 'T')) {
		char buf[STRLEN];
		move(1, 0);
		prints("旧标题: %s", save_title);
		sprintf(buf, "%s", save_title);
		getdata(2, 0, "新标题: ", buf, 50, DOECHO, NA);
		if (strcmp(save_title, buf) && strlen(buf) != 0) {
			strncpy(save_title, buf, STRLEN);
		}
	} else if ((abort[0] == 'r' || abort[0] == 'R') && uinfo.mode == POSTING) {
		retv = POST_NOREPLY;
	}

	if (retv != REAL_ABORT) {
		if ((fp = fopen(filename, "w")) == NULL) {
			indigestion(5);
			abort_bbs();
		}
		if (!strncmp(filename, "2nd/", 4)) {
			sprintf(genbuf, "%s(%s), 发布信息时ip: %s\n\n",
				currentuser->userid, currentuser->username,
				fromhost);
			fputs(genbuf, fp);
		} else if (saveheader)
			write_header(fp, 0);
		/* Added by deardragon 1999.11.13 禁止修改文章头部信息 */
		/* 如果是修改文章, 则在保存时, 恢复文章头信息. */
		/* Patched by lepton 2002.11.23 */
		else if (!modifyheader)
			keepoldheader(fp, RESTOREHEADER);
	}
	if ((edit) && (retv != REAL_ABORT )) {
			saveEdit(edit, fp);
	}
	if (fp)
		fclose(fp);
	return retv;
}

void
keep_fail_post()
{
	char filename[STRLEN];
	struct textLine *p;
	FILE *fp;
	if (!currentedit)
		return;
	p = currentedit->firstline;
	if (!p)
		return;

	sprintf(filename, "home/%c/%s/%s.deadve",
		mytoupper(currentuser->userid[0]), currentuser->userid,
		currentuser->userid);
	if ((fp = fopen(filename, "w")) == NULL) {
		indigestion(5);
		return;
	}
	while (p != NULL) {
		struct textLine *v = p;
		fwrite(p->text, 1, p->len, fp);
		if (p->br)
			fputc('\n', fp);
		p = p->next;
		free(v);
	}
	fclose(fp);
	return;
}

/*Function Add by SmallPig*/
static int
editOrigin(struct textLine *ln)
{
	char *str = ":．" MY_BBS_NAME " " MY_BBS_DOMAIN;
	char *str1 = ":．" MY_BBS_NAME " http://" MY_BBS_DOMAIN;
	if (uinfo.mode != EDIT)
		return 0;
	if (!ln)
		return 0;
	if (ln->text[0] != ':' && strnstr(ln->text, str, ln->len)
	    && strnstr(ln->text, "[FROM:", ln->len))
		return 1;
	if (ln->text[0] != ':' && strnstr(ln->text, str1, ln->len)
	    && strnstr(ln->text, " [FROM:", ln->len))
		return 1;
	return 0;
}

int
vedit(filename, saveheader, modifyheader)
char *filename;
int saveheader;
int modifyheader;
{
	int ans;
	ans = editor(filename, saveheader, modifyheader);
	return ans;
}
