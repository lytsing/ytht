#ifndef __YTHTBBS_H
#define __YTHTBBS_H
#include "bbsconfig.h"
#include "ythtlib.h"
#define STRLEN               80	/* Length of most string data */
#define NAMELEN              40	/* Length of username/realname */
#define IDLEN                12	/* Length of userids */
#define OLDPASSLEN              14	/* Length of encrypted passwd field */
#define PASSLEN	     		32	
#define MAXMESSAGE	     5
#define BADLOGINFILE "logins.bad"
#define PATHTMPBRC MY_BBS_HOME "/bbstmpfs/brc"
#define PATHUSERATTACH MY_BBS_HOME "/bbstmpfs/userattach"
#define PATHZMODEM MY_BBS_HOME "/bbstmpfs/zmodem"
#define MAXATTACHSIZE (1024*1024*5)
#include "boardrc.h"
#include "misc.h"
#include "record.h"
#include "user.h"
#include "article.h"
#include "modes.h"
#include "docutil.h"
#include "modetype.h"
#include "msg.h"
#include "politics.h"
#include "goodgbid.h"
#include "binaryattach.h"
#include "permissions.h"
#include "bbseva.h"
#include "attach.h"
#include "regform.h"
#include "announce.h"
#include "board.h"
#include "uinfo.h"
#include "bbsinfo.h"
#include "sectree.h"
#include "broadcast.h"
#include "mail.h"
#include "vote.h"
#include "yspam.h"
#endif
