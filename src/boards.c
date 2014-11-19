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

#define BBS_PAGESIZE    (t_lines - 4)

struct allbrc *allbrc = NULL;
struct onebrc brc = { 0, 0, 0, "" };
unsigned char board_sorttype;
char *sysconf_str();
struct newpostdata {
	char *name;
	char flag;
	short pos;
	char status;
	char unread:2;
	unsigned char zap:1;
	//int inboard;
} __attribute__ ((__packed__));

struct newpostdata nbrd[MAXBOARD];

int *zapbuf;
int zapbufchanged = 0;
int yank_flag = 0;
//unsigned char boardprefix[5];
int allboard = 0;
struct boardmem *getbcache(char *);

//���ư���Ĵ���, ȡ��fb2000.dhs.org.     --ecnegrevid
struct goodboard {
	char ID[GOOD_BRC_NUM][20];	//������࿴����17+1�ֽ�
	int num;
} GoodBrd;

static int inGoodBrds(char *bname);
static void load_GoodBrd(void);
static void save_GoodBrd(void);
static void load_zapbuf(void);
static void save_zapbuf(void);
static int zapped(int n, struct boardmem *bptr);
static int search_board(int *num, int brdnum, int secnum);
static int check_newpostt(struct newpostdata *ptr);
static void show_brdlist(int page, int clsflag, int newflag, int brdnum,
			 const struct sectree *sec);
static int cmpboard(struct newpostdata *brd, struct newpostdata *tmp);
static int choose_board(int newflag, const struct sectree *sec);
static void readwritebrc(struct allbrc *allbrc);
static int readtitle();
static char *readdoent(int num, struct fileheader *ent, char buf[512]);
static char *makedatestar(char *datestr, struct fileheader *ent);

void
GoodBrds()			// �˵��ĵ��ú���
{
//        if(!strcmp(currentuser->userid,"guest")) return;
	GoodBrd.num = 9999;
	allboard = 0;
	choose_board(1, NULL);
}

static int
inGoodBrds(char *bname)		// �жϰ����Ƿ��Ƕ��İ���
{
	int i;
	for (i = 0; i < GoodBrd.num && i < GOOD_BRC_NUM; i++)
		if (!strcasecmp(bname, GoodBrd.ID[i]))
			return i + 1;
	return 0;
}

static void
load_GoodBrd()			//���ļ��л�ȡ���İ��棬������ݽṹ GoodBrd
{
	char buf[STRLEN];
	FILE *fp;

	GoodBrd.num = 0;
	setuserfile(buf, ".goodbrd");
	if ((fp = fopen(buf, "r"))) {
		for (GoodBrd.num = 0; GoodBrd.num < GOOD_BRC_NUM;) {
			if (!fgets(buf, sizeof (buf), fp))
				break;
			strsncpy(GoodBrd.ID[GoodBrd.num], strtrim(buf),
				 sizeof (GoodBrd.ID[GoodBrd.num]));
			if (canberead(GoodBrd.ID[GoodBrd.num]))
				GoodBrd.num++;
		}
		fclose(fp);
	}
	if (GoodBrd.num == 0) {
		GoodBrd.num++;
		if (getbcache(DEFAULTBOARD))
			strcpy(GoodBrd.ID[0], DEFAULTBOARD);
		else
			strcpy(GoodBrd.ID[0], currboard);
	}
}
static void
save_GoodBrd()			// �����û����ĵİ���
{
	int i;
	FILE *fp;
	char fname[STRLEN];

	if (GoodBrd.num <= 0) {
		GoodBrd.num = 1;
		if (getbcache(DEFAULTBOARD))
			strcpy(GoodBrd.ID[0], DEFAULTBOARD);
		else
			strcpy(GoodBrd.ID[0], currboard);
	}
	setuserfile(fname, ".goodbrd");
	if ((fp = fopen(fname, "wb+")) != NULL) {
		for (i = 0; i < GoodBrd.num; i++)
			fprintf(fp, "%s\n", GoodBrd.ID[i]);
		fclose(fp);
	}
}

void
EGroup(cmd)
char *cmd;
{
	const struct sectree *sec;
	char buf[2];
	buf[0] = cmd[0];
	buf[1] = 0;
	sec = getsectree(buf);
	allboard = 0;
	choose_board(USERDEFINE(currentuser, DEF_NEWPOST) ? 1 : 0, sec);
}

void
Boards()
{
	allboard = 1;
	choose_board(0, &sectree);
}

void
New()
{
	allboard = 1;
	choose_board(1, &sectree);
}

static void
load_zapbuf()
{
	char fname[STRLEN];
	int fd, size;

	size = MAXBOARD * sizeof (int);
	zapbuf = (int *) malloc(size);
	bzero(zapbuf, size);
	setuserfile(fname, ".newlastread");
	if ((fd = open(fname, O_RDONLY, 0600)) != -1) {
		size = numboards * sizeof (int);
		read(fd, zapbuf, size);
		close(fd);
	}
	zapbufchanged = 0;
}

static void
save_zapbuf()
{
	char fname[STRLEN];
	int fd, size;

	if (!zapbufchanged)
		return;
	zapbufchanged = 0;

	setuserfile(fname, ".newlastread");
	if ((fd = open(fname, O_WRONLY | O_CREAT, 0600)) != -1) {
		size = numboards * sizeof (int);
		write(fd, zapbuf, size);
		close(fd);
	}
}

static int
zapped(int n, struct boardmem *bptr)
{
	if (zapbuf[n] == 0)	//û�� z 
		return 0;
	if (zapbuf[n] < bptr->header.board_ctime)	//z���ˣ����ǲ����������
		return 0;
	return 1;
}

