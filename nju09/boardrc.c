#include "bbslib.h"

static struct allbrc allbrc;
static char allbrcuser[STRLEN];
static struct onebrc *pbrc, brc;
static int brcexpired = 1;

void
brc_expired()
{
	brcexpired = 1;
}

static int
readuserallbrc(char *userid, char *userhost)
{
	char buf[STRLEN];
	int oldbrcexpired = brcexpired;
	if (!userid)
		return 0;
	brcexpired = 0;
	if (!*userid || !strcmp(userid, "guest")) {
		snprintf(buf, sizeof (buf), "guest.%s", userhost);
		if (!oldbrcexpired
		    && !strncmp(allbrcuser, buf, sizeof (allbrcuser))) return 0;
		strsncpy(allbrcuser, buf, sizeof (allbrcuser));
		brc_init(&allbrc, allbrcuser, NULL);

	} else {
		if (!oldbrcexpired
		    && !strncmp(allbrcuser, userid, sizeof (allbrcuser)))
			    return 0;
		sethomefile(buf, userid, "brc");
		strsncpy(allbrcuser, userid, sizeof (allbrcuser));
		brc_init(&allbrc, userid, buf);
	}
	return 0;
}

static void
brc_dosave(char *userid, char *userhost)
{
	if (!u_info)
		return;
	if (!*userid || !strcmp(userid, "guest")) {
		char str[STRLEN];
		sprintf(str, "guest.%s", userhost);
		brc_fini(&allbrc, str);
	} else
		brc_fini(&allbrc, userid);
}

void
brc_update(char *userid)
{
	return;
#if 0
	if (!pbrc->changed || !u_info)
		return;
	readuserallbrc(userid);
	brc_putboard(&allbrc, pbrc);
	brc_dosave(userid, realfromhost);
#endif
}

struct brcinfo *
brc_readinfo(char *userid)
{
	static struct brcinfo info;
	readuserallbrc(userid, realfromhost);
	bzero(&info, sizeof (info));
	brc_getinfo(&allbrc, &info);
	return &info;
}

void
brc_saveinfo(char *userid, struct brcinfo *info)
{
	brc_putinfo(&allbrc, info);
	brc_dosave(userid, realfromhost);
}

int
brc_initial(char *userid, char *boardname)
{
	if (u_info)
		pbrc = &u_info->brc;
	else {
		pbrc = &brc;
		bzero(&brc, sizeof (brc));
		return 0;
	}
	if (boardname && !strncmp(boardname, pbrc->board, sizeof (pbrc->board)))
		return 0;
	readuserallbrc(userid, realfromhost);
	//更换版面时，保存上一个版面的阅读纪录
	if (pbrc->changed && u_info) {
		brc_putboard(&allbrc, pbrc);
		brc_dosave(userid, realfromhost);
	}
	//调入新版面的阅读纪录
	if (boardname)
		brc_getboard(&allbrc, pbrc, boardname);
	return 0;
}

int
brc_uentfinial(struct user_info *uinfo)
{
	if (!uinfo->active || uinfo->pid != 1 || !uinfo->brc.changed)
		return 0;
	pbrc = &uinfo->brc;
	readuserallbrc(uinfo->userid, uinfo->from);
	brc_putboard(&allbrc, pbrc);
	brc_dosave(uinfo->userid, uinfo->from);
	return 0;
}

void
brc_add_read(struct fileheader *fh)
{
	SETREAD(fh, pbrc);
}

void
brc_add_readt(int t)
{
	brc_addlistt(pbrc, t);
}

int
brc_un_read(struct fileheader *fh)
{
	return UNREAD(fh, pbrc);
}

void
brc_clear()
{
	brc_clearto(pbrc, time(NULL));
}

int
brc_un_read_time(int ftime)
{
	return brc_unreadt(pbrc, ftime);
}

int
brc_board_read(char *board, int ftime)
{
	return !brc_unreadt_quick(&allbrc, board, ftime);
}
