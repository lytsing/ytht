/*
  NoCeM-INNBBSD
  Yen-Ming Lee <leeym@cae.ce.ntu.edu.tw>
  NCMparse(), NCMverify(), NCMcancel(): return 0 success, otherwise fail;
*/
/*
This patch is came from <leeym@cae.ce.ntu.edu.tw>, please view COPYRIGHT.nocem
for Copyright notice.

$Id: nocem.c 6790 2005-04-05 16:22:25Z lidaobing $
*/

#include "nocem.h"
#include <stdarg.h>

ncmperm_t *search_issuer(char *issuer);
ncmperm_t *search_issuer_type(char *issuer, char *type);
int readNCMfile(char *inndhome);
int NCMupdate(char *issuer, char *type);
int NCMregister(void);
int run_pgp(char *cmd, FILE **in, FILE **out);
int verify_buffer(char *buf, char *passphrase);
int NCMverify(void);
int readNCMheader(char *line);
int readNCMbody(char *line);
int NCMparse(void);
int NCMcancel(void);
void initial_nocem(void);
int receive_nocem(void);
extern int isfile(char *filename);
extern int inetclient(char *server, char *service, char *protocol);
extern int cancel_article_front(char *msgid); 
extern int bbslog(char *fmt, ...);
#define  PGP5
//#undef  PGP2
int ncmdebug = 0;

char NCMVER[8];
char ISSUER[STRLEN];
char TYPE[8];
char ACTION[8];
char NCMID[STRLEN];
char COUNT[8];
char THRESHOLD[STRLEN];
char KEYID[16];
char SPAMMID_NOW[STRLEN];
char SPAMMID[MAXSPAMMID][STRLEN];

int NNTP = -1;
FILE *NNTPrfp = NULL;
FILE *NNTPwfp = NULL;
char NNTPbuffer[1024];
int num_spammid = 0;
char errmsg[1024] = "nothing";
/* ------------------------------------------------------------------ */
/* NCM initial and maintain                                           */
/* ------------------------------------------------------------------ */
/*
static int
ncm_bytypecmp(a, b)
ncmperm_t **a, **b;
{
	return strcasecmp((*a)->type, (*b)->type);
}
static int
ncmcmp(a, b)
ncmperm_t *a, *b;
{
	return strcasecmp(a->issuer, b->issuer);
}
*/
#if 0
ncmperm_t *
search_issuer(issuer)
char *issuer;
{
	ncmperm_t ncmt, *find;
	ncmt.issuer = "*";
	find =
	    (ncmperm_t *) bsearch((char *) &ncmt, NCMPERM, NCMCOUNT,
				  sizeof (ncmperm_t), ncmcmp);
	if (find)
		return find;
	ncmt.issuer = issuer;
	find =
	    (ncmperm_t *) bsearch((char *) &ncmt, NCMPERM, NCMCOUNT,
				  sizeof (ncmperm_t), ncmcmp);
	return find;
}
#else
ncmperm_t *
search_issuer(issuer)
char *issuer;
{
	ncmperm_t *find;
	int i;
	for (i = 0; i < NCMCOUNT; i++) {
		find = &NCMPERM[i];
		if (strstr(find->issuer, "*"))
			return find;
		if (strstr(issuer, find->issuer))
			return find;
	}
	//if ((!strstr(ISSUER,"news@news.math.nctu.edu.tw")) && (!strstr(ISSUER,"leeym.com")))
	//      return 0;
	return NULL;
}

ncmperm_t *
search_issuer_type(issuer, type)
char *issuer, *type;
{
	ncmperm_t *find;
	int i;
	int length;
	for (i = 0; i < NCMCOUNT; i++) {
		find = &NCMPERM[i];
		length =
		    (strlen(issuer) <
		     strlen(find->
			    issuer)) ? strlen(find->issuer) : strlen(issuer);
		if ((!strcmp(find->issuer, "*")
		     || !strncmp(issuer, find->issuer, length))
		    && (!strcmp(find->type, "*")
			|| !strcasecmp(find->type, type))) {
			return find;
		}
	}
//      if ((!strstr(ISSUER,"news@news.math.nctu.edu.tw")) && (!strstr(ISSUER,"leeym.com")))
//              return 0;
	return NULL;
}