static int
load_boards(int *brdnum, const struct sectree *sec, int *loadtime)
{
	struct boardmem *bptr;
	struct newpostdata *ptr;
	int n, addto = 0, goodbrd = 0;

	resolve_boards();
	if (GoodBrd.num != 9999 && bbsinfo.bcacheshm->uptime < *loadtime
	    && zapbuf != NULL && (*brdnum > 0
				  || (sec && sec->nsubsec > 0 && !allboard))
	    && (*brdnum >= 0))
		return 0;
	*loadtime = time(NULL);
	if (zapbuf == NULL) {
		load_zapbuf();
	}

	*brdnum = 0;
	if (!sec && !allboard)
		goodbrd = 1;	// ��ʾ�����Ķ����ư���״̬
	if (GoodBrd.num == 9999)	// ǿ�� load ���İ���
		load_GoodBrd();
	for (n = 0; n < numboards; n++) {
		bptr = &(bbsinfo.bcache[n]);
		if (!(bptr->header.filename[0]))
			continue;
		if (goodbrd == 0) {	//��������Ķ����Ƶİ���, ��...
			if (!allboard) {
				/*
				   if (strcmp(sec->basestr, bptr->header.sec1)
				   && (strcmp(sec->basestr, bptr->header.sec2)
				   || bptr->header.sec2[0] == 0))
				   continue; */
				if (getsectree(bptr->header.sec1) != sec
				    && (!bptr->header.sec2[0]
					|| getsectree(bptr->header.sec2) != sec)
				    && (sec != &sectree
					|| now_t - bptr->header.board_ctime >
					3600 * 24 * 7))
					continue;
			}
			if (!hasreadperm(&(bptr->header)))
				continue;
			addto = yank_flag || !zapped(n, bptr)
			    || (bptr->header.level & PERM_NOZAP);
		} else
			addto = inGoodBrds(bptr->header.filename);	//�����ж��Ƿ��Ƕ��ĵİ���
		if (addto) {	// addto ��־�ð���Ӧ�ÿ����Ķ�
			ptr = &nbrd[*brdnum];
			(*brdnum)++;
			ptr->name = bptr->header.filename;
			ptr->flag = bptr->header.flag |
			    ((bptr->header.
			      level & PERM_NOZAP) ? NOZAP_FLAG : 0);
			if (bptr->header.flag2 & NJUINN_FLAG)
				ptr->flag |= INNBBSD_FLAG;
			ptr->pos = n;
			ptr->unread = -1;	//����Ϊ -1 ��ʾδ��ʼ��
			if (zapped(n, bptr))
				ptr->zap = 1;
			else
				ptr->zap = 0;
			//ptr->inboard = bptr->inboard;
			if (bptr->header.level & PERM_POSTMASK)
				ptr->status = 'p';
			else if (bptr->header.level & PERM_NOZAP)
				ptr->status = 'z';
			else if ((bptr->header.level & ~PERM_POSTMASK) != 0)
				ptr->status = 'r';
			else
				ptr->status = ' ';
			if (ptr->status == ' ') {
				if (bptr->header.flag & CLUB_FLAG) {
					if (bptr->header.flag & CLOSECLUB_FLAG) {
						if (bptr->header.flag &
						    CLUBLEVEL_FLAG)
							ptr->status = 'C';
						else
							ptr->status = 'c';
					} else
						ptr->status = 'o';
				}
			}
		}
	}
	if (*brdnum == 0 && !yank_flag) {
		if (goodbrd) {	// ������ڶ��ư����У���û���κΰ���Ļ�����ˢ��
			GoodBrd.num = 0;
			save_GoodBrd();
			GoodBrd.num = 9999;
		} else {
			if (!sec->nsubsec) {
				char ans[3];
				getdata(t_lines - 1, 0,
					"����������İ����Ѿ�����ȫ��ȡ���ˣ��Ƿ�鿴������������(Y/N)[N]",
					ans, 2, DOECHO, YEA);
				if (toupper(ans[0]) == 'Y') {
					*brdnum = -1;
					yank_flag = 1;
					return -1;
				}
			}
		}
	}
	return 1;
}

static int
search_board(num, brdnum, secnum)
int *num, brdnum, secnum;
{
	static int i = 0, find = YEA;
	static char bname[STRLEN];
	int n, ch, tmpn = NA;

	if (find == YEA) {
		bzero(bname, sizeof (bname));
		find = NA;
		i = 0;
	}
	while (1) {
		move(t_lines - 1, 0);
		clrtoeol();
		prints("������Ҫ��Ѱ�� board ���ƣ�%s", bname);
		ch = egetch();

		if (isprint2(ch)) {
			bname[i++] = ch;
			for (n = secnum; n < brdnum + secnum; n++) {
				if (!strncasecmp
				    (nbrd[n - secnum].name, bname, i)) {
					tmpn = YEA;
					*num = n;
					if (!strcmp
					    (nbrd[n - secnum].name, bname))
						return 1;	/*�ҵ����Ƶİ棬�����ػ� */
				}
			}
			if (tmpn)
				return 1;
			if (find == NA) {
				bname[--i] = '\0';
			}
			continue;
		} else if (ch == Ctrl('H') || ch == KEY_LEFT
			   || ch == KEY_DEL || ch == '\177') {
			i--;
			if (i < 0) {
				find = YEA;
				break;
			} else {
				bname[i] = '\0';
				continue;
			}
		} else if (ch == '\t') {
			find = YEA;
			break;
		} else if (ch == '\n' || ch == '\r' || ch == KEY_RIGHT) {
			find = YEA;
			break;
		}
		bell();
	}
	if (find) {
		move(t_lines - 1, 0);
		clrtoeol();
		return 2 /*������ */ ;
	}
	return 1;
}

static int
check_newpostt(ptr)
struct newpostdata *ptr;
{
	struct boardmem *bptr;
	if (!allbrc) {
		allbrc = malloc(sizeof (struct allbrc));
		readwritebrc(allbrc);
	}

	bptr = &(bbsinfo.bcache[ptr->pos]);
	if (bptr->total <= 0)
		ptr->unread = 0;
	else {
		if (brc_unreadt_quick(allbrc, ptr->name, bptr->lastpost))
			ptr->unread = 1;
		else
			ptr->unread = 0;
	}
	return 0;
}

int
unread_position(dirfile, bptr)
char *dirfile;
struct boardmem *bptr;
{
	int fd, offset, step, num, filetime;
	//bptr = &bcache[ptr->pos];

	num = bptr->total + 1;
	if ((fd = open(dirfile, O_RDONLY)) > 0) {
		if (!brc_initial(bptr->header.filename, 1)) {
			num = 1;
		} else {
			offset = (int) &((struct fileheader *) 0)->filetime;
			num = bptr->total - 1;
			step = 4;
			while (num > 0) {
				lseek(fd,
				      offset + num * sizeof (struct fileheader),
				      SEEK_SET);
				if (read(fd, &filetime, sizeof (filetime)) <= 0
				    || !brc_unreadt(&brc, filetime))
					break;
				num -= step;
				if (step < 32)
					step += step / 2;
			}
			if (num < 0)
				num = 0;
			while (num < bptr->total) {
				lseek(fd,
				      offset + num * sizeof (struct fileheader),
				      SEEK_SET);
				if (read(fd, &filetime, sizeof (filetime)) <= 0
				    || brc_unreadt(&brc, filetime))
					break;
				num++;
			}
		}
		close(fd);
	}
	if (num < 0)
		num = 0;
	return num;
}

