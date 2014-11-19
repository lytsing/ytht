#include "bbslib.h"

int
bbsboa_main()
{
	char *secstr;
	const struct sectree *sec;
	struct brcinfo *brcinfo;

	brcinfo = brc_readinfo(currentuser->userid);

	secstr = getparm("secstr");
	sec = getsectree(secstr);

	if (secstr[0] != '*' && !strcmp(sec->basestr, brcinfo->lastsec)) {
		if (cache_header
		    (max(thisversion, file_time(MY_BBS_HOME "/wwwtmp")), 120))
			return 0;
	}

	if (secstr[0] != '*' && strcmp(sec->basestr, brcinfo->lastsec)) {
		strsncpy(brcinfo->lastsec, sec->basestr,
			 sizeof (brcinfo->lastsec));
		brc_saveinfo(currentuser->userid, brcinfo);
	}

	html_header(1);
	check_msg();
	printf("<style type=text/css>A {color: #0000f0}</style></head>");
	changemode(SELECT);
	if (secstr[0] == '*') {
		printf("<body><center>\n");
		printf("%s -- 预定讨论区总览<hr>", BBSNAME);
		showboardlist("*");
		printf("<hr>");
		return 0;
	}
	printf("<body topmargin=0 leftMargin=1 MARGINWIDTH=1 MARGINHEIGHT=0>");
	showsecpage(sec);
	printf("</body></html>");
	return 0;
}

int
showsecpage(const struct sectree *sec)
{
	FILE *fp;
	char buf[1024], *param, *ptr;
	sprintf(buf, "wwwtmp/secpage.sec%s", sec->basestr);
	fp = fopen(buf, "rt");
	if (!fp)
		return showdefaultsecpage(sec);
	while (fgets(buf, sizeof (buf), fp)) {
		if (buf[0] != '#') {
			fputs(buf, stdout);
			continue;
		}
		ptr=strtok(buf, " \t\r\n");
		param=strtok(NULL, " \t\r\n");
		if (!strcmp(ptr, "#showfile")&&param) {
			showfile(param);
		} else if (!strcmp(ptr, "#showblist")) {
			showboardlist(sec->basestr);
		} else if (!strcmp(ptr, "#showsecintro"))
			showsecintro(sec);
		else if (!strcmp(ptr, "#showsecnav"))
			showsecnav(sec);
		else if (!strcmp(ptr, "#showstarline")&&param)
			showstarline(param);
		else if (!strcmp(ptr, "#showhotboard")&&param)
			showhotboard(sec, param);
		else if (!strcmp(ptr, "#showsechead"))
			showsechead(sec);
		else if (!strcmp(ptr, "#showsecmanager"))
			showsecmanager(sec);
		else if(!strcmp(ptr,"#showboardlistscript"))
			showboardlistscript(sec->basestr);
	}
	fclose(fp);
	return 0;
}

int
showdefaultsecpage(const struct sectree *sec)
{
	printf("<center>");
	showsechead(sec);
	printf("<h2>%s</h2>", nohtml(sec->title));
	printf("<div align=right>");
	showsecmanager(sec);
	printf("</div>");
	printf("<hr>");
	if (showsecintro(sec) == 0)
		printf("<hr>");
	if(showboardlist(sec->basestr))
		printf("<hr>");
	printf("</center>");
	return 0;
}

int
showsechead(const struct sectree *sec)
{
	const struct sectree *sec1, *sec2;
	int i, ntr;
	sec2 = sec;
	while (sec2->parent && sec2->parent != &sectree)
		sec2 = sec2->parent;
	printf("<table border=1 class=colortb1><tr>");
	if (sec == &sectree)
		printf("<td align=center class=f1>&nbsp;<b>%s</b>&nbsp;</td>",
		       nohtml(sectree.title));
	else
		printf
		    ("<td align=center class=f1>&nbsp;<a href=boa?secstr=? class=blk>%s</a>&nbsp;</td>",
		     nohtml(sectree.title));
	ntr = sectree.nsubsec / 9 + 1;
	for (i = 0; i < sectree.nsubsec; i++) {
		sec1 = sectree.subsec[i];
		if (sec1 == sec)
			printf
			    ("<td align=center class=f1>&nbsp;<b>%s</b>&nbsp;</td>",
			     nohtml(sec1->title));
		else if (sec1 == sec2)
			printf
			    ("<td align=center class=f1>&nbsp;<b><a href=boa?secstr=%s class=blk>%s</a></b>&nbsp;</td>",
			     sec1->basestr, nohtml(sec1->title));
		else
			printf
			    ("<td align=center class=f1>&nbsp;<a href=boa?secstr=%s class=blk>%s</a>&nbsp;</td>",
			     sec1->basestr, nohtml(sec1->title));
		if (i != sectree.nsubsec - 1
		    && (i + 2) % (sectree.nsubsec / ntr + 1) == 0)
			printf("</tr><tr>");
	}
	printf("</tr></table>");
	return 0;
}

int
showstarline(char *str)
{
	printf("<tr><td class=tb2_blk><font class=star>★</font>"
	       "&nbsp;%s</td></tr>", str);
	return 0;
}

int
showsecnav(const struct sectree *sec)
{
	char buf[256];
	printf("<table width=100%%>");
	sprintf(buf,
		"近日精彩话题推荐 &nbsp;(<a href=bbsshownav?secstr=%s class=blk>"
		"查看全部</a>)", sec->basestr);
	showstarline(buf);
	printf("<tr><td>");
	shownavpart(0, sec->basestr);
	printf("</td></tr></table>");
	return 0;
}

