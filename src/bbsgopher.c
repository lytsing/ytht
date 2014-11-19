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
#include "bbsgopher.h"
#include "bbstelnet.h"

int a;
int inrp_by_user = YEA;

GOPHER *g_main[100];		/*100 directories to move in to */
char *cur_page;
GOPHER *tmpitem;
int gopher_position = 0;
char gophertmpfile[40];

int
interrupt_chk()
{
	int key;

	while (1) {
		key = egetch();
		if (!inrp_by_user) {
			return 0;
		}
		if (key == Ctrl('C')) {
			kill(uinfo.pid, SIGINT);
			return 0;
		}
	}
}

static GOPHER *
nth_item(n)
int n;
{
	return (GOPHER *) & (cur_page[n * sizeof (GOPHER)]);
}

void
clear_gophertmpfile()
{
	unlink(gophertmpfile);
}

int
readfield(fd, ptr, maxlen)
int fd;
char *ptr;
int maxlen;
{
	int n;
	int rc;
	char c;

	for (n = 1; n < maxlen; n++) {
		if ((rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\t') {
				*(ptr - 1) = '\0';
				break;
			}
		} else if (rc == 0) {
			if (n == 1)
				return (0);	/* EOF, no data read */
			else
				break;	/* EOF, some data was read */
		} else

			return (-1);	/* error */
	}
	*ptr = 0;		/* Tack a NULL on the end */
	return (n);
}