static void
show_brdlist(page, clsflag, newflag, brdnum, sec)
int page, clsflag, newflag, brdnum;
const struct sectree *sec;
{
	struct newpostdata *ptr;
	struct boardmem *bptr;
	int n;
	char tmpBM[IDLEN + 1], buf[STRLEN];
	char title[20];
	int secnum;

	switch (board_sorttype) {
	case 0x00:
		strcpy(title, "[�������б�] [����]");
		break;
	case 0x10:
		strcpy(title, "[�������б�] [����]");
		break;
	case 0x20:
		strcpy(title, "[�������б�] [��ĸ]");
		break;
	case 0x30:
		strcpy(title, "[�������б�] [����]");
		break;
	default:
		strcpy(title, "[�������б�]");
		break;
	}

	if (clsflag) {
		clear();
		docmdtitle(title,
			   "  \033[m��ѡ��[\033[1;32m��\033[m,\033[1;32me\033[m] �Ķ�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] �г�[\033[1;32my\033[m] ����[\033[1;32ms\033[m] ��Ѱ[\033[1;32m/\033[m] �л�[\033[1;32mc\033[m] ����[\033[1;32mh\033[m]\n");
		prints
		    ("\033[1;44;37m %s ����������   V ���   %-21sS ����Ա      ����   ����\033[m\n",
		     newflag ? " ȫ�� δ" : " ��� δ", "��  ��  ��  ��");
	}

	if (sec && !allboard)
		secnum = sec->nsubsec;
	else
		secnum = 0;
	move(3, 0);
	clrtobot();
	for (n = page; n < page + BBS_PAGESIZE; n++) {
		if (n >= brdnum + secnum)
			break;
		if (n < secnum) {
			int i;
			struct secmanager *secm =
			    getsecm(sec->subsec[n]->basestr);
			if (!newflag)
				prints(" %4d  ", n + 1);
			else
				prints(" %4s  ", sec->subsec[n]->basestr);
			prints("�� ");
			prints("%-20s %-22s ", sec->subsec[n]->title,
			       sec->subsec[n]->des);
			if (secm)
				for (i = 0; i < secm->n && i < 3; i++)
					prints(" %s", secm->secm[i]);
			prints("\n");
		} else {
			ptr = &nbrd[n - secnum];
			bptr = &(bbsinfo.bcache[ptr->pos]);
			if (ptr->unread == -1)
				check_newpostt(ptr);
			if (!newflag)
				prints(" %4d  ", n + 1);
			else
				prints(" %5d ", bptr->total);
			prints("%s%c",
			       (ptr->flag & INNBBSD_FLAG) ? (ptr->unread ?
							     "\033[1;32m��\033[m"
							     :
							     "\033[1;32m��\033[m")
			       : (ptr->unread ? "��" : "��"), (ptr->zap
							       && !(ptr->flag &
								    NOZAP_FLAG))
			       ? '-' : ' ');
			strncpy(tmpBM, bptr->header.bm[0], IDLEN);
			sprintf(buf, "[%s] %s", bptr->header.type,
				bptr->header.title);
			if (ptr->status == 'p')
				memcpy(buf, "[ֻ��]", 6);
			prints("%-13s%s%-28s %c %-12s%4d %6d\n", ptr->name, (ptr->flag & VOTE_FLAG) ? "\033[1;31mV\033[m" : " ", buf, ptr->status, (tmpBM[0] == '\0' ? "����������" : tmpBM), bptr->inboard, bptr->score);	//USERPERM(currentuser, PERM_POST)?ptr->status:' ',
		}
	}
}
static int
cmpboard(brd, tmp)
struct newpostdata *brd, *tmp;
{
	int type = 0;
	struct boardmem *bptrbrd, *bptrtmp;
	bptrbrd = &(bbsinfo.bcache[brd->pos]);
	bptrtmp = &(bbsinfo.bcache[tmp->pos]);

	switch (board_sorttype) {
	case 0x00:
		type = bptrbrd->header.sec1[0] - bptrtmp->header.sec1[0];
/*		if (type == 0)
			type =
			    strncasecmp(bptrbrd->header.type,
					bptrtmp->header.type, 4);
		if (type == 0)
			type = strcasecmp(brd->name, tmp->name);
			*/
		break;
	case 0x10:
		type = bptrtmp->score - bptrbrd->score;
		break;
	case 0x20:
		type = strcasecmp(brd->name, tmp->name);
		break;
	case 0x30:
		type = bptrtmp->inboard - bptrbrd->inboard;
		break;
	default:
		break;
	}
	return type;
}

void
update_postboards(void)
{
	int i, begin = 0;
	char buf[64], *bname;
	FILE *fp, *fw;
	sprintf(buf, "tmp/postb.pl.%d", uinfo.pid);
	fw = fopen(buf, "w");
	if (NULL == fw) {
		errlog("can't open postb.pl to write!");
		return;
	}
	fputs("#!/usr/bin/perl\n@board=(\n", fw);
	for (i = 0; i < bbsinfo.bcacheshm->number; i++) {
		bname = &(bbsinfo.bcache[i].header.filename[0]);
		if (!bname[0])
			continue;
		snprintf(buf, 64, "boards/%s/.POSTBOARDS", bname);
		if (valid_fname(bname)) {
			if (begin)
				fputs(",\n", fw);
			else
				begin = 1;
			fprintf(fw, "\" %s", bname);
		} else
			continue;
		fp = fopen(buf, "r");
		if (NULL == fp)
			goto end;

		while (fgets(buf, sizeof (buf), fp)) {
			if ('\n' == buf[strlen(buf) - 1])
				buf[strlen(buf) - 1] = 0;
			if (valid_fname(buf))
				fprintf(fw, " %s", buf);
		}
		fclose(fp);
	      end:
		fputs(" \"", fw);
	}
	fputs("\n);\n", fw);
	fclose(fw);
	sprintf(buf, "tmp/postb.pl.%d", uinfo.pid);
	rename(buf, "etc/postb.pm");
	return;
}