int
genhotboard(struct hotboard *hb, const struct sectree *sec, int max)
{
	int count = 0, i, j, len;
	struct boardmem *bmem[MAXHOTBOARD], *x, *x1;
	if (max < 3 || max > MAXHOTBOARD)
		max = 10;
	if (max == hb->max && hb->uptime > shm_bcache->uptime)
		return hb->count;
	len = strlen(sec->basestr);
	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
		x = &(shm_bcache->bcache[i]);
		if (x->header.filename[0] <= 32 || x->header.filename[0] > 'z')
			continue;
		if (hideboard_x(x))
			continue;
		if (strncmp(sec->basestr, x->header.sec1, len) &&
		    strncmp(sec->basestr, x->header.sec2, len))
			continue;
		for (j = 0; j < count; j++) {
			if (x->score > bmem[j]->score)
				break;
			if (x->score == bmem[j]->score
			    && x->inboard > bmem[j]->inboard)
				break;
		}
		for (; j < count; j++) {
			x1 = bmem[j];
			bmem[j] = x;
			x = x1;
		}
		if (count < max)
			bmem[count++] = x;
	}
	for (i = 0; i < count; i++) {
		strsncpy(hb->bname[i], bmem[i]->header.filename,
			 sizeof (hb->bname[0]));
		strsncpy(hb->title[i], bmem[i]->header.title,
			 sizeof (hb->title[0]));
		hb->bnum[i] = getbnumx(bmem[i]);
	}
	hb->max = max;
	hb->count = count;
	hb->uptime = now_t;
	hb->sec = sec;
	return count;
}

int
showhotboard(const struct sectree *sec, char *s)
{
	static struct hotboard *(hbs[50]);
	static int count = 0;
	struct hotboard *hb = NULL;
	int i;
	for (i = 0; i < count; i++) {
		if (hbs[i]->sec == sec) {
			hb = hbs[i];
			break;
		}
	}
	if (!hb) {
		if (count >= 50)
			return -1;
		hbs[count] = calloc(1, sizeof (struct hotboard));
		hb = hbs[count];
		count++;
	}
	genhotboard(hb, sec, atoi(s));
	printf
	    ("<table style='width:360pt' border=1><tr><td class=colortb1 width=15%% align=center>热门讨论区推荐</td><td>");
	for (i = 0; i < hb->count; i++) {
		if (i)
			printf("%s", " &nbsp;");
		printf("<a href=home?B=%d class=pur><u>%s</u></a>",
		       hb->bnum[i], void1(nohtml(hb->title[i])));
	}
	printf("</td></tr></table>");
	return 0;
}

int
showfile(char *fn)
{
	struct mmapfile mf = { ptr:NULL };
	int retv = 0;
	MMAP_TRY {
		if (mmapfile(fn, &mf) < 0) {
			MMAP_RETURN(-1);
		}
		retv = fwrite(mf.ptr, 1, mf.size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	return retv;
}

int
showsecintro(const struct sectree *sec)
{
	char filename[80];
	int i;
	if (!sec->introstr[0])
		return -1;
	sprintf(filename, "wwwtmp/lastmark.js.sec%s", sec->basestr);
	printf("<script>");
	if (showfile(filename) < 0) {
		printf("</script>");
		for (i = 0; i < sec->nsubsec; i++) {
			printf
			    ("<li><a href=bbsboa?secstr=%s>%s</a>",
			     sec->subsec[i]->basestr, sec->subsec[i]->title);
		}
	} else
		printf("</script>");

	return 0;
}

int
showboardlistscript(const char *secstr)
{
#if 0
	printf("<script src=boardlistscript?secstr=%s></script>", secstr, secstr);
	return 1;
#else
	struct boardmem *(data[MAXBOARD]);
	int total;
	if (*secstr == '*') {
		total=listmybrd(data);
	} else {
		total=makeboardlist(getsectree(secstr), data);
	}
	printf("<script>var boardlistscript=");
	boardlistscript(data, total);
	printf("</script>");
	return total;
#endif
}

int
showboardlist(const char *secstr)
{
	char var[20];
	int total;
	if(*secstr=='*')
		strcpy(var, "boardlistmybrd");
	else
		snprintf(var, sizeof(var), "boardlist%s", secstr); 
	printf("<table width=100%%><tr><td><div id=BL%s name=BL%s></div></td>",
	       secstr, secstr);
	printf("<td valign=top><div id=BI%s name=BI%s></div></td></tr></table>",
	       secstr, secstr);
	total = showboardlistscript(secstr);
	if(total==0)
		return 0;
	printf("<script>var %s=boardlistscript;</script>", var);
	printf
	    ("<script>document.getElementById('BL%s').innerHTML=fullBoardList(%s);",
	     secstr, var);
	printf
	    ("document.getElementById('BI%s').innerHTML=boardIndex(%s);</script>",
	     secstr, var);
	return total;
}

int
showsecmanager(const struct sectree *sec)
{
	struct secmanager *secm;
	int i;
	if (!sec->basestr[0] || !(secm = getsecm(sec->basestr)) || !secm->n)
		return -1;
	printf("区长:");
	for (i = 0; i < secm->n; i++) {
		printf(" <a href=qry?U=%s>%s</a>", secm->secm[i],
		       secm->secm[i]);
	}
	return 0;
}