int
readline(fd, ptr, maxlen)
int fd;
char *ptr;
int maxlen;
{
	int n;
	int rc;
	char c;

	for (n = 1; n < maxlen; n++) {
		if ((rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;
		} else if (rc == 0) {
			if (n == 1)
				return (0);	/* EOF, no data read */
			else
				break;	/* EOF, some data was read */
		} else
			return (-1);	/* error */
	}

	*ptr = 0;		/* Tack a NULL on the end */
	return (n);
}

int
savetmpfile(tmpname)
char tmpname[];
{
	char buf[256];
	FILE *fp;
	int cc;

	if ((fp = fopen(tmpname, "w")) == NULL)
		return -1;
	show_message("\033[5mת���ļ�����Ϊ�ݴ浵");
	fprintf(fp, "��  Դ: %s\n", tmpitem->server);
	fprintf(fp, "��  ��: %s(ʹ�� %d ��)\n", tmpitem->file, tmpitem->port);
	fprintf(fp, "��  ��: %s\n\n", tmpitem->title + 1);
	while (1) {
		if ((cc = read(a, buf, 255)) > 0) {
			buf[cc] = '\0';
			fprintf(fp, "%s", buf);
		} else {
/*                fclose(fp);  */
			break;
		}
	}
	show_message(NULL);
	fclose(fp);
	close(a);
	return 1;

}

void
print_gophertitle()
{
	char buf[256];
	char title[256];

	sprintf(title, "%s", g_main[gopher_position]->title + 1);
	title[70] = '\0';
	move(0, 0);
	clrtobot();
	sprintf(buf, "%*s", (int) (80 - strlen(title)) / 2, " ");
	prints("\033[1;44m%s%s%s\033[m\n", buf, title, buf);
	prints
	    ("             \033[1;32mF \033[37m�Ļ��Լ������� \033[32m ����\033[37m �ƶ�  \033[32m�� <Enter>\033[37m ��ȡ \033[32m ��\033[37m �뿪");
}

void
printgopher_title()
{

	move(2, 0);
	clrtoeol();

	prints
	    ("\033[1;37;44m ��� [���] ��    ��                                                          \033[m\n");
}

void
g_refresh()
{
	print_gophertitle();
	show_gopher();
	update_endline();
}

int
deal_gopherkey(ch, allnum, pagenum)
char ch;
int allnum, pagenum;
{
	char fname[STRLEN], fpath[STRLEN];

	switch (ch) {
	case 'h':
	case 'H':
		show_help("help/announcereadhelp");
		g_refresh();
		break;
	case 'E':
	case 'e':
		tmpitem = nth_item(allnum - pagenum);
		setuserfile(fname, "gopher.tmp");
		if (tmpitem->title[0] != '0') {
			return 1;
		}
		if (get_con(tmpitem->server, tmpitem->port) == -1)
			return 1;
		enterdir(tmpitem->file);
		savetmpfile(fname);
		if (file_isfile(fname)) {
			vedit(fname, NA, YEA);
			unlink(fname);
			g_refresh();
		}
		show_message(NULL);
		break;
	case '=':
		{
			tmpitem = nth_item(allnum - pagenum);
			move(2, 0);
			clrtobot();
			prints("\033[1;44;37m");
			printdash("BBS Gopher �����������");
			prints("\033[m");
			prints("���ͣ�%c (%s)\n", tmpitem->title[0],
			       (tmpitem->title[0] == '0') ? "�ļ�" : "Ŀ¼");
			prints("���⣺%s\n", tmpitem->title + 1);
			prints("·����%s\n", tmpitem->file);
			prints("λ�ã�%s\n", tmpitem->server);
			prints("ʹ�ã�%d��\n", tmpitem->port);
			pressanykey();
			g_refresh();
		}
		break;
	case Ctrl('P'):
		tmpitem = nth_item(allnum - pagenum);
		if (!USERPERM(currentuser, PERM_POST))
			break;
		setuserfile(fname, "gopher.tmp");
		if (tmpitem->title[0] != '0') {
			return 1;
		}
		if (get_con(tmpitem->server, tmpitem->port) == -1)
			return 1;
		enterdir(tmpitem->file);
		savetmpfile(fname);
		if (file_isfile(fname)) {
			char bname[30];
			clear();
			if (get_a_boardname
			    (bname, "������Ҫת��������������: ")) {
				move(1, 0);
				sprintf(fpath, "��ȷ��Ҫת���� %s ����", bname);
				if (askyn(fpath, NA, NA) == 1) {
					move(2, 0);
					postfile(fname, bname,
						 tmpitem->title + 1, 2);
					sprintf(fpath,
						"\033[1m�Ѿ�����ת���� %s ����...\033[m",
						bname);
					prints(fpath);
					refresh();
					sleep(1);
				}
			}
		}
		unlink(fname);
		g_refresh();
		return 1;
	case 'U':
	case 'F':
	case 'u':
	case 'f':
	case 'z':
	case 'Z':
		tmpitem = nth_item(allnum - pagenum);
		setuserfile(fname, "gopher.tmp");
		if (tmpitem->title[0] != '0') {
			return 1;
		}
		if (get_con(tmpitem->server, tmpitem->port) == -1)
			return 1;
		enterdir(tmpitem->file);
		savetmpfile(fname);
		if (!file_isfile(fname))
			return 1;
		switch (doforward(fname, tmpitem->title + 1,
				  (ch == 'u' || ch == 'U') ? 1 : 0)) {
		case 0:
			show_message("����ת�����!");
			break;
		case -1:
			show_message("system error!!.");
			break;
		case -2:
			show_message("invalid address.");
			break;
		default:
			show_message("ȡ��ת�Ķ���.");
		}
		sleep(2);
		pressanykey();
		g_refresh();
		unlink(fname);
		return 1;
	default:
		return 0;
	}
	return 1;
}

void
enterdir(path)
char path[];
{
	char buf[256];
	sprintf(buf, "%s\r\n", path == NULL ? "" : path);
	write(a, buf, sizeof (buf));
}

int
get_con(servername, port)
char *servername;
int port;
{
	struct hostent *h2;
	struct sockaddr_in sin;

	inrp_by_user = YEA;
	show_message("���������� (Ctrl-C �ж�) ...");
	if ((h2 = gethostbyname(servername)) == NULL)
		sin.sin_addr.s_addr = inet_addr(servername);
	else
		memcpy(&sin.sin_addr.s_addr, h2->h_addr, h2->h_length);
	sin.sin_family = AF_INET;
	if (!(a = socket(AF_INET, SOCK_STREAM, 0))) {
		perror("Socket:");
	}
	sin.sin_port = htons(port);
	signal(SIGALRM, (void *) interrupt_chk);
	siginterrupt(SIGINT, 1);
	alarm(1);
	if ((connect(a, (struct sockaddr *) &sin, sizeof (sin)))) {
		signal(SIGALRM, SIG_IGN);
		perror("connect fail");
		return -1;
	}
	signal(SIGALRM, SIG_IGN);
	inrp_by_user = NA;
#ifdef SSHBBS
	ssh_write(0, " ", 1);
#else
	write(0, " ", 1);
#endif
	show_message(NULL);
	return 1;
}

int
do_gopher(page, num)
int page, num;
{
	return -1;
}

int
show_gopher()
{
	extern int page, range;
	int i;
	GOPHER *tmpnode;

	printgopher_title();
	move(3, 0);
	clrtobot();
	get_records(gophertmpfile, cur_page, sizeof (GOPHER), page + 1, 19);
	for (i = page; i < page + 19 && i < range; i++) {
		tmpnode = nth_item(i - page);
		move(i - page + 3, 0);
		prints(" %4d [\033[1m%9s\033[m] %-65s\n", i + 1,
		       ((tmpnode->title[0] == '0') ? "\033[32m����" :
			"\033[33m��Ŀ"), tmpnode->title + 1);
	}
	return 0;
}

void
showout()
{
	int i = 0, i2 = 0;
	char foo[1024];
	char tmpfile[STRLEN];
	char buf[20];
	int notreload = 0;
	GOPHER newitem;

	while (1) {
		if (gopher_position < 0) {
			return;
		}
		print_gophertitle();
		printgopher_title();
		update_endline();
		if (!notreload) {
			i = 0;
			if (get_con
			    (g_main[gopher_position]->server,
			     g_main[gopher_position]->port) == -1) {
				show_message(NULL);
				free(g_main[gopher_position]);
				gopher_position--;
				notreload = 0;
				continue;
			}
			enterdir(g_main[gopher_position]->file);
			show_message("��ȡ׼����");
			for (i = 0; i < MAXGOPHERITEMS; i++) {
				if (readfield(a, foo, 1024) <= 0) {
					break;
				}
				if (foo[0] == '.' && foo[1] == '\r'
				    && foo[2] == '\n') {
					break;
				}
				strncpy(newitem.title, foo, 70);
				newitem.title[69] = 0;
				if (readfield(a, foo, 1024) == 0) {
					break;
				}
				strncpy(newitem.file, foo, 80);
				newitem.file[79] = 0;
				if (readfield(a, foo, 1024) == 0) {
					break;
				}
				strncpy(newitem.server, foo, 40);
				newitem.server[39] = 0;
				if (readline(a, foo, 1024) == 0) {
					break;
				}
				newitem.port = atoi(foo);
				if (newitem.title[0] != newitem.file[0]) {
					break;
				}
				if (newitem.title[0] != '0'
				    && newitem.title[0] != '1') {
					i--;
					continue;
				}
				refresh();
				append_record(gophertmpfile, &newitem,
					      sizeof (GOPHER));
				sprintf(buf,
					"\033[1;3%dmת\033[3%dm��\033[3%dm��\033[3%dm��\033[3%dm��\033[m",
					(i % 7) + 1, ((i + 1) % 7) + 1,
					((i + 2) % 7) + 1, ((i + 3) % 7) + 1,
					((i + 4) % 7) + 1);
				show_message(buf);
			}
			show_message(NULL);
		} else
			notreload = 0;
		if (i <= 0) {
			move(3, 0);
			clrtobot();
			move(10, 0);
			clrtoeol();
			prints
			    ("                             \033[1;31mû���κε�����...\033[m");
			pressanykey();
			free(g_main[gopher_position]);
			gopher_position--;
			continue;
		}
		close(a);
		move(0, 0);
		clrtobot();
		setlistrange(i);
		i2 = choose(NA, g_main[gopher_position]->position,
			    print_gophertitle, deal_gopherkey, show_gopher,
			    do_gopher);
		if (i2 == -1) {
			free(g_main[gopher_position]);
			clear_gophertmpfile();
			gopher_position--;
			continue;
		}
		g_main[gopher_position]->position = i2;
		get_record(gophertmpfile, &newitem, sizeof (GOPHER), i2 + 1);
		tmpitem = &newitem;
		if (newitem.title[0] == '0') {
			if (get_con(newitem.server, newitem.port) == -1)
				continue;
			enterdir(newitem.file);
			setuserfile(tmpfile, "gopher.tmp");
			savetmpfile(tmpfile);
			ansimore(tmpfile, YEA);
			notreload = 1;
			unlink(tmpfile);
			continue;
		} else {
			GOPHER *newgi;

			clear_gophertmpfile();
			gopher_position++;
			newgi = (GOPHER *) malloc(sizeof (GOPHER));
			strncpy(newgi->server, tmpitem->server, 40);
			newgi->server[39] = 0;
			strncpy(newgi->file, tmpitem->file, 80);
			newgi->file[STRLEN - 1] = 0;
			strncpy(newgi->title, tmpitem->title, 70);
			newgi->title[69] = 0;
			newgi->port = tmpitem->port;
			newgi->position = 0;
			g_main[gopher_position] = newgi;
			continue;
		}
	}
}

int
gopher(serv, dire, port, title)
char serv[], dire[], title[];
int port;
{
	GOPHER *newitem;
	char buf[80];

	modify_user_mode(CCUGOPHER);
	sprintf(gophertmpfile, "tmp/gophertmp.%s.%05d", currentuser->userid,
		uinfo.pid);
	gopher_position = 0;
	newitem = (GOPHER *) malloc(sizeof (GOPHER));
	strncpy(newitem->server, serv, 40);
	newitem->server[39] = 0;
	strncpy(newitem->file, dire, 80);
	newitem->file[79] = 0;
	sprintf(buf, " %s", title);
	strncpy(newitem->title, buf, 70);
	newitem->title[69] = 0;
	newitem->port = port;
	newitem->position = 0;
	g_main[gopher_position] = newitem;
	cur_page = (char *) malloc(sizeof (GOPHER) * 19);
	showout();
	free(cur_page);
	modify_user_mode(DIGEST);
	return 0;
}