static int
choose_board(newflag, sec)
int newflag;
const struct sectree *sec;
{
	int num = 0;
	struct newpostdata *ptr;
	int page = -1, ch = 0, tmp, number = -1, tmpnum;
	int loop_mode = 0, retv;
	int brdnum, secnum;
	struct boardmem *bptr;
	int loadtime = 0;

	if (sec && !allboard)
		secnum = sec->nsubsec;
	else
		secnum = 0;
	if (!strcmp(currentuser->userid, "guest"))
		yank_flag = 1;
	modify_user_mode(newflag ? READNEW : READBRD);
	brdnum = 0;
	while (1) {
		retv = load_boards(&brdnum, sec, &loadtime);
		if (retv < 0)
			retv = load_boards(&brdnum, sec, &loadtime);
		if (brdnum + secnum <= 0)
			break;
		if (retv) {
			qsort(nbrd, brdnum, sizeof (nbrd[0]),
			      (void *) cmpboard);
			page = -1;
		}
		if (num < 0)
			num = 0;
		if (num >= brdnum + secnum)
			num = brdnum + secnum - 1;
		if (page < 0) {
			if (newflag && (num >= secnum)) {
				for (tmp = 0; tmp < brdnum; tmp++)
					check_newpostt(&nbrd[tmp]);
				tmp = num;
				while (num < brdnum + secnum) {
					ptr = &nbrd[num - secnum];
					if (ptr->unread == -1)
						check_newpostt(ptr);
					if (ptr->unread)
						break;
					num++;
				}
				if (num >= brdnum + secnum) {
					num = tmp;
				}
			}
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			show_brdlist(page, 1, newflag, brdnum, sec);
			update_endline();
		}
		if (num < page || num >= page + BBS_PAGESIZE) {
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			show_brdlist(page, 0, newflag, brdnum, sec);
			update_endline();
		}
		//since allbrc is large, we should not waste too much memory on it, 
		//free it as soon as possible
		if (allbrc) {
			free(allbrc);
			allbrc = NULL;
		}
		move(3 + num - page, 0);
		prints(">");
		move(3 + num - page, 0);
		if (loop_mode == 0) {
			can_R_endline = 1;
			ch = egetch();
			can_R_endline = 0;
		}
		move(3 + num - page, 0);
		prints(" ");
		if (ch == 'q' || ch == 'e' || ch == KEY_LEFT || ch == EOF)
			break;
		switch (ch) {
		case 'P':
		case 'b':
		case Ctrl('B'):
		case KEY_PGUP:
			if (num == 0)
				num = brdnum + secnum - 1;
			else
				num -= BBS_PAGESIZE;
			break;
		case 'F':
		case 'f':
			if (newflag == 1)
				newflag = 0;
			else
				newflag = 1;
			show_brdlist(page, 1, newflag, brdnum, sec);
			update_endline();
			break;
		case 'L':	/* ppfoong */
			show_allmsgs();
			page = -1;
			break;
		case 'N':
		case ' ':
		case Ctrl('F'):
		case KEY_PGDN:
			if (num == brdnum + secnum - 1)
				num = 0;
			else
				num += BBS_PAGESIZE;
			break;
		case 'p':
		case 'k':
		case KEY_UP:
			if (num-- <= 0)
				num = brdnum + secnum - 1;
			break;
		case 'n':
		case 'j':
		case KEY_DOWN:
			if (++num >= brdnum + secnum)
				num = 0;
			break;
		case '$':
			num = brdnum + secnum - 1;
			break;
		case '!':	/* youzi leave */
			return Q_Goodbye();
		case 'w':
			if ((in_mail != YEA)
			    && USERPERM(currentuser, PERM_READMAIL))
				m_read();
			page = -1;
			break;
		case 'h':
			show_help("help/boardreadhelp");
			page = -1;
			break;
		case '/':
			move(3 + num - page, 0);
			prints(">");
			tmpnum = num;
			tmp = search_board(&num, brdnum, secnum);
			move(3 + tmpnum - page, 0);
			prints(" ");
			if (tmp == 1)
				loop_mode = 1;
			else {
				loop_mode = 0;
				update_endline();
			}
			break;
		case 's':	/* sort/unsort -mfchen */
			board_sorttype += 0x10;
			board_sorttype = board_sorttype % 0x40;
			qsort(nbrd, brdnum, sizeof (nbrd[0]),
			      (void *) cmpboard);
			page = -1;
			break;
		case 'y':
			if (sec) {
				yank_flag = !yank_flag;
				brdnum = -1;
			}
			break;
		case 'z':
			if (num >= secnum + brdnum || num < secnum)
				break;
			if (USERPERM(currentuser, PERM_BASIC)
			    && !(nbrd[num - secnum].flag & NOZAP_FLAG)) {
				ptr = &nbrd[num - secnum];
				ptr->zap = !ptr->zap;
				ptr->unread = -1;
				zapbuf[ptr->pos] = (ptr->zap ? now_t : 0);
				zapbufchanged = 1;
				page = 999;
			}
			break;
		case 'C':
			if (USERPERM(currentuser, PERM_SPECIAL2)
			    || clubtest("deleterequest")) {
				do1984menu();
				page = -1;
			}
			break;
		case KEY_HOME:
			num = 0;
			break;
		case KEY_END:
			num = brdnum + secnum - 1;
			break;
		case '\n':
		case '\r':
			if (number > 0) {
				num = number - 1;
				break;
			} else if (number == 0) {
				num = brdnum + secnum - 1;
				break;
			}
			/* fall through */
		case KEY_RIGHT:
			if (num >= secnum) {
				char tmp_b[80];
				ptr = &nbrd[num - secnum];
				brc_initial(ptr->name, 1);
				strsncpy(tmp_b, currboard, 80);
				strcpy(currboard, ptr->name);
				if (USERDEFINE(currentuser, DEF_FIRSTNEW)) {
					char buf[STRLEN];
					setbdir(buf, currboard, digestmode);
					if (getkeep(buf, -1, 0) == NULL) {
						tmp =
						    unread_position(buf,
								    &(bbsinfo.
								      bcache
								      [ptr->
								       pos]));
						page = tmp - t_lines / 2;
						getkeep(buf,
							page > 1 ? page : 1,
							tmp + 1);
					}
				}
				selboard = 1;
				if (Read() == 1)
					strcpy(currboard, tmp_b);
				ptr->unread = page = -1;
				modify_user_mode(newflag ? READNEW : READBRD);
			} else {
				if (sec) {
					choose_board(newflag, sec->subsec[num]);
					page = -1;
					brdnum = -1;
				}
			}
			break;
		case KEY_TAB:
			if (USERDEFINE(currentuser, DEF_FILTERXXX))
				ansimore("etc/dayf", NA);
			else
				ansimore("0Announce/bbslist/day", NA);
			what_to_do();
			page = -1;
			break;
		case 'u':
			clear();
			prints("��ѯ����״̬");
			t_query(NULL);
			page = -1;
			break;
		case 'H':
			if (USERDEFINE(currentuser, DEF_FILTERXXX))
				ansimore("etc/dayf", NA);
			else
				ansimore("0Announce/bbslist/day", NA);
			what_to_do();
			page = -1;
			break;
		case 'S':	/* sendmsg ... youzi */
			if (!USERPERM(currentuser, PERM_PAGE))
				break;
			s_msg();
			page = -1;
			break;
		case 'c':	/* show friends ... youzi */
			if (!USERPERM(currentuser, PERM_BASIC))
				break;
			t_friends();
			modify_user_mode(newflag ? READNEW : READBRD);
			page = -1;
			break;
		case 'E':
			if (num < secnum || num >= secnum + brdnum)
				break;
			editboard(nbrd[num - secnum].name);
			page = -1;
			break;
		case Ctrl('D'):
			if (num >= secnum + brdnum || num < secnum)
				break;
			ptr = &nbrd[num - secnum];
			if (ptr->status == 'r')
				break;

			bptr = getbcache(ptr->name);
			if (!bptr)
				break;
			if (!chk_editboardperm(&(bptr->header))
			    && !clubtest("deleterequest"))
				break;
			if (ptr->status == 'p') {
				if (!USERPERM(currentuser, PERM_BLEVELS))
					break;
			}
			page = -1;
			sprintf(genbuf, "ȷ��Ҫ%s%s����?",
				(ptr->status == 'p') ? "���" : "��",
				ptr->name);
			if (askyn(genbuf, NA, YEA) == NA)
				break;

			{
				struct boardheader fh;
				int pos;
				if (!
				    (pos =
				     new_search_record(BOARDS, &fh,
						       sizeof (struct
							       boardheader),
						       (void *) cmpbnames,
						       ptr->name))) {
					prints("���������������");
					pressreturn();
					break;
				}
				if (fh.level & ~(PERM_BLEVELS | PERM_POSTMASK)) {
					prints
					    ("��������ֱ�ӷ���Ȩ��, �޷�����, �����޸İ������ù���");
					pressreturn();
					break;
				}
				if (ptr->status == 'p') {
					fh.level &=
					    ~(PERM_POSTMASK | PERM_BLEVELS);
					ptr->status = ' ';
				} else {
					fh.level |=
					    (PERM_POSTMASK | PERM_BLEVELS);
					ptr->status = 'p';
				}
				substitute_record(BOARDS, &fh,
						  sizeof (struct boardheader),
						  pos);
				reload_boards();
				sprintf(genbuf, "%s������: %s",
					(ptr->status == 'p') ? "��" : "���",
					ptr->name);
				securityreport(genbuf, genbuf);
				prints("�Ѿ�%s��������: %s",
				       (ptr->status == 'p') ? "��" : "���",
				       ptr->name);
			}
			pressreturn();
			break;
		case 'B':
			if (num >= brdnum + secnum || num < secnum)
				break;
			ptr = &nbrd[num - secnum];
			if (!USERPERM(currentuser, PERM_BLEVELS))
				break;
			page = -1;
			sprintf(genbuf, "ȷ��Ҫ�༭ %s ����ת�Ŷ�Ӧ�б���?",
				ptr->name);
			if (askyn(genbuf, NA, YEA) == NA)
				break;
			snprintf(genbuf, 64, "boards/%s/.POSTBOARDS",
				 ptr->name);
			if (vedit(genbuf, 0, YEA) == -1)
				break;
			update_postboards();
			pressreturn();
			break;

		case 'a':
			if (num >= brdnum + secnum || num < secnum)
				break;
			if (!sec) {
				if (GoodBrd.num >= GOOD_BRC_NUM) {
					move(t_lines - 1, 0);
					prints("�������Ű����Ѿ�������(%d)",
					       GOOD_BRC_NUM);
					//pressreturn();
				} else {
					char bname[STRLEN], bpath[STRLEN];
					struct stat st;
					move(0, 0);
					clrtoeol();
					prints
					    ("ѡ��һ�������� (Ӣ����ĸ��Сд�Կ�)\n");
					prints
					    ("������������ (���հ׼��Զ���Ѱ): ");
					clrtoeol();
					make_blist();
					namecomplete((char *) NULL, bname);
					FreeNameList();
					setbpath(bpath, bname);
					if ((*bname == '\0')
					    || (stat(bpath, &st) == -1)) {
						move(2, 0);
						prints("����ȷ��������.\n");
						pressreturn();
					} else {
						if (!inGoodBrds(bname)) {
							strcpy(GoodBrd.
							       ID[GoodBrd.
								  num++],
							       bname);
							save_GoodBrd();
							GoodBrd.num = 9999;
							brdnum = -1;
							break;
						}
					}
					page = -1;
				}
			} else {
				load_GoodBrd();
				ptr = &nbrd[num - secnum];
				if (GoodBrd.num >= GOOD_BRC_NUM) {
					move(t_lines - 1, 0);
					clrtoeol();
					prints("�������Ű����Ѿ�������(%d)",
					       GOOD_BRC_NUM);
					GoodBrd.num = 0;
					//pressreturn();
				} else {
					if (!inGoodBrds(ptr->name)) {
						strcpy(GoodBrd.ID
						       [GoodBrd.num++],
						       ptr->name);
						save_GoodBrd();
						GoodBrd.num = 0;
						move(t_lines - 1, 0);
						clrtoeol();
						prints("����%s�ѱ������ղؼ�",
						       ptr->name);
						break;
					}
					GoodBrd.num = 0;
				}
			}
			break;
		case 'd':
			if (num >= brdnum + secnum || num < secnum)
				break;
			if (!sec) {
				int i, pos;
				char ans[5];
				sprintf(genbuf,
					"Ҫ�� %s ���ղؼ���ȥ�� (Y/N)? [N]",
					nbrd[num - secnum].name);
				getdata(t_lines - 1, 0, genbuf, ans, 2,
					DOECHO, YEA);
				if (ans[0] != 'y' && ans[0] != 'Y') {
					page = -1;
					break;
				}
				pos = inGoodBrds(nbrd[num - secnum].name);
				for (i = pos - 1; i < GoodBrd.num - 1; i++)
					strcpy(GoodBrd.ID[i],
					       GoodBrd.ID[i + 1]);
				GoodBrd.num--;
				save_GoodBrd();
				GoodBrd.num = 9999;
				brdnum = -1;
			}
			break;
		default:
			;
		}
		if (inputgotonumber(ch, &number)) {
			printgotonumber(number);
		} else if (number >= 0) {
			number = -1;
			update_endline();
		}
	}
	if (allbrc) {
		free(allbrc);
		allbrc = NULL;
	}
	clear();
	save_zapbuf();
	return -1;
}