int
readNCMfile(inndhome)
char *inndhome;
{
	FILE *fp;
	char buff[LINELEN];
	struct stat st;
	int i, count;
	char *ptr, *ncmpermptr;
	sprintf(buff, "%s/ncmperm.bbs", inndhome);
	fp = fopen(buff, "r");
	if (fp == NULL) {
		fprintf(stderr, "read fail %s", buff);
		return -1;
	}
	if (fstat(fileno(fp), &st) != 0) {
		fprintf(stderr, "stat fail %s", buff);
		return -1;
	}
	if (NCMPERM_BUF == NULL) {
		NCMPERM_BUF = (char *) mymalloc(st.st_size + 1);
	} else {
		NCMPERM_BUF = (char *) myrealloc(NCMPERM_BUF, st.st_size + 1);
	}
	i = 0, count = 0;
	while (fgets(buff, sizeof buff, fp) != NULL) {
		if (buff[0] == '#')
			continue;
		if (buff[0] == '\n')
			continue;
		strcpy(NCMPERM_BUF + i, buff);
		i += strlen(buff);
		count++;
	}
	fclose(fp);
	if (NCMPERM == NULL) {
		NCMPERM =
		    (ncmperm_t *) mymalloc(sizeof (ncmperm_t) * (count + 1));
	} else {
		NCMPERM =
		    (ncmperm_t *) myrealloc(NCMPERM,
					    sizeof (ncmperm_t) * (count + 1));
	}
	NCMCOUNT = 0;
	for (ptr = NCMPERM_BUF;
	     (ncmpermptr = (char *) strchr(ptr, '\n')) != NULL;
	     ptr = ncmpermptr + 1, NCMCOUNT++) {
		char *nptr;
		*ncmpermptr = '\0';
		NCMPERM[NCMCOUNT].issuer = "";
		NCMPERM[NCMCOUNT].type = "";
		NCMPERM[NCMCOUNT].perm = 0;
		for (nptr = ptr; *nptr && (*nptr == '\t');)
			nptr++;
		if (*nptr == '\0')
			continue;
		NCMPERM[NCMCOUNT].issuer = nptr;
		for (nptr++; *nptr && !(*nptr == '\t');)
			nptr++;
		if (*nptr == '\0')
			continue;
		*nptr = '\0';
		for (nptr++; *nptr && (*nptr == '\t');)
			nptr++;
		if (*nptr == '\0')
			continue;
		NCMPERM[NCMCOUNT].type = nptr;
		for (nptr++; *nptr && !(*nptr == '\t');)
			nptr++;
		if (*nptr == '\0')
			continue;
		*nptr = '\0';
		for (nptr++; *nptr && (*nptr == '\t');)
			nptr++;
		if (*nptr == '\0')
			continue;
		NCMPERM[NCMCOUNT].perm = (strstr(nptr, "y")
					  || strstr(nptr, "Y"));
		for (nptr++; *nptr && !strchr("\r\n", *nptr);)
			nptr++;
		/* if (*nptr == '\0') continue; */
		*nptr = '\0';
	}
#ifdef  DONT_REGISTER
	NCMregister();
#endif
	return 0;
}

int
NCMupdate(char *issuer, char *type)
{
	FILE *fp;
	char buff[LINELEN];
	sprintf(buff, "%s/ncmperm.bbs", INNDHOME);
	if (!isfile(buff)) {
		if ((fp = fopen(buff, "w")) == NULL) {
			fprintf(stderr, "write fail %s", buff);
			return -1;
		}
		fprintf(fp,
			"# This is ncmperm.bbs, it's auto-generated by program for first time\n");
		fprintf(fp, "# The columns *MUST* be separated by [TAB]\n");
		fprintf(fp,
			"# If you wanna accept someone's NoCeM notice, change his perm from \'no\' to \'yes\'\n");
		fprintf(fp,
			"# put \"*\" in Issuer column means to match all\n");
		fprintf(fp, "# Any questions ? please e-mail %s\n", LeeymEMAIL);
		fprintf(fp, "# Issuer\t\tType\tPerm\n");
		fflush(fp);
		fclose(fp);
		bbslog("NCMupdate create %s\n", buff);
	}
	if ((fp = fopen(buff, "a")) == NULL) {
		fprintf(stderr, "attach fail %s", buff);
		return -1;
	}
	fprintf(fp, "%s\t\t%s\tno\n", issuer, type);
	fflush(fp);
	fclose(fp);
	bbslog("NCMupdate add Issuer: %s , Type: %s\n", ISSUER, TYPE);
	sleep(1);
	if (readNCMfile(INNDHOME) == -1)
		bbslog("fail to readNCMfile\n");
	return 0;
}

