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

#include "modes.h"
#include "ythtbbs.h"

char *
ModeType(int mode)
{
	switch (mode) {
	case IDLE:
		return "";
	case NEW:
		return "��վ��ע��";
	case LOGIN:
		return "���뱾վ";
	case DIGEST:
		return "���������";
	case MMENU:
		return "��ѡ��";
	case ADMIN:
		return "������ѡ��";
	case SELECT:
		return "ѡ��������";
	case READBRD:
		return "��������";
	case READNEW:
		return "��������";
	case READING:
		return "Ʒζ����";
	case POSTING:
		return "�ĺ��ӱ�";
	case MAIL:
		return "�����ż�";
	case SMAIL:
		return "�����Ÿ�";
	case RMAIL:
		return "�����ż�";
	case TMENU:
		return "����ѡ��";
	case LUSERS:
		return "�����ķ�";
	case FRIEND:
		return "Ѱ�Һ���";
	case MONITOR:
		return "̽������";
	case QUERY:
		return "��ѯ����";
	case TALK:
		return "����";
	case PAGE:
		return "����";
	case CHAT1:
		return "��������";
	case CHAT2:
		return "��Ϳ������";
	case CHAT3:
		return "����������";
	case CHAT4:
		return "վ�������";
	case IRCCHAT:
		return "��̸IRC";
	case LAUSERS:
		return "̽������";
	case XMENU:
		return "ϵͳ��Ѷ";
	case VOTING:
		return "ͶƱ";
	case BBSNET:
		return "BBSNET";
	case EDITWELC:
		return "�༭Welc";
	case EDITUFILE:
		return "�༭���˵�";
	case EDITSFILE:
		return "����ϵͳ��";
	case ZAP:
		return "����������";
	case GAME:
		return "��������";
	case SYSINFO:
		return "���ϵͳ";
	case ARCHIE:
		return "ARCHIE";
	case DICT:
		return "�����ֵ�";
	case LOCKSCREEN:
		return "ӫĻ����";
	case NOTEPAD:
		return "���԰�";
	case GMENU:
		return "������";
	case MSG:
		return "ѶϢ��";
	case USERDEF:
		return "�Զ�����";
	case EDIT:
		return "�޸�����";
	case OFFLINE:
		return "��ɱ��..";
	case EDITANN:
		return "���޾���";
	case WWW:
		return "���� WWW";
	case HYTELNET:
		return "Hytelnet";
	case CCUGOPHER:
		return "��վ����";
	case LOOKMSGS:
		return "�쿴ѶϢ";
	case WFRIEND:
		return "Ѱ������";
	case WORKER:
		return "������";
	case TETRIS:
		return "����˹����";
	case WINMINE:
		return "ɨ��";
	case WINMINE2:
		return "��Ӧʽɨ��";
	case TT:
		return "������ϰ";
	case ADDRESSBOOK:
		return "�쿴ͨѶ¼";
	case SELBACKNUMBER:
		return "ѡ�����";
	case BACKNUMBER:
		return "�������";
	case RECITE:
		return "������";
	case CHESS:
		return "��������";
	case QKMJ:
		return "���齫";
	case NCCE:
		return "�Ƽ��ʵ�";
	case M_2NDHAND:
		return "�����г�";
	case DO1984:
		return "�о�����";
	case MONEY:
		return "ר����Ǯ";
	case QUICKCALC:
		return "��������";
	case FREEIP:
		return "��ѯIP";
	case FRIENDTEST:
		return "�������";
	case KILLER:
		return "ɱ������";
	default:
		return "ȥ������!?";
	}
}

char *
ModeColor(int mode)
{
	switch (mode) {
	case POSTING:
		return "\033[1;32m";
	case CHAT1:
	case CHAT2:
	case CHAT3:
	case CHAT4:
	case BBSNET:
		return "\033[1;33m";
	default:
		return "";
	}
}