static void
readwritebrc(struct allbrc *allbrc)
{
	char dirfile[STRLEN];
	if (strcmp(currentuser->userid, "guest")) {
		setuserfile(dirfile, "brc");
		brc_init(allbrc, currentuser->userid, dirfile);
		brc_putboard(allbrc, &brc);
		brc_fini(allbrc, currentuser->userid);
	} else {
		sprintf(dirfile, "%s.%s", "guest", uinfo.from);
		brc_init(allbrc, dirfile, NULL);
		brc_putboard(allbrc, &brc);
		brc_fini(allbrc, dirfile);
	}

}

void
brc_update()
{
	if (!brc.changed)
		return;
	if (allbrc) {
		readwritebrc(allbrc);
		return;
	}
	allbrc = malloc(sizeof (struct allbrc));
	readwritebrc(allbrc);
	free(allbrc);
	allbrc = NULL;

}

int
brc_initial(char *boardname, int keep)
{
	if (!strncmp(brc.board, boardname, BRC_STRLEN - 1)) {
		if (!keep && allbrc) {
			free(allbrc);
			allbrc = NULL;
		}
		return brc.num;
	}
	if (!allbrc) {
		allbrc = malloc(sizeof (struct allbrc));
		readwritebrc(allbrc);
	} else if (brc.changed)
		readwritebrc(allbrc);
	brc_getboard(allbrc, &brc, boardname);
	if (!keep) {
		free(allbrc);
		allbrc = NULL;
	}
	return brc.num;
}

