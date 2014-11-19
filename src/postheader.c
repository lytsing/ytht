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
extern int numofsig;
struct boardmem *getbcache();

static void
write_back_signature(short new_signature)
{
	char buf[20];
	if (new_signature == uinfo.signature)
		return;
	uinfo.signature = new_signature;
	snprintf(buf, 20, "%d", new_signature);
	saveuservalue(currentuser->userid, "signature", buf);
}

int
post_header(header)
struct postheader *header;
{
	int anonyboard = 0;
	char r_prompt[20], mybuf[256], ans[5], genbuf[STRLEN];
	char titlebuf[STRLEN];
	struct boardmem *bp;
	short new_signature = uinfo.signature;
	if (new_signature > numofsig || new_signature < -1) {
		new_signature = 1;
		write_back_signature(new_signature);
	}
	if (header->reply_mode) {
		strcpy(titlebuf, header->title);
		header->include_mode = 'S';
	} else
		titlebuf[0] = '\0';
	bp = getbcache(currboard);
	if (bp == NULL) {
		return NA;
	}
	if (header->postboard)
		anonyboard = bp->header.flag & ANONY_FLAG;
	if (strcmp(currboard, DEFAULTANONYMOUS))
		header->chk_anony = 0;
	else
		header->chk_anony = 1;

	while (1) {
		if (header->reply_mode)
			sprintf(r_prompt, "����ģʽ [\033[1m%c\033[m]",
				header->include_mode);
		move(t_lines - 4, 0);
		clrtobot();
		prints("%s \033[1m%s\033[m      %s\n",
		       (header->postboard) ? "����������" : "�����ˣ�",
		       header->ds,
		       (anonyboard) ? (header->chk_anony ==
				       1 ? "\033[1mҪ\033[mʹ������" :
				       "\033[1m��\033[mʹ������") : "");
		move(t_lines - 3, 0);
		prints("ʹ�ñ���: \033[1m%-50s\033[m\n", (header->title[0] ==
							  '\0') ?
		       "[�����趨����]" : header->title);
		move(t_lines - 2, 0);
		sprintf(genbuf, "�� \033[1m%d\033[m ��", new_signature);
		prints("ʹ��%sǩ����     %s",
		       ((new_signature == -1) ? "���" : genbuf)
		       , (header->reply_mode) ? r_prompt : "");
		if (titlebuf[0] == '\0') {
			move(t_lines - 1, 0);
			if (header->postboard == YEA
			    || strcmp(header->title, "û����"))
				strcpy(titlebuf, header->title);
			getdata(t_lines - 1, 0, "����: ", titlebuf, 50, DOECHO,
				NA);
			{
				int i = strlen(titlebuf) - 1;
				while (i > 0 && isspace(titlebuf[i]))
					titlebuf[i--] = 0;
			}
			if (titlebuf[0] == '\0') {
				if (header->title[0] != '\0') {
					titlebuf[0] = ' ';
					continue;
				} else {
					write_back_signature(new_signature);
					return NA;
				}
			}
			strcpy(header->title, titlebuf);
			continue;
		}
		move(t_lines - 1, 0);
		sprintf(mybuf,
			"�밴\033[1;32m0\033[m~\033[1;32m%d/V/L\033[mѡ/��/���ǩ����%s,"
			"\033[1;32mT\033[m�ı���%s,%s\033[1;32mEnter\033[mȷ��:",
			numofsig,
			(header->reply_mode) ?
			",\033[1;32mS\033[m/\033[1;32mY\033[m/\033[1;32mN\033[m/\033[1;32mR\033[m/\033[1;32mA\033[m������ģʽ"
			: "", (anonyboard) ? ",\033[1;32mC\033[m����" : "",
			in_mail ? "" : "\033[1;32mQ\033[m����,");
		getdata(t_lines - 1, 0, mybuf, ans, 3, DOECHO, YEA);
		ans[0] = toupper(ans[0]);
		if ((ans[0] - '0') >= 0 && ans[0] - '0' <= 9) {
			if (atoi(ans) <= numofsig) {
				new_signature = atoi(ans);
			}
		} else if (ans[0] == 'Q' && !in_mail) {
			write_back_signature(new_signature);
			return NA;
		} else if ((header->reply_mode &&
			    (ans[0] == 'Y' || ans[0] == 'N'
			     || ans[0] == 'A' || ans[0] == 'R'))
			   || ans[0] == 'S') {
			header->include_mode = ans[0];
		} else if (ans[0] == 'T') {
			titlebuf[0] = '\0';
		} else if (ans[0] == 'C' && anonyboard) {
			header->chk_anony = (header->chk_anony == 1) ? 0 : 1;
		} else if (ans[0] == 'V') {
			setuserfile(mybuf, "signatures");
			if (askyn("Ԥ����ʾǰ����ǩ����, Ҫ��ʾȫ����", NA, YEA)
			    == YEA)
				ansimore(mybuf, YEA);
			else {
				clear();
				ansimore2(mybuf, YEA, 0, 18);
			}
		} else if (ans[0] == 'L') {
			new_signature = -1;
		} else {
			if (header->title[0] == '\0') {
				write_back_signature(new_signature);
				return NA;
			} else {
				write_back_signature(new_signature);
				return YEA;
			}
		}
	}
}