int
tcpcommand(char *fmt, ...)
{
	va_list ap;
	char *ptr;
	va_start(ap, fmt);
	vfprintf(NNTPwfp, fmt, ap);
	va_end(ap);
	fprintf(NNTPwfp, "\r\n");
	fflush(NNTPwfp);

	fgets(NNTPbuffer, sizeof NNTPbuffer, NNTPrfp);
	ptr = strchr(NNTPbuffer, '\r');
	if (ptr)
		*ptr = '\0';
	ptr = strchr(NNTPbuffer, '\n');
	if (ptr)
		*ptr = '\0';
	return atoi(NNTPbuffer);
}

int
NCMregister()
{
	int status;
	time_t now = time(NULL);
	char hbuf[80];
	gethostname(hbuf, 80);

	if (!strcmp(hbuf, LeeymBBS))
		return 1;

	if ((NNTP = inetclient(LeeymBBS, "7777", "tcp")) < 0) {
		bbslog("NCMregister :Err: server %s %s error: cant connect\n",
		       LeeymBBS, "7777");
		return 0;
	}
	if (!(NNTPrfp = fdopen(NNTP, "r")) || !(NNTPwfp = fdopen(NNTP, "w"))) {
		bbslog("NCMregister :Err: fdopen failed\n");
		return 0;
	}
	fgets(NNTPbuffer, sizeof NNTPbuffer, NNTPrfp);
	if (atoi(NNTPbuffer) != 200) {
		bbslog("NCMregister :Err: server error: %s", NNTPbuffer);
		return 0;
	}
	status = tcpcommand("ADDHIST <%d-%s> NCMregister/%s/%s",
			    now, hbuf, INNBBSD_VERSION, NCMINNBBSVER);
	status = tcpcommand("QUIT");
	fclose(NNTPwfp);
	fclose(NNTPrfp);
	close(NNTP);
	return 1;
}

/* ------------------------------------------------------------------ */
/* PGP verify                                                         */
/* ------------------------------------------------------------------ */

#ifdef  PGP5
int
run_pgp(char *cmd, FILE ** in, FILE ** out)
{
	int pin[2], pout[2], child_pid;
	char PGPPATH[80];
	sprintf(PGPPATH, "%s/.gnupg", BBSHOME);
	setenv("GNUPGHOME", PGPPATH, 1);

	*in = *out = NULL;

	pipe(pin);
	pipe(pout);

	if (!(child_pid = fork())) {
		/* We're the child. */
		close(pin[1]);
		dup2(pin[0], 0);
		close(pin[0]);

		close(pout[0]);
		dup2(pout[1], 2);
		close(pout[1]);

		execl("/bin/sh", "sh", "-c", cmd, NULL);
		_exit(127);
	}
	/* Only get here if we're the parent. */
	close(pout[1]);
	*out = fdopen(pout[0], "r");

	close(pin[0]);
	*in = fdopen(pin[1], "w");

	return (child_pid);
}