void
clear_new_flag_quick(int t)
{
	int bnum;
	if (!t) {
		t = time(NULL);
		bnum = getbnum(brc.board);
		if (bnum && bbsinfo.bcache[bnum - 1].lastpost > t)
			t = bbsinfo.bcache[bnum - 1].lastpost;
	}
	brc_clearto(&brc, t);
}

void
clear_all_new_flag()
{
	int i;
	char ans[3];
	int brdnum;
	int loadtime = 0;
	GoodBrd.num = 0;
	allboard = 1;
	brdnum = -1;
	getdata(t_lines - 2, 0,
		"ȷ��Ҫ������а����δ����ǣ�(Y/N) [N]:", ans, 2,
		DOECHO, YEA);
	if (ans[0] != 'y' && ans[0] != 'Y')
		return;
	if (load_boards(&brdnum, 0, &loadtime) < 0)
		return;
	for (i = 0; i < brdnum; i++) {
		brc_initial(nbrd[i].name, (i == brdnum - 1) ? 0 : 1);
		clear_new_flag_quick(0);
	}
	brc_update();
}

int
Read()
{
	char buf[STRLEN];
	char notename[STRLEN];
	struct stat st;
	int bnum;
	//struct userec tmpu;
	if (!selboard || !strcmp(currboard, "")) {
		move(2, 0);
		prints("����ѡ��������\n");
		pressreturn();
		move(2, 0);
		clrtoeol();
		return -1;
	}
	modify_user_mode(READING);
	brc_initial(currboard, 0);
	setbdir(buf, currboard, digestmode);
	bnum = getbnum(currboard);
	if (!clubsync(bnum))
		return 1;
	if (bbsinfo.bcache[bnum - 1].header.flag & CLOSECLUB_FLAG) {	//c��C���ֲ�
		if (bbsinfo.bcache[bnum - 1].header.flag & CLUBLEVEL_FLAG) {	//C���ֲ�
			if (!USERPERM(currentuser, PERM_SYSOP)) {	//�������վ��������ֲ�Ȩ��
				if (!clubtest(currboard))
					return 1;
			} else {
				if (strcmp(currentuser->userid, "SYSOP"))	//�����վ�������Ƿ��ں�����
					if (!clubtest(currboard)
					    && clubtestdenysysop(currboard))
						return 1;
			}
		} else {	//c���ֲ�
			if (!clubtest(currboard))
				return 1;
		}
	}
	if (uinfo.curboard && bbsinfo.bcache[uinfo.curboard - 1].inboard > 0)
		bbsinfo.bcache[uinfo.curboard - 1].inboard--;
	uinfo.curboard = bnum;
	update_utmp();
	bbsinfo.bcache[uinfo.curboard - 1].inboard++;
	setvfile(notename, currboard, "notes");
	clear();
	if (stat(notename, &st) != -1) {
		if (st.st_mtime > brc.notetime
		    || now_t - brc.notetime > 7 * 86400) {
			show_board_notes(currboard);
			brc.notetime = now_t;
			brc.changed = 1;
			if (!USERDEFINE(currentuser, DEF_INTOANN)
			    || brc.num > 1)
				pressanykey();
		}
	}
#if 0
	if (USERDEFINE(currentuser, DEF_INTOANN)
	    && brc_unreadt(&brc, 2)) {
		char ans[3];
		getdata(t_lines - 1, 0,
			"\033[0m\033[1m�����η��ʱ���, �Ƿ����Ȳ쿴������?"
			" (ѡA����������ʾ)(Y/N/A) [Y]:", ans, 2, DOECHO, YEA);
		brc_addlistt(&brc, 2);
		if (ans[0] == 'A' || ans[0] == 'a') {
			memcpy(&tmpu, currentuser, sizeof (struct userec));
			tmpu.userdefine &= ~DEF_INTOANN;
			updateuserec(&tmpu, usernum);
		} else if (ans[0] != 'N' && ans[0] != 'n') {
			into_announce();
			show_board_notes(currboard);
			move(t_lines - 1, 0);
			prints
			    ("\033[0m\033[1m��ӭ����, ����������뱾�����, �ڰ��水'x'������ʱ���뾫����");
			egetch();
		}
	}
#endif
	currboardstarttime = now_t;
	i_read(READING, buf, readtitle,
	       (void *) readdoent, read_comms, sizeof (struct fileheader));
	loguseboard();
	brc_update();
	if (uinfo.curboard && bbsinfo.bcache[uinfo.curboard - 1].inboard > 0)
		bbsinfo.bcache[uinfo.curboard - 1].inboard--;
	uinfo.curboard = 0;
	update_utmp();
	return 0;
}

void
loguseboard()
{
	if (now_t - currboardstarttime <= 4)
		return;
	tracelog("%s use %s %ld", currentuser->userid, currboard,
		 (long int) min((int) (now_t - currboardstarttime), 3600 * 8));
	currboardstarttime = now_t;
}

int
showhell()
{
	selboard = 1;
	strcpy(currboard, "hell");
	return Read();
}

int
showprison()
{
	selboard = 1;
	strcpy(currboard, "prison");
	return Read();
}

