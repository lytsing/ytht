#include "bbslib.h"
int
bbsboardlistscript_main()
{
	int total;
	struct boardmem *(data[MAXBOARD]);
	char *secstr;
	changemode(READING);
	if (cache_header
	    (max(thisversion, file_time(MY_BBS_HOME "/wwwtmp")), 500))
		return 0;
	secstr = getparm("secstr");
	if (*secstr == '*')
		total = listmybrd(data);
	else
		total = makeboardlist(getsectree(secstr), data);
	printf("Content-type: application/x-javascript; charset=%s\r\n\r\n",
	       CHARSET);
	printf("var boardlistscript=");
	boardlistscript(data, total);
	return 0;
}

int
listmybrd(struct boardmem *(data[]))
{
	struct boardmem *x;
	int i, total = 0;
	readmybrd(currentuser->userid);
	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
		x = &(shm_bcache->bcache[i]);
		if (x->header.filename[0] <= 32 || x->header.filename[0] > 'z')
			continue;
		if (!has_view_perm_x(currentuser, x))
			continue;
		if (!ismybrd(x->header.filename))
			continue;
		data[total] = x;
		total++;
	}
	return total;
}

int
makeboardlist(const struct sectree *sec, struct boardmem *(data[]))
{
	int total = 0, len, hasintro = 0, i;
	struct boardmem *x;
	char *secstr = sec->basestr;
	len = strlen(secstr);
	if (sec->introstr[0])
		hasintro = 1;
	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
		x = &(shm_bcache->bcache[i]);
		if (x->header.filename[0] <= 32 || x->header.filename[0] > 'z')
			continue;
		if (hasintro) {
			if (strcmp(secstr, x->header.sec1) &&
			    strcmp(secstr, x->header.sec2))
				continue;
		} else {
			if (strncmp(secstr, x->header.sec1, len) &&
			    strncmp(secstr, x->header.sec2, len))
				continue;
		}
		if (!has_view_perm_x(currentuser, x))
			continue;
		data[total] = x;
		total++;
	}
	return total;
}

int
boardlistscript(struct boardmem *(data[]), int total)
{
	int i;
	printf("new Array(");
	for (i = 0; i < total; i++) {
		oneboardscript(data[i]);
		if (i < total - 1)
			printf(",\n");
	}
	printf(")");
	return 0;
}

int
oneboardscript(struct boardmem *brd)
{
	char bmbuf[IDLEN * 4 + 4], *ptr;
	int i, limit, brdnum, hasicon;
	struct boardaux *boardaux = getboardaux(getbnumx(brd));
	updatewwwindex(brd);
	brdnum = getbnumx(brd);
	hasicon = brd->wwwicon;
	if (hasicon)
		hasicon += hideboard_x(brd) ? 1 : 0;
	printf("new abrd(%d,'%s','%s',%d,%d,%d,%d,%d,",
	       brdnum, brd->header.filename, scriptstr(brd->header.title),
	       hasicon, brd->total, brd->score,
	       (brd->header.flag & VOTE_FLAG) ? 1 : 0,
	       (brd->header.
		level & PERM_POSTMASK) ? 2 : ((brd->header.
					       flag & CLOSECLUB_FLAG) ? 1 : 0));

	ptr = userid_str(bm2str(bmbuf, &(brd->header)));
	if (!*ptr)
		printf("'ÉÐÎÞ°æÖ÷',");
	else
		printf("'%s',", ptr);
	printf("'%s ',", scriptstr(boardaux->intro));
	limit = 8;
	if ((ptr = strstr(brd->header.filename, "admin"))) {
		if (strlen(ptr) == 5)
			limit = 4;
	}
	if ((brd->header.flag & CLOSECLUB_FLAG))
		printf("''");
	else {
		limit = min(limit, boardaux->nlastmark);
		printf("new Array(");
		for (i = 0; i < limit; i++) {
			struct lastmark *lm = &boardaux->lastmark[i];
			printf("new alm(%d,'%s%s','%s')",
			       lm->thread, (lm->marked) ? "¡ï" : "¡¤",
			       scriptstr(lm->title), lm->authors);
			if (i < limit - 1)
				printf(",");
		}
		printf(")");
	}
	printf(")");
	return 0;
}