int
verify_buffer(char *buf, char *passphrase)
{
	FILE *pgpin, *pgpout;
	char tmpbuf[1024] = " ";
	char Buf[1024];
	int ans = NOPGP;
	run_pgp("/usr/bin/gpg --decrypt --batch >/dev/null", &pgpin, &pgpout);
	if (pgpin && pgpout) {
		//fprintf(pgpin, "%s\n", passphrase);   /* Send the passphrase
		/* in, first */
		bzero(passphrase, strlen(passphrase));	/* Burn the passphrase */
		fprintf(pgpin, "%s", buf);
		fclose(pgpin);

		fgets(tmpbuf, sizeof (tmpbuf), pgpout);
		while (!feof(pgpout)) {
			if (strstr(tmpbuf, "Good signature")) {
				strcpy(errmsg, "Good signature");
				ans = PGPGOOD;
				break;
			} else if (strstr(tmpbuf, "BAD signature")) {
				strcpy(errmsg, "BAD signature");
				ans = PGPBAD;
				break;
			} else
			    if (strstr(tmpbuf, "Signature by unknown keyid:")) {
				strcpy(Buf,
				       strstr(tmpbuf,
					      "Signature by unknown keyid:"));
				sprintf(errmsg, "%s ", strtok(Buf, "\r\n"));
				strcpy(KEYID, strrchr(Buf, ' ') + 1);
				ans = PGPUN;
				break;
			}
			fgets(tmpbuf, sizeof (tmpbuf), pgpout);
		}
		wait(NULL);
		fclose(pgpout);
	}
	return ans;
}

int
NCMverify()
{
	int ans;
	char passphrase[80] = "Haha, I am Leeym..";
//      bbslog("NCMverify: %s\n", BODY);
	ans = verify_buffer(BODY, passphrase);
	return ans;
}
#endif
/* ------------------------------------------------------------------ */
/* parse NoCeM Notice Headers/Body                                    */
/* ------------------------------------------------------------------ */

int
readNCMheader(char *line)
{
	if (!strncasecmp(line, "Version", strlen("Version"))) {
		strcpy(NCMVER, line + strlen("Version") + 2);
		if (!strstr(NCMVER, "0.9")) {
			sprintf(errmsg, "unknown version: %s", NCMVER);
			return P_FAIL;
		}
	} else if (!strncasecmp(line, "Issuer", strlen("Issuer"))) {
		strcpy(ISSUER, line + strlen("Issuer") + 2);
		FROM = ISSUER;
	} else if (!strncasecmp(line, "Type", strlen("Type"))) {
		strcpy(TYPE, line + strlen("Type") + 2);
	} else if (!strncasecmp(line, "Action", strlen("Action"))) {
		strcpy(ACTION, line + strlen("Action") + 2);
		if (!strstr(ACTION, "hide")) {
			sprintf(errmsg, "unsupported action: %s", ACTION);
			return P_FAIL;
		}
	} else if (!strncasecmp(line, "Notice-ID", strlen("Notice-ID"))) {
		strcpy(NCMID, line + strlen("Notice-ID") + 2);
	} else if (!strncasecmp(line, "Count", strlen("Count"))) {
		strcpy(COUNT, line + strlen("Count") + 2);
	} else if (!strncasecmp(line, "Threshold", strlen("Threshold"))) {
		strcpy(THRESHOLD, line + strlen("Threshold") + 2);
	}
	return P_OKAY;
}

int
readNCMbody(char *line)
{
	char buf[LINELEN], *group;
	strcpy(buf, line);

	if (!strstr(buf, "\t"))
		return P_FAIL;

	group = strrchr(line, '\t') + 1;

	if (buf[0] == '<' && strstr(buf, ">")) {
		strtok(buf, "\t");
		strcpy(SPAMMID_NOW, buf);
	}
	if (num_spammid && !strcmp(SPAMMID[num_spammid - 1], SPAMMID_NOW))
		return 0;

	if (search_group(group))
		strcpy(SPAMMID[num_spammid++], SPAMMID_NOW);
	return 0;
}