static int
readtitle()
{
	struct boardmem *bp;

	char header[200], title[STRLEN];
	char readmode[10];
	int active, invisible, i, bnum;
	char tmp[40];
	bp = getbcache(currboard);
	if (bp == NULL)
		return -1;
	IScurrBM = chk_currBM(&(bp->header), 0);

	bnum = 0;
	if (bp->header.bm[0][0] == 0) {
		strcpy(header, "����������");
	} else {
		strcpy(header, "����: ");
		for (i = 0; i < 4; i++) {	//ֻ��ʾǰ�ĸ���೤
			if (bp->header.bm[i][0] == 0)
				break;
			active = bp->bmonline & (1 << i);
			invisible = bp->bmcloak & (1 << i);
			if (active && !invisible)
				sprintf(tmp, "\033[32m%s\033[33m ",
					bp->header.bm[i]);
			else if (active && invisible
				 && (USERPERM(currentuser, PERM_SEECLOAK)
				     || !strcmp(bp->header.bm[i],
						currentuser->userid)))
				sprintf(tmp, "\033[36m%s\033[33m ",
					bp->header.bm[i]);
			else
				sprintf(tmp, "%s ", bp->header.bm[i]);
			strcat(header, tmp);
		}
	}
	if (chkmail())
		strcpy(title, "[�����ż�,�밴 w �鿴�ż�]");
	else if (!strcmp(currboard, "deleteobserver")
		 && bbsinfo.utmpshm->watchman
		 && now_t > bbsinfo.utmpshm->watchman - 300) {
		if (now_t > bbsinfo.utmpshm->watchman)
			strcpy(title, "�� �Ѿ������ˣ���îඡ� ��");
		else
			strcpy(title, "�� ��Ҫ�����ˣ���îඡ� ��");
	} else if ((bp->header.flag & VOTE_FLAG))
		sprintf(title, "�� ͶƱ�У��� v ����ͶƱ ��");
	else
		strcpy(title, bp->header.title);

	showtitle(header, title);
	prints
	    ("�뿪[\033[1;32m��\033[m,\033[1;32mq\033[m] ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m] �Ķ�[\033[1;32m��\033[m,\033[1;32mRtn\033[m] ��������[\033[1;32mCtrl-P\033[m] ɾ��[\033[1;32md\033[m] ����¼[\033[1;32mTAB\033[m] ����[\033[1;32mh\033[m]\n");
	switch (digestmode) {
	case 0:
		if (USERDEFINE(currentuser, DEF_THESIS))	/* youzi 1997.7.8 */
			strcpy(readmode, "����");
		else
			strcpy(readmode, "һ��");
		break;
	case 1:
		strcpy(readmode, "��ժ");
		break;
	case 2:
		strcpy(readmode, "����");
		break;
	case 3:
		strcpy(readmode, "��ˮ");
		break;
	case 4:
		strcpy(readmode, "����");
		break;
	case 5:
		strcpy(readmode, "ֽ¨");
		break;
	}
	if (USERDEFINE(currentuser, DEF_THESIS) && digestmode == 0)
		prints
		    ("\033[1;37;44m ���   %-12s %6s %-28s����:%4d [%4sʽ����] \033[m\n",
		     "�� �� ��", "��  ��", " ��  ��", bp->inboard, readmode);
	else
		prints
		    ("\033[1;37;44m ���   %-12s %6s %-30s����:%4d [%4sģʽ] \033[m\n",
		     "�� �� ��", "��  ��", " ��  ��", bp->inboard, readmode);
	if (bp->ban) {
		char buf[80];
		move(2, 40);
		snprintf(buf, sizeof (buf),
			 "\033[1;44;%dm������%s����\033[m\n",
			 33 - bp->ban, bp->ban == 1 ? "��Ҫ" : "�Ѿ�");
		prints("%s", buf);
	}

	clrtobot();
	return 0;
}

static char *
readdoent(num, ent, buf)
int num;
struct fileheader *ent;
char buf[512];
{
	char date[80], owner[13];
	char *TITLE;
	char type, *type1, type2, danger = 0;
	char typestring[32];
	int noreply = 0, attached = 0, allcanre = 0;

	{
		char *ptr;
		strsncpy(owner, ent->owner, sizeof (owner));
		if (!owner[0])
			strcpy(owner, "Anonymous");
		if ((ptr = strchr(owner, '.')) != NULL)
			*(ptr + 1) = 0;
	}

	type = (UNREAD(ent, &brc) ? '*' : ' ');
	if ((ent->accessed & FH_DIGEST)) {
		if (type == ' ')
			type = 'g';
		else
			type = 'G';
	}
	if ((ent->accessed & FH_DANGEROUS) && (IScurrBM || ISdelrq))
		danger = 1;
	if ((ent->accessed & FH_DEL) && IScurrBM) {
		if (danger)
			type1 = "\033[1;31mX\033[0m";
		else
			type1 = "X";
	} else {
		if (danger)
			type1 = "\033[1;31m!\033[0m";
		else
			type1 = " ";
	}
	type2 = (ent->accessed & FH_SPEC) ? (IScurrBM ? '$' : ' ') : ' ';
	noreply = ent->accessed & FH_NOREPLY;
	attached = ent->accessed & FH_ATTACHED;
	allcanre = ent->accessed & FH_ALLREPLY;

	if (ent->accessed & FH_MARKED) {
		switch (type) {
		case ' ':
			type = 'm';
			break;
		case '*':
			type = 'M';
			break;
		case 'g':
			type = 'b';
			break;
		case 'G':
			type = 'B';
			break;
		}
	}
	if (IScurrBM && (ent->accessed & FH_ANNOUNCE)) {
		sprintf(typestring, "%s\033[1;32;42m%c\033[m%c", type1,
			type, type2);
	} else
		sprintf(typestring, "%s%c%c", type1, type, type2);

	makedatestar(date, ent);
	/*  Re-Write By Excellent */

	TITLE = ent->title;
	filteransi(TITLE);
	if ((ent->accessed & FH_ISTOP)) {
		if (ent->thread != ent->filetime && !strncmp(TITLE, "Re: ", 4))
			sprintf(buf,
				" \033[1;33m[��ʾ]\033[m %-12.12s%s %cRe: %-.45s\033[m",
				owner, date, attached ? '@' : ' ', TITLE + 3);
		else
			sprintf(buf,
				" \033[1;33m[��ʾ]\033[m %-12.12s%s %c�� %-.45s\033[m",
				owner, date, attached ? '@' : ' ', TITLE);
	} else if (uinfo.mode != RMAIL && digestmode != 1 && digestmode != 4 && digestmode != 5) {	//���·���
		if (ent->thread != ent->filetime && !strncmp(TITLE, "Re: ", 4)) {	//Re��
			if (readingthread == ent->thread)	//��ǰ���ڶ�������
				sprintf(buf,
					" \033[1;36m%4d\033[m%s%-12.12s%s\033[1;36m.%c%sRe:\033[0;1;36m%-.45s\033[m",
					num, typestring, owner, date,
					attached ? '@' : ' ',
					noreply ? "\033[0;1;4;33m" : "",
					TITLE + 3);
			else
				sprintf(buf,
					" %4d%s%-12.12s%s %c%sRe:\033[m%-.45s\033[m",
					num, typestring, owner, date,
					attached ? '@' : ' ',
					noreply ? "\033[0;1;4;33m" : "",
					TITLE + 3);
		} else {
			if (readingthread == ent->thread)
				sprintf(buf,
					" \033[1;33m%4d\033[m%s%-12.12s%s\033[1;33m.%c%s��\033[0;1;33m %-.45s\033[m",
					num, typestring, owner, date,
					attached ? '@' : ' ',
					noreply ? "\033[0;1;4;33m" : "", TITLE);
			else
				sprintf(buf,
					" %4d%s%-12.12s%s %c%s%s %-.45s\033[m",
					num, typestring, owner, date,
					attached ? '@' : ' ',
					noreply ? "\033[0;1;4;33m" : "",
					allcanre ? "\033[1;31m��" :
					"��\033[m", TITLE);
		}
	} else {
		if (!strncmp("Re:", ent->title, 3)
		    || !strncmp("RE:", ent->title, 3)) {
			if (strncmp(ReplyPost, ent->title, 45) == 0) {
				sprintf(buf,
					" \033[1;36m%4d\033[m%s%-12.12s%s\033[1;36m.%c%sRe:\033[0;1;36m%-.45s\033[m",
					num, typestring, owner, date,
					attached ? '@' : ' ',
					noreply ? "\033[0;1;4;33m" : "",
					TITLE + 3);
			} else {
				sprintf(buf,
					" %4d%s%-12.12s%s %c%sRe:\033[m%-.45s\033[m",
					num, typestring, owner, date,
					attached ? '@' : ' ',
					noreply ? "\033[0;1;4;33m" : "",
					TITLE + 3);
			}
		} else {
			if (strncmp(ReadPost, ent->title, 45) == 0) {
				sprintf(buf,
					" \033[1;33m%4d\033[m%s%-12.12s%s\033[1;33m.%c%s��\033[0;1;33m %-.45s\033[m",
					num, typestring, owner, date,
					attached ? '@' : ' ',
					noreply ? "\033[0;1;4;33m" : "", TITLE);
			} else {
				sprintf(buf,
					" %4d%s%-12.12s%s %c%s%s %-.45s\033[m",
					num, typestring, owner, date,
					attached ? '@' : ' ',
					noreply ? "\033[0;1;4;33m" : "",
					allcanre ? "\033[1;31m��" :
					"��\033[m", TITLE);
			}
		}
	}
	return buf;
}

