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
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#define MAXLINELEN 80 
#define EXTRALEN 20
#define REAL_ABORT -1
#define KEEP_EDITING -2
#define POST_NOREPLY -3

struct textLine {
        struct textLine *prev, *next;
        char text[MAXLINELEN + EXTRALEN];
        int len;
	char br, changed, selected; //selected ��ѡ���������ʼ��ֵΪ 1, ������ѡ����ֵΪ 2
};

struct edit {
        struct textLine *firstline, *curline;
        int line, col, colcur, linecur; //col: �����ֽ�λ��; colcur: ��Ӧ����ʾλ��
        int position;           //�����ƶ����ʱ��ͼȥ��λ��
        int overwrite;          //���Ƿ�ʽ���߲��뷽ʽ
        int redrawall;
};