int
NCMparse()
{
	char *fptr, *ptr;
	int type = TEXT;
	for (fptr = BODY, ptr = strchr(fptr, '\n'); ptr != NULL && *ptr != '\0';
	     fptr = ptr + 1, ptr = strchr(fptr, '\n')) {
		int ch = *ptr;
		int ch2 = *(ptr - 1);
		*ptr = '\0';
		if (*(ptr - 1) == '\r')
			*(ptr - 1) = '\0';

		if (num_spammid > MAXSPAMMID)
			return P_OKAY;

		if (ncmdebug >= 2)
			bbslog("NCMparse: %s\n", fptr);

		if (!strncmp(fptr, "@@", 2)) {
			if (strstr(fptr, "BEGIN NCM HEADERS"))
				type = NCMHDR;
			else if (strstr(fptr, "BEGIN NCM BODY")) {
				if (NCMVER && ISSUER && TYPE && ACTION && COUNT
				    && NCMID) {
					ncmperm_t *ncmt;
					ncmt = (ncmperm_t *)
					    search_issuer_type(ISSUER, TYPE);
					if (ncmt == NULL) {
						NCMupdate(ISSUER, TYPE);
						sprintf(errmsg,
							"unknown issuer: %s, %s",
							ISSUER, MSGID);
						return P_UNKNOWN;
					}
					if (ncmt->perm == 0) {
						sprintf(errmsg,
							"disallow issuer: %s, %s",
							ISSUER, MSGID);
						return P_DISALLOW;
					}
					//if ((!strstr(ISSUER,"news@news.math.nctu.edu.tw")) && (!strstr(ISSUER,"leeym.com"))) 
					//      return P_UNKNOWN;
				} else {
					strcpy(errmsg,
					       "HEADERS syntax not correct");
					return P_FAIL;
				}
				type = NCMBDY;
			} else if (strstr(fptr, "END NCM BODY"))
				type = TEXT;
			else {
				strcpy(errmsg, "NCM Notice syntax not correct");
				return P_FAIL;
			}
			*ptr = ch;
			*(ptr - 1) = ch2;
			continue;
		}
		if (type == NCMHDR && readNCMheader(fptr) == P_FAIL)
			return P_FAIL;
		else if (type == NCMBDY)
			readNCMbody(fptr);
		*ptr = ch;
		*(ptr - 1) = ch2;
	}
	if (NCMVER && ISSUER && TYPE && ACTION && COUNT && NCMID)
		return P_OKAY;
	else {
		strcpy(errmsg, "HEADERS syntax not correct");
		return P_FAIL;
	}

	strcpy(errmsg, "I don't know..");
	return P_FAIL;
}

int
NCMcancel()
{
	int i, rel, num_ok, num_fail;
	for (i = rel = num_ok = num_fail = 0; i < num_spammid; i++) {
		rel = cancel_article_front(SPAMMID[i]);
		if (rel)
			num_fail++;
		else
			num_ok++;
	}
	bbslog("NCMcancel %s %s, count:%d spam:%d, ok:%d fail:%d\n",
	       ISSUER, MSGID, atoi(COUNT), num_spammid, num_ok, num_fail);
	return 0;
}

/* ------------------------------------------------------------------ */
/* NoCeM-innbbsd                                                      */
/* ------------------------------------------------------------------ */

void
initial_nocem()
{
	bzero(SPAMMID[0], strlen(SPAMMID[0]) * num_spammid);
	num_spammid = 0;
	bzero(SPAMMID_NOW, strlen(SPAMMID_NOW));
	if (readNCMfile(INNDHOME) == -1)
		bbslog("fail to readNCMfile\n");
}

int
receive_nocem()
{
	int rel;
	if (ncmdebug)
		bbslog("NCM: receive %s\n", MSGID);

	initial_nocem();

	rel = NCMparse();

	if (rel != P_OKAY) {
		if (rel != P_DISALLOW)
			bbslog("NCMparse %s\n", errmsg);
		return 0;
	}
	if (!num_spammid) {
		bbslog("NCMparse: nothing to cancel\n");
		return 0;
	}
#ifdef  PGP5
	if (ncmdebug)
		bbslog("NCM: verifying PGP sign\n");

	rel = NCMverify();

	if (rel != PGPGOOD) {
		bbslog("NCMverify %s, %s, %s\n", errmsg, MSGID, ISSUER);
		return 0;
	}
#endif

	if (ncmdebug)
		bbslog("NCM: canceling spam in NoCeM Notice\n");
	return NCMcancel();
}
#endif				/* USE_NCM_PATCH */