static char *
makedatestar(char *datestr, struct fileheader *ent)
{
	char str[30] = " ", buf[30];
	char backcolor[6] = "062351";
	int i, j, better, sz;
	char fg, bg;
	time_t filetime;

	filetime = ent->filetime;
	fg = '1' + localtime(&filetime)->tm_wday;
	bg = backcolor[ent->staravg50 / 50];

	better = (ent->hasvoted - 1) / 5 + 1;
	if (better >= 7)
		better = 7;
	if (ent->hasvoted == 0)
		better = 0;
	if ((ent->accessed & FH_ISTOP))
		better = 0;

	sz = ent->sizebyte;
	if (sz > 30)
		sz = 1 + (sz - 30) / 25;
	else
		sz = 0;

	if (sz > 7)
		sz = 7;

	if (filetime > 740000000)
		strncpy(str + 1, ctime(&filetime) + 4, 6);
	else
		strncpy(str + 1, "      ", 6);
	sprintf(datestr, "\033[1;4;3%c;4%cm", fg, bg);
	j = strlen(datestr);
	for (i = 0; i < 7; i++) {
		if (i == better && better == sz) {
			sprintf(buf, "\033[0;1;3%cm", fg);
			strcpy(datestr + j, buf);
			j += strlen(buf);
		} else if (i == better) {
			if (sz > better)
				sprintf(buf, "\033[0;1;4;3%cm", fg);
			else
				sprintf(buf, "\033[0;1;3%cm", fg);
			strcpy(datestr + j, buf);
			j += strlen(buf);
		} else if (i == sz) {
			if (sz > better)
				sprintf(buf, "\033[0;1;3%cm", fg);
			else
				sprintf(buf, "\033[0;1;3%c;4%cm", fg, bg);
			strcpy(datestr + j, buf);
			j += strlen(buf);
		}
		datestr[j] = str[i];
		j++;
	}
	datestr[j] = 0;
	strcat(datestr, "\033[m");
	return datestr;
}

int
clubtest(char *board)
{
	char buf[256];
	sprintf(buf, "boards/%s/club_users", board);
	return seek_in_file(buf, currentuser->userid);
}

int
clubtestdenysysop(char *board)
{
	char buf[256];
	sprintf(buf, "boards/%s/club_deny_sysop", board);
	return (seek_in_file(buf, currentuser->userid) |
		seek_in_file(buf, "SYSOP"));
}

int
show_board_info()
{
	int pos;
	struct boardheader fh;
	pos = new_search_record(BOARDS, &fh, sizeof (fh), (void *) cmpbnames,
				currboard);
	if (pos <= 0)
		return DONOTHING;
	clear();
	move(2, 0);
	prints("�������ϸ��Ϣ���£�\n\n");
	prints("����������: %s\n", fh.filename);
	prints("������˵��: %s\n", fh.title);
	prints("������: %s\n", fh.sec1);
	prints("��������: %s\n", fh.sec2);
	prints("����������: %s\n", (fh.flag & ANONY_FLAG) ? "Yes" : "No");
	prints("���ֲ�����: %s\n", (fh.flag & CLUB_FLAG) ? "Yes" : "No");
	prints("cn.bbsת��������: %s\n",
	       (fh.flag & INNBBSD_FLAG) ? "Yes" : "No");
	prints("��Ե�ת��������: %s\n",
	       (fh.flag2 & NJUINN_FLAG) ? "Yes" : "No");
	prints("����������: %d\n", fh.limitchar ? fh.limitchar * 100 : 3000);
	prints("���� %s Ȩ��: %s\n",
	       (fh.level & PERM_POSTMASK) ? "POST" :
	       (fh.level & PERM_NOZAP) ? "ZAP" : "READ",
	       (fh.level & ~PERM_POSTMASK) == 0 ? "������" : "������");
	prints("����POST���İ�: %s\n", junkboard()? "Yes" : "No");
	pressanykey();
	return FULLUPDATE;
}
