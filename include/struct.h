/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        G    struct
    {
      char author[IDLEN + 1];
      char board[18];
      char title[66];
      time_t date;
      int number;
    }      postlog;

uy Vega, gtvega@seabass.st.usm.edu
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

#define BM_LEN 60
struct oldboardheader {
	char filename[STRLEN];
	int stocknum;
	int score;
	int clubnum;
	int total;
	int lastpost;
	char BM[BM_LEN - 1];
	char flag;
	char title[STRLEN];
	unsigned level;
	short inboard;
	unsigned char unused[10];
};

struct oldfileheader {		/* This structure is used to hold data in */
	char filename[STRLEN];	/* the DIR files */
	char owner[60];
	time_t edittime;
	char unused[3];
	char realauthor[IDLEN + 1];
	char title[STRLEN];
	unsigned char star_avg:3, hasvoted:5;
	unsigned char sizebyte;
	short viewtime;
	unsigned char accessed[12];	/* struct size = 256 bytes */
};

struct one_key {		/* Used to pass commands to the readmenu */
	int key;
	int (*fptr) ();
	char func[33];		//add by gluon for self-define menu
};

struct postheader {
	char title[STRLEN];
	char ds[40];
	int reply_mode;
	char include_mode;
	int chk_anony;
	int postboard;
};

struct postlog {
	char author[IDLEN + 1];
	char board[18];
	char title[66];
	time_t date;
	int number;
};

struct fivechess {
	int winner;
	int hand, tdeadf, tlivef, livethree, threefour;
	int playboard[15][15];
};

