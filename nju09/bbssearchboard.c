#include "bbslib.h"

//secstr == NULL: all boards
//secstr == "": boards that doesnn't belong to any group
//return the bnum of one of the match boards
static int
searchlist_alphabetical(const char *sectitle, const char *secstr, char *match,
			int showintro, int *bnum)
{
	struct boardmem *(data[MAXBOARD]), *bx;
	int i, len = 0, total = 0;
	if (secstr) {
		len = strlen(secstr);
		if (len == 0)
			len = 1;
	}
	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
		bx = &shm_bcache->bcache[i];
		if (secstr && strncmp(bx->header.sec1, secstr, len)
		    //&& strncmp(bx->header.sec2, secstr, len)
		    )
			continue;
		if (!strcasestr(bx->header.filename, match) &&
		    !strcasestr(bx->header.title, match) &&
		    !strcasestr(getboardaux(i)->intro, match))
			continue;
		if (has_view_perm_x(currentuser, bx)) {
			*bnum = i;
			data[total] = bx;
			total++;
		}
	}
	if (!total)
		return 0;

	printhr();
	printf("<b>分区: <a href=boa?secstr=%s>%s</a></b>", secstr,
	       nohtml(sectitle));
	qsort(data, total, sizeof (struct boardmem *), (void *) cmpboard);
	printf("<table width=100%%>\n");
	for (i = 0; i < total; i++) {
		if (showintro || (i && i % 3 == 0))
			printf("</tr>");
		if (showintro || i % 3 == 0)
			printf("\n<tr valign=top bgcolor=%x>",
			       0xf0f0d0 | (unsigned) (i % 2) * 0x0f0f0f);
		printf
		    ("<td><a href='javascript:setEl(\"%s\")'><nobr>%s(%s)</nobr></a></td>",
		     data[i]->header.filename, data[i]->header.filename,
		     nohtml(data[i]->header.title));
		if (showintro) {
			struct boardaux *baux;
			baux = getboardaux(getbnumx(data[i]));
			if (baux != NULL)
				printf("<td>%s</td>", nohtml(baux->intro));
		}
	}
	printf("</table>\n");
	return total;
}

static void
searchlist_grouped(char *match, int dored)
{
	int i, count = 0, bnum = 0;
	for (i = 0; i < sectree.nsubsec; i++) {
		count += searchlist_alphabetical(sectree.subsec[i]->title,
						 sectree.subsec[i]->basestr,
						 match, *match, &bnum);
	}
	if (!count) {
		printf("一个都没找到，换个关键词试试？");
	} else if (count == 1 && dored) {
		char buf[80];
		sprintf(buf, "home?B=%d", bnum);
		if (!(shm_bcache->bcache[bnum].header.flag & CLOSECLUB_FLAG))
			redirect(buf);
	}
}

int
bbssearchboard_main()
{
	char *element, *match;
	element = getparm("element");
	match = getparm("match");
	html_header(1);
	printf("</head><body>");
	check_msg();
	if (*element) {
		printf("<script>\n"
		       "function setEl(value) {opener.document.%s=value;window.close();}\n"
		       "</script>\n", element);
	} else {
		printf("<script>\n"
		       "function setEl(value) {location.href='home?B='+value;}\n"
		       "</script>");
	}
	printf("<form>版面搜索：<input type=text value='%s' name=match> "
	       "<input type=hidden name=element value='%s'>"
	       "<input type=submit value=搜索></form>", nohtml(match), element);
	searchlist_grouped(match, *element ? 0 : 1);
	return 0;
}
