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
    Copyright (C) 1999, Zhou Lin, kcn@cic.tsinghua.edu.cn
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#ifndef  _BBS_H_
#define _BBS_H_

#ifndef BBSIRC

/* Global includes, needed in most every source file... */
//#ifndef ENABLE_FASTCGI
#include <stdio.h>
//#else
//#include <fcgi_stdio.h>
//#endif
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>
#include <utime.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <dirent.h>
#include <sys/ioctl.h>

#ifdef lint
#include <sys/uio.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <stdarg.h>
#include <sys/resource.h>
#include <pwd.h>

#include <sys/wait.h>
#include <netinet/tcp.h>
#include <arpa/telnet.h>

#ifdef AIX
#include <sys/select.h>
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

#if defined(BSD44)
#include <stdlib.h>

#elif defined(LINUX)
/* include nothing :-) */
#elif defined(CYGWIN)
/* include nothing :-) */
#else

#include <rpcsvc/rstat.h>
#endif

#include "sys_config.h"
#include "bbsconfig.h"		/* User-configurable stuff */
#include "ythtbbs.h"

#ifdef BBSMAIN
#define perror prints
#endif

#define VERSION_ID "FIREBIRD 3.0K"

#ifndef LOCK_EX
#define LOCK_EX         2	/* exclusive lock */
#define LOCK_UN         8	/* unlock */
#endif

#ifdef XINU
extern int errno;
#endif

#define randomize() srand((unsigned)time(NULL))

#define YEA (1)			/* Booleans  (Yep, for true and false) */
#define NA  (0)

#define MAXFRIENDS (200)
#define MAXREJECTS (32)
#define GOOD_BRC_NUM    40	// 最多有 GOOD_BRC_NUM 个个人定制版面
#define NUMPERMS   (31)
//#define REG_EXPIRED         180    /* 重做身份确认期限 */

#define FILE_BUFSIZE        250	/* max. length of a file in SHM */
#define FILE_MAXLINE         25	/* max. line of a file in SHM */
#define MAX_WELCOME          15	/* 欢迎画面数 */
#define MAX_GOODBYE          15	/* 离站画面数 */
#define MAX_ISSUE            15	/* 最大进站画面数 */
#define MAX_ENDLINE		15	/* 最大底线叶面数 */
#define MAX_DIGEST         1000	/* 最大文摘数 */
#define MAX_POSTRETRY       100
#define MAXITEMS        1024       /* 精华区最大条目数 */
#define CLUB_SIZE	4          /* 4 * sizeof(int) 为完全 close club数目上限 */
#define MAXnettyLN            5    /* lines of  activity board  */        
#define ACBOARD_BUFSIZE     255    /* max. length of each line for activity board  */
#define ACBOARD_MAXLINE      160    /* max. lines of  activity board  */
#define MAXGOPHERITEMS     9999    /*max of gopher items*/
#define PASSFILE     ".PASSWDS"    /* Name of file User records stored in */
#define ULIST_BASE   ".UTMP"       /* Names of users currently on line */
extern  char ULIST[];

#ifndef BBSIRC 

#define BOARDS      ".BOARDS"      /* File containing list of boards */
#define DOT_DIR     ".DIR"         /* Name of Directory file info */
#define THREAD_DIR  ".THREAD"      /* Name of Thread file info */
#define DIGEST_DIR  ".DIGEST"      /* Name of Digest file info */
#define BADWORDS    "etc/.badwords_new"     /* word list to filter */
#define SBADWORDS   "etc/.sbadwords_new"
#define PBADWORDS   "etc/.pbadwords_new"

#define FILE_READ  0x1		/* Ownership flags used in fileheader structure */
#define FILE_OWND  0x2		/* accessed array */
#define FILE_VISIT 0x4
#define FILE_MARKED 0x8
#define FILE_DIGEST 0x10	/* Digest Mode */
#define FILE_FORWARDED 0x20	/* undelete */
#define FILE_NOREPLY 0x40
#define FILE_ATTACHED 0x80
#define FILE1_DEL 0x1        /* Marked for being deleted */	/*accessed array, the 2nd byte */
#define FILE1_SPEC 0x2		/* Will be put to 0Announce, and this flag would be clear then */
#define FILE1_INND 0x4		/* write into innd/out.bntp */
#define FILE1_ANNOUNCE 0x8	/* have put to 0Announce */
#define FILE1_1984 	0x10	/* have been checked to see if there is any ... */
#define MAIL_REPLY 0x20

#define VOTE_FLAG    0x1
#define NOZAP_FLAG   0x2
#define CLOSECLUB_FLAG 0x4	/* 1 for close club , 0 for open club */
#define ANONY_FLAG   0x8
#define CLUBLEVEL_FLAG 0x10  /** close club表示版面是否可见， 否则表示是否是 open club
                              * 综上所述：
			      * CLOSECLUB_FLAG & CLUBLEVEL_FLAG ，则为除了站务谁都不可见的 close club，
			      * CLOSECLUB_FLAG & !CLUBLEVEL_FLAG，则为能看见版面但进不去的 close club
			      * !CLOSECLUB_FLAG & CLUBLEVEL_FLAG，则为 open club，进行发文限制
			      * !CLOSECLUB_FLAG & !CLUBLEVEL_FLAG，则为普通版面 
			      * 其中仅第一种版面具有 clubnum*/
#define CLUB_FLAG (CLOSECLUB_FLAG | CLUBLEVEL_FLAG)
#define INNBBSD_FLAG 0x20
#define IS1984_FLAG    0x40
#define POLITICAL_FLAG	0x80

/* the flowing is board flag2 */
#define NJUINN_FLAG     0x1
#define WATCH_FLAG	0x2
/* board flag2 end */

#define ZAPPED  0x1		/* For boards...tells if board is Zapped */

/* For All Kinds of Pagers */
#define ALL_PAGER       0x1
#define FRIEND_PAGER    0x2
#define ALLMSG_PAGER    0x4
#define FRIENDMSG_PAGER 0x8

/* END */

#endif				/* BBSIRC */

#include "struct.h"
#define isprint2(c)     ( c<256 && ((c & 0x80) || isprint(c)) )

#ifdef  SYSV
#define bzero(tgt, len)         memset( tgt, 0, len )
#define bcopy(src, tgt, len)    memcpy( tgt, src, len)

#define usleep(usec)            {               \
    struct timeval t;                           \
    t.tv_sec = usec / 1000000;                  \
    t.tv_usec = usec % 1000000;                 \
    select( 0, NULL, NULL, NULL, &t);           \
}

#endif				/* SYSV */
#endif
#endif				/* of _BBS_H_ */
