# include "innbbsconf.h"
# include "bbslib.h"
# include "bbsconfig.h"
# include <stdarg.h>
#include <sys/mman.h>
#include <stdlib.h>
#ifndef AIX
#  include <sys/fcntl.h>
#endif

#include "daemon.h"
#include "nntp.h"
#include "externs.h"
#include "lang.h"
/*
 * TODO 1. read newsfeeds.bbs, read nodelist.bbs, read bbsname.bbs 2. scan
 * new posts and append to .link 3. rename .link to .send (must lock) 4.
 * start to send .send out and append not sent to .link
 * 
 * 5. node.LOCK (with pid) 6. log articles sent
 */

#ifndef MAXBUFLEN
#define MAXBUFLEN 256
#endif
typedef struct Over_t {
	time_t mtime;
	char date[MAXBUFLEN];
	char nickname[MAXBUFLEN];
	char subject[MAXBUFLEN];
	char from[MAXBUFLEN];
	char msgid[MAXBUFLEN];
	char site[MAXBUFLEN];
	char board[MAXBUFLEN];
} linkoverview_t;

typedef struct SendOver_t {
	char *board, *filename, *group, *from, *subject;
	char *outgoingtype, *msgid, *path;
	char *date, *control;
	time_t mtime;
} soverview_t;

typedef struct Stat_t {
	int localsendout;
	int localfailed;
	int remotesendout;
	int remotefailed;
} stat_t;

static stat_t *BBSLINK_STAT;

static int NoAction = 0;
static int Verbose = 0;
static int VisitOnly = 0;
static int NoVisit = 0;
static char *DefaultFeedSite = "";
static int KillFormerBBSLINK = 0;

extern char *SITE;
extern char *GROUPS;

char NICKNAME[MAXBUFLEN];

char DATE_BUF[MAXBUFLEN];
extern char *DATE;

char FROM_BUF[MAXBUFLEN];
extern char *FROM;

char MYADDR[MAXBUFLEN];
char MYSITE[MAXBUFLEN];
char SITE_BUF[MAXBUFLEN];
char *SITE_PTR = MYSITE;

char SUBJECT_BUF[MAXBUFLEN];
extern char *SUBJECT;

char MSGID_BUF[MAXBUFLEN];
extern char *MSGID;

char LINKPROTOCOL[MAXBUFLEN];
int LINKPORT;
char ORGANIZATION[MAXBUFLEN];
char NEWSCONTROL[MAXBUFLEN];
char NEWSAPPROVED[MAXBUFLEN];
char NNTPHOST_BUF[MAXBUFLEN];
extern char *NNTPHOST;
char PATH_BUF[MAXBUFLEN];
extern char *PATH;

char CONTROL_BUF[MAXBUFLEN];
extern char *CONTROL;

char *BODY, *HEAD;

int USEIHAVE = 1;
int USEPOST = 0;
int USEDATA = 0;
int FEEDTYPE = ' ';

int NNTP = -1;
FILE *NNTPrfp = NULL;
FILE *NNTPwfp = NULL;
char NNTPbuffer[1024];
static char *NEWSFEED;
static char *REMOTE = "REMOTE";
static char *LOCAL = "LOCAL";

static int FD, FD_SIZE;
static char *FD_BUF = NULL, *FD_BUF_FILTER = NULL;
static char *FD_END;

extern char *fileglue(char *fmt, ...);
extern int isfile(char *);
extern newsfeeds_t *search_board(char *board);
extern void bbslog(char *,...);
extern nodelist_t *search_nodelist_bynode(char *node);
extern long filesize(char *);
extern char *filterdata(newsfeeds_t *arp, int direction, char *indata, char **filterdata);
extern int isMsgTxt(TxtClass *txtclass, char *orgtxt);
extern char *parselocaltime(char *date_buf);
extern int inetclient(char *server, char *service, char *protocol);
void close_link();
extern int iszerofile(char *);
extern void initial_lang();

void
bbslink_un_lock(file)
char *file;
{
	char *lockfile = fileglue("%s.LOCK", file);

	if (isfile(lockfile))
		unlink(lockfile);
}

int
bbslink_get_lock(file)
char *file;
{
	int lockfd;
	char LockFile[MAXPATHLEN];

	strncpy(LockFile, (char *) fileglue("%s.LOCK", file), sizeof LockFile);
	if ((lockfd = open(LockFile, O_RDONLY)) >= 0) {
		char buf[10];
		int pid;

		if (read(lockfd, buf, sizeof buf) > 0 &&
		    (pid = atoi(buf)) > 0 && kill(pid, 0) == 0) {
			if (KillFormerBBSLINK) {
				kill(pid, SIGTERM);
				unlink(LockFile);
			} else {
				fprintf(stderr,
					"another process [%d] running\n", pid);
				return 0;
			}
		} else {
/*      fprintf(stderr, "no process [%d] running, but lock file existed, unlinked\n", pid); */
			unlink(LockFile);
		}
		close(lockfd);
	}

	if ((lockfd = open(LockFile, O_RDWR | O_CREAT | O_EXCL, 0644)) < 0) {
		fprintf(stderr,
			"lock %s error: another bbslink process running\n",
			LockFile);
		return 0;
	} else {
		char buf[10];

		sprintf(buf, "%-.8d\n", getpid());
		write(lockfd, buf, strlen(buf));
		close(lockfd);
		return 1;
	}
}

int
tcpcommand(char *fmt, ...)
{
	va_list ap;
	char *ptr;

	va_start(ap, fmt);
	vfprintf(NNTPwfp, fmt, ap);
	fprintf(NNTPwfp, "\r\n");
	fflush(NNTPwfp);

	fgets(NNTPbuffer, sizeof NNTPbuffer, NNTPrfp);
	ptr = strchr(NNTPbuffer, '\r');
	if (ptr)
		*ptr = '\0';
	ptr = strchr(NNTPbuffer, '\n');
	if (ptr)
		*ptr = '\0';
	va_end(ap);
	return atoi(NNTPbuffer);
}

char *
tcpmessage()
{
	char *ptr;

	ptr = strchr(NNTPbuffer, ' ');
	if (ptr)
		return ptr;
	return NNTPbuffer;
}

int
read_article(lover, filename, userid)
linkoverview_t *lover;
char *filename, *userid;
{
	struct stat st;

	if (stat(filename, &st) != 0)
		return 0;
	lover->mtime = st.st_mtime;
	return 1;
}

void
save_outgoing(sover, filename, userid, poster, mtime)
soverview_t *sover;
char *filename, *userid, *poster;
time_t mtime;
{
	newsfeeds_t *nf;
	char *group, *server, *serveraddr;
	char *board;
	char *ptr1, *ptr2;

	board = sover->board;

	PATH = MYBBSID;
	nf = (newsfeeds_t *) search_board(board);
	if (nf == NULL) {
		bbslog("<bbslink> save_outgoing: No such board %s\n", board);
		return;
	} else {
		group = nf->newsgroups;
		server = nf->path;
		bbslog("<bbslink> save_outgoing:group %s   server %s\n", group,
		       server);
	}
	if (!server || !*server) {
		sprintf(PATH_BUF, "%.*s (local)", sizeof PATH_BUF - 9, MYBBSID);
		PATH = PATH_BUF;
		serveraddr = "";
		sover->path = PATH;
	}
	for (ptr1 = server; ptr1 && *ptr1;) {
		nodelist_t *nl;
		char savech;

		for (; *ptr1 && isspace(*ptr1); ptr1++) ;
		if (!*ptr1)
			break;
		for (ptr2 = ptr1; *ptr2 && !isspace(*ptr2); ptr2++) ;
		savech = *ptr2;
		*ptr2 = '\0';
		nl = (nodelist_t *) search_nodelist_bynode(ptr1);
		*ptr2 = savech;
		ptr1 = ptr2++;
		if (nl == NULL) {
			continue;
		}
		/* if (nl->feedfp == NULL) continue; */

		if (nl->host && *nl->host) {
			if (nl->feedfp == NULL) {
				nl->feedfp =
				    fopen(fileglue
					  ("%s/%s.link", INNDTMPFS, nl->node),
					  "a");
				if (nl->feedfp == NULL) {
					bbslog
					    ("<save outgoing> append failed for %s/%s.link",
					     INNDTMPFS, nl->node);
				}
			}
			if (nl->feedfp != NULL) {
				flock(fileno(nl->feedfp), LOCK_EX);
				fprintf(nl->feedfp, "%s\t%s\t%s\t%ld\t%s\t%s\n",
					sover->board, filename, group, mtime,
					FROM, sover->subject);
				fflush(nl->feedfp);
				flock(fileno(nl->feedfp), LOCK_UN);
			}
		}
		if (savech == '\0')
			break;
	}
}

/* process_article() read_article() save_outgoing() save_article() */

void
process_article(board, filename, userid, nickname, subject)
char *board, *filename, *userid, *nickname, *subject;
{
	char *filepath;
	char poster[MAXBUFLEN];
	soverview_t sover;

	if (!*userid) {
		return;
	} else if (!subject || !*subject) {
		subject = "����";
	}
	filepath = fileglue("%s/boards/%s/%s", BBSHOME, board, filename);
	if (isfile(filepath)) {
		linkoverview_t lover;

		if (read_article(&lover, filepath, userid)) {
			strncpy(FROM_BUF,
				fileglue("%s.bbs@%s (%s)", userid, MYADDR,
					 nickname), sizeof FROM_BUF);
			FROM_BUF[sizeof(FROM_BUF) - 1] = 0;
			FROM = FROM_BUF;
			sover.from = FROM;
			sover.board = board;
			sover.subject = subject;
			PATH = MYBBSID;
			sover.path = MYBBSID;
			sover.date = lover.date;
			sover.mtime = lover.mtime;
			if (!VisitOnly) {
				save_outgoing(&sover, filename, userid, poster,
					      lover.mtime);
			}
		}
	}
}

char *
baseN(val, base, len)
int val, base, len;
{
	int n;
	static char str[MAXBUFLEN];
	int index;

	for (index = len - 1; index >= 0; index--) {
		n = val % base;
		val /= base;
		if (n < 10) {
			n += '0';
		} else if (n < 36) {
			n += 'A' - 10;
		} else if (n < 62) {
			n += 'a' - 36;
		} else {
			n = '_';
		}
		str[index] = n;
	}
	str[len] = '\0';
	return str;
}

char *
hash_value(str)
char *str;
{
	int val, n;
	char *ptr;

	if (*str)
		ptr = str + strlen(str) - 1;
	else
		ptr = str;
	val = 0;
	while (ptr >= str) {
		n = *ptr;
		val = (val + n * 0x100) ^ n;
		ptr--;
	}
	return baseN(val, 64, 3);
}

/* process_cancel() save_outgoing() hash_value(); baseN(); ascii_date(); */

int
read_outgoing(sover)
soverview_t *sover;
{
	char *board, *filename, *group, *from, *subject, *outgoingtype, *msgid,
	    *path;
	char *buffer, *bufferp;
	char *hash;
	char times[MAXBUFLEN];
	time_t mtime;
	char *indata, *fd_buf;
	int fd_size;

	board = sover->board;
	filename = sover->filename;
	group = sover->group;
	mtime = sover->mtime;
	from = sover->from;
	subject = sover->subject;
	outgoingtype = sover->outgoingtype;
	msgid = sover->msgid;
	path = sover->path;
	if (Verbose) {
		printf("<read_outgoing> %s:%s:%s\n", board, filename, group);
		printf("  => %ld:%s\n", mtime, from);
		printf("  => %s\n", subject);
		printf("  => %s:%s\n", outgoingtype, msgid);
		printf("  => %s\n", path);
	}
	if (NEWSFEED == LOCAL) {
		char *end = strrchr(filename, '.');

		if (end)
			*end = '\0';
		strncpy(times, baseN(atol(filename + 2), 48, 6), sizeof times);
		times[sizeof(times) - 1] = 0;
		if (end)
			*end = '.';
		hash = hash_value(fileglue("%s.%s", filename, board));
		sprintf(MSGID_BUF, "%s$%s@%s", times, hash, MYADDR);
	} else {
		strncpy(MSGID_BUF, msgid, sizeof MSGID_BUF);
	}
	sover->msgid = MSGID;
	if (mtime == -1) {
		static char BODY_BUF[MAXBUFLEN];

		strncpy(BODY_BUF, fileglue("%s\r\n", subject), sizeof BODY_BUF);
		BODY_BUF[sizeof(BODY_BUF) - 1] = 0;
		BODY = BODY_BUF;
		sprintf(SUBJECT_BUF, "cmsg cancel <%s>", MSGID);
		SUBJECT = SUBJECT_BUF;
		sprintf(CONTROL_BUF, "cancel <%s>", MSGID);
		CONTROL = CONTROL_BUF;
		strncpy(MSGID_BUF, fileglue("%d.%s", getpid(), MSGID_BUF),
			sizeof MSGID_BUF);
		MSGID_BUF[sizeof(MSGID_BUF) - 1] = 0;
		sprintf(DATE_BUF, "%s", ascii_date(time(NULL)));
		DATE = DATE_BUF;
		sover->subject = SUBJECT;
		sover->control = CONTROL;
		sover->msgid = MSGID;
		sover->date = DATE;
	} else {
		sover->control = CONTROL;
		sover->date = DATE_BUF;
		DATE = DATE_BUF;
		*CONTROL = '\0';
		sprintf(DATE, "%s", ascii_date((mtime)));
		if (NEWSFEED == LOCAL && !NoAction) {
			SITE = SITE_PTR;
			PATH = MYBBSID;
			GROUPS = group;
		}
		BODY = "";
		FD =
		    open(fileglue("%s/boards/%s/%s", BBSHOME, board, filename),
			 O_RDONLY);
		if (FD < 0) {
			if (Verbose)
				printf(" !! can't open %s/boards/%s/%s\n",
				       BBSHOME, board, filename);
			else
				fprintf(stderr, "can't open %s/boards/%s/%s\n",
					BBSHOME, board, filename);
			return -1;
		}

		FD_SIZE =
		    filesize(fileglue
			     ("%s/boards/%s/%s", BBSHOME, board, filename));
		fd_size = FD_SIZE;
#ifdef  FILTER
		fd_size += 266 * 3;
#endif
		if (FD_BUF == NULL) {
			FD_BUF = (char *) mymalloc(fd_size + 1);
		} else {
			FD_BUF = (char *) myrealloc(FD_BUF, fd_size + 1);
		}
		fd_buf = FD_BUF;
#ifdef FILTER
		if (sover->from && *sover->from) {
			sprintf(fd_buf, "From: %.256s\n", sover->from);
			fd_buf += strlen(fd_buf);
		}
		if (sover->subject && *sover->subject) {
			sprintf(fd_buf, "Subject: %.256s\n", sover->subject);
			fd_buf += strlen(fd_buf);
		}
		if (NEWSFEED == LOCAL && MYSITE && *MYSITE) {
			sprintf(fd_buf, "Organization: %.256s\n", MYSITE);
			fd_buf += strlen(fd_buf);
		}
#endif

		FD_END = fd_buf + FD_SIZE;
		*FD_END = '\0';

		read(FD, fd_buf, FD_SIZE);
		if (Verbose) {
			printf("<read in> %s/boards/%s/%s\n", BBSHOME, board,
			       filename);
		}
		indata = FD_BUF;

#ifdef FILTER
		{
			/* print Subject, From, Organization to FD_BUF,
			   filter,
			   parse out again
			 */
			newsfeeds_t *nf;
			char *outdata;
			outdata = NULL;
			nf = (newsfeeds_t *) search_board(board);
			if (nf != NULL) {
				outdata =
				    (char *) filterdata(nf, 1, FD_BUF,
							&FD_BUF_FILTER);
			}
			if (outdata)
				indata = outdata;
			/*indata = (char*)processfilter(FD_BUF, FD_BUF_FILTER, 1); */
		}
#endif

		*ORGANIZATION = '\0';
		*NEWSCONTROL = '\0';
		*NEWSAPPROVED = '\0';
		*NNTPHOST_BUF = '\0';
		NNTPHOST = NULL;

		for (buffer = indata, bufferp = strchr(buffer, '\n');
		     buffer && *buffer; bufferp = strchr(buffer, '\n')) {
			if (bufferp)
				*bufferp = '\0';
			if (*buffer == '\0') {
				break;
			}
			/* printf("get buffer %s\n", buffer); */
#ifdef FILTER
			if (strncmp(buffer, "Subject: ", 9) == 0) {
				strncpy(SUBJECT_BUF, buffer + 9,
					sizeof SUBJECT_BUF);
				sover->subject = SUBJECT_BUF;
			} else if (strncmp(buffer, "From: ", 6) == 0) {
				strncpy(FROM_BUF, buffer + 6, sizeof FROM_BUF);
				sover->from = FROM_BUF;
			} else if (strncmp(buffer, "Organization: ", 14) == 0) {
				if (NEWSFEED == LOCAL)
					strncpy(SITE_BUF, buffer + 14,
						sizeof SITE_BUF);
				SITE_PTR = SITE_BUF;
			} else
#endif
			if (NEWSFEED == REMOTE) {
				if (strncmp(buffer, "Date:      ", 11) == 0) {
					strcpy(DATE_BUF, buffer + 11);
					DATE = DATE_BUF;
#ifndef FILTER
				} else if (strncmp(buffer, OrganizationTxt, 8)
					   == 0) {
#else
				} else
				    if (isMsgTxt(OrganizationTxtClass, buffer))
				{
#endif
					char *m, *n;

					m = strrchr(buffer, '(');
					n = strrchr(buffer, ')');
					if (m && n) {
						strncpy(DATE_BUF, m + 1,
							n - m - 1);
						DATE_BUF[n - m - 2] = '\0';
						DATE = DATE_BUF;
						strncpy(ORGANIZATION,
							buffer + 8,
							m - 8 - buffer - 1);
						ORGANIZATION[m - 8 - buffer -
							     2] = '\0';
					}
				} else if (strncmp(buffer, "Control: ", 9) == 0) {
					strncpy(NEWSCONTROL, buffer + 9, sizeof NEWSCONTROL);
					NEWSCONTROL[sizeof(NEWSCONTROL) - 1] = 0;
				}
					else
				    if (strncmp(buffer, "Approved: ", 10) ==
					0) {
					strncpy(NEWSAPPROVED, buffer + 10, sizeof NEWSAPPROVED);
					NEWSAPPROVED[sizeof(NEWSAPPROVED) - 1] = 0;
				} else if (strncmp(buffer, "��  ��: ", 8) == 0) {
					strncpy(NNTPHOST_BUF, buffer + 8, sizeof NNTPHOST_BUF);
					NNTPHOST_BUF[sizeof(NNTPHOST_BUF) - 1] = 0;
					NNTPHOST = NNTPHOST_BUF;
				}
			}
			if (bufferp) {
				*bufferp = '\n';
				buffer = bufferp + 1;
			} else {
				break;
			}
		}
		if (NEWSFEED == REMOTE) {
			char *datevalue;
			datevalue = (char *) parselocaltime(DATE_BUF);
			strncpy(DATE_BUF, datevalue, sizeof DATE_BUF);
			DATE_BUF[sizeof(DATE_BUF) - 1] = 0;
		}
		if (bufferp) {
			BODY = bufferp + 1;
		} else
			BODY = "";
		if (bufferp)
			for (buffer = bufferp + 1, bufferp =
			     strchr(buffer, '\n'); buffer && *buffer;
			     bufferp = strchr(buffer, '\n')) {
				if (bufferp)
					*bufferp = '\0';
				/* printf("get line (%s)\n", buffer); */
				/* if( strcmp(buffer,".")==0 ) { buffer[1]='.'; buffer[2]='\0'; } */
				if (NEWSFEED == REMOTE &&
				    strncmp(NEWSCONTROL, "cancel", 5) == 0 &&
				    strncmp(buffer, "------------------",
					    18) == 0) {
					break;
				}
				/* $BODY[ @BODY ] = "$_\r\n"; */
				if (bufferp) {
					*bufferp = '\n';
					buffer = bufferp + 1;
				} else {
					break;
				}
			}
		/* #       fprintf("BODY @BODY\n"; */
		close(FD);
	}
	if (Verbose) {
		printf("<read_outgoing> %s:%s:%s\n", sover->board,
		       sover->filename, sover->group);
		printf("  => %ld:%s\n", mtime, sover->from);
		printf("  => %s\n", sover->subject);
		printf("  => %s:%s\n", sover->outgoingtype, sover->msgid);
		printf("  => %s\n", sover->path);
	}
	return 0;
}

void
openfeed(node)
nodelist_t *node;
{
	if (node->feedfp == NULL) {
		node->feedfp =
		    fopen(fileglue("%s/%s.link", INNDTMPFS, node->node), "a");
	}
}

void
queuefeed(node, textline)
nodelist_t *node;
char *textline;
{
	openfeed(node);
	if (node->feedfp != NULL) {
		flock(fileno(node->feedfp), LOCK_EX);
		fprintf(node->feedfp, "%s", textline);
		fflush(node->feedfp);
		flock(fileno(node->feedfp), LOCK_UN);
	}
}

int
post_article(node, site, sover, textline)
nodelist_t *node;
char *site;
soverview_t *sover;
char *textline;
{
	int status;
	char *filename = sover->filename;
	char *msgid = sover->msgid;
	char *board = sover->board;
	char *bodyp, *body;
	int attached = 0;

	if (Verbose)
		fprintf(stdout, "<post_article> %s %s %s\n", site, filename,
			msgid);
	if (NoAction && Verbose) {
		printf("  ==>%s\n", sover->path);
		printf("  ==>%s:%s\n", sover->from, sover->group);
		printf("  ==>%s:%s\n", sover->subject, sover->date);
		body = BODY;
		bodyp = strchr(body, '\n');
		if (bodyp)
			*bodyp = '\0';
		printf("  ==>%s\n", body);
		if (bodyp)
			*bodyp = '\n';
		if (bodyp) {
			body = bodyp + 1;
			bodyp = strchr(body, '\n');
			if (bodyp)
				*bodyp = '\0';
			printf("  ==>%s\n", body);
			if (bodyp)
				*bodyp = '\n';
		}
	}
	if (NoAction)
		return 1;
	if (NEWSFEED == REMOTE) {
		fprintf(NNTPwfp, "Path: %s\r\n", sover->path);
		fprintf(NNTPwfp, "From: %s\r\n", sover->from);
		fprintf(NNTPwfp, "Newsgroups: %s\r\n", sover->group);
		fprintf(NNTPwfp, "Subject: %s\r\n", sover->subject);
		/* #       fprintf( NNTPwfp,"Post with subject ($subject)\n"); */
		fprintf(NNTPwfp, "Date: %s\r\n", sover->date);
		if (*ORGANIZATION)
			fprintf(NNTPwfp, "Organization: %s\r\n", ORGANIZATION);
		fprintf(NNTPwfp, "Message-ID: <%s>\r\n", sover->msgid);
		fprintf(NNTPwfp, "Mime-Version: 1.0\r\n");
		fprintf(NNTPwfp,
			"Content-Type: text/plain; charset=\"gb2312\"\r\n");
		fprintf(NNTPwfp, "Content-Transfer-Encoding: 8bit\r\n");

		if (*NEWSCONTROL)
			fprintf(NNTPwfp, "Control: %s\r\n", NEWSCONTROL);
		if (*NEWSAPPROVED)
			fprintf(NNTPwfp, "Approved: %s\r\n", NEWSAPPROVED);
	} else {
		fprintf(NNTPwfp, "Path: %s\r\n", MYBBSID);
		fprintf(NNTPwfp, "From: %s\r\n", sover->from);
		fprintf(NNTPwfp, "Newsgroups: %s\r\n", sover->group);
		fprintf(NNTPwfp, "Subject: %s\r\n", sover->subject);
		fprintf(NNTPwfp, "Date: %s\r\n", sover->date);
		fprintf(NNTPwfp, "Organization: %s\r\n", SITE_PTR);
		fprintf(NNTPwfp, "Message-ID: <%s>\r\n", sover->msgid);
		fprintf(NNTPwfp, "Mime-Version: 1.0\r\n");
		fprintf(NNTPwfp,
			"Content-Type: text/plain; charset=\"gb2312\"\r\n");
		fprintf(NNTPwfp, "Content-Transfer-Encoding: 8bit\r\n");

		fprintf(NNTPwfp, "X-Filename: %s/%s\r\n", sover->board,
			sover->filename);
	}
	if (NNTPHOST && *NNTPHOST && USEIHAVE)
		fprintf(NNTPwfp, "NNTP-Posting-Host: %s\r\n", NNTPHOST);
	else if (NNTPHOST && *NNTPHOST)
		fprintf(NNTPwfp, "X-Auth-From: %s\r\n", NNTPHOST);
	if (*CONTROL) {
		fprintf(NNTPwfp, "Control: %s\r\n", CONTROL);
	}
	fputs("\r\n", NNTPwfp);
	for (body = BODY, bodyp = strchr(body, '\n');
	     body && *body; bodyp = strchr(body, '\n')) {
#ifndef CANSENDATTACH
		if (!strncmp(body, "begin 644 ", 10) && bodyp) {
			char *ptr, *end;
			int endc;
			ptr = strrchr(body, '\r');
			if (ptr)
				*ptr = 0;
			*bodyp = 0;
			end = strstr(bodyp + 1, "\nend\n");
			endc = 5;
			if (!end) {
				end = strstr(bodyp + 1, "\r\nend\r\n");
				endc = 7;
			}
			if (!end) {
				end = strstr(bodyp + 1, "\r\nend\r\n");
				endc = 5;
			}
			if (end) {
				fprintf(NNTPwfp,
					"���ﱾ����һ������ %s\n����ĿǰCN BBSת��ϵͳ��ת���нϴ󸽼����ż�,���� %s BBSȥ����.\n",
					body + 10, MYBBSID);
				attached = 1;
				body = end + endc + 1;
				if (ptr)
					*ptr = '\r';
				*bodyp = '\n';
				continue;
			} else {
				fprintf(NNTPwfp,
					"���ﱾ����һ������ %s\n���ǿ��� %s BBSϵͳԭʼ�ļ���,�޷�����ת��.\n",
					body + 10, MYBBSID);
				break;
			}
		}
#endif
		if (!strncmp(body, "beginbinaryattach ", 18)) {
			if (bodyp && bodyp < FD_END && !(*(bodyp + 1))) {
				char *ptr;
				unsigned int len;
				ptr = strchr(body, '\r');
				if (ptr)
					*ptr = 0;
				*bodyp = 0;
				len = *((unsigned int *) (bodyp + 2));
				len = ntohl(len);
				if (bodyp + 5 + len < FD_END) {
					attached = 1;
#ifdef CANSENDATTACH
					fr = fmemopen(bodyp + 6, len, "r");
					if (NULL != fr) {
						uuencode(fr, NNTPwfp, len,
							 body + 18);
						fclose(fr);
					} else
						fprintf(NNTPwfp,
							"���ﱾ����һ������ %s\n���� %s BBSϵͳ�ڲ�����,�޷�����ת��.\n",
							body + 18, MYBBSID);
#else
					fprintf(NNTPwfp,
						"���ﱾ����һ������ %s\n����ĿǰCN BBSת��ϵͳ��ת���нϴ󸽼����ż�,���� %s BBS ȥ����.\n",
						body + 18, MYBBSID);
#endif
					*bodyp = '\n';
					if (ptr)
						*ptr = '\r';
					body = bodyp + 5 + len + 1;
					continue;
				} else {
					fprintf(NNTPwfp,
						"���ﱾ����һ������ %s\n���� %s BBSϵͳԭʼ�ļ���,�޷�����ת��.\n",
						body + 18, MYBBSID);
					break;
				}
			}
		}
		if (bodyp)
			*bodyp = '\0';

		fputs(body, NNTPwfp);
		if (body[0] == '.' && body[1] == '\0')
			fputs(".", NNTPwfp);
		fputs("\r\n", NNTPwfp);
		if (bodyp) {
			*bodyp = '\n';
			body = bodyp + 1;
		} else {
			break;
		}
	}
	if (attached) {
		fprintf(NNTPwfp,
			"�������ʵ�ַ��http://" MY_BBS_DOMAIN "/" SMAGIC
			"/con?B=%s&F=%s\r\n", board, filename);
	}
	/* print "send out @BODY\n"; */
	status = tcpcommand(".");
	/* 435 duplicated article 437 invalid header */

	if (USEIHAVE) {
		if (status == 235) {
			if (NEWSFEED == LOCAL) {
				bbslog("Sendout <%s> from %s/%s\n", msgid,
				       board, filename);
			}
		} else if (status == 437 || status == 435) {
			bbslog("<bbslink> :Warn: %d %s <%s>\n", status,
			       (char *) tcpmessage(), msgid);
			if (Verbose)
				printf(":Warn: %d %s <%s>\n", status,
				       (char *) tcpmessage(), msgid);
			return 0;
		} else {
			bbslog("<bbslink> :Err: %d %s of <%s>\n", status,
			       (char *) tcpmessage(), msgid);
			if (Verbose)
				printf(":Err: %d %s of <%s>\n", status,
				       (char *) tcpmessage(), msgid);
			queuefeed(node, textline);
			return 0;
		}
	} else if (USEPOST) {
		if (status == 240) {
			bbslog("Sendout <%s> from %s/%s\n", msgid, board,
			       filename);
		} else {
			bbslog("<bbslink> :Err: %d %s of <%s>\n", status,
			       (char *) tcpmessage(), msgid);
			queuefeed(node, textline);
			return 0;
		}
	} else {
		if (status == 250) {
			bbslog("<bbslink> DATA Sendout <%s> from %s/%s\n",
			       msgid, board, filename);
			if (Verbose)
				printf("<DATA Sendout> <%s> from %s/%s\n",
				       msgid, board, filename);
		} else {
			bbslog("<bbslink> :Err: %d %s of <%s>\n", status,
			       (char *) tcpmessage(), msgid);
			if (Verbose)
				printf(":Err: %d %s of <%s>\n", status,
				       (char *) tcpmessage(), msgid);
			queuefeed(node, textline);
			return 0;
		}
	}
	return 1;
}

void
process_cancel(board, filename, userid, nickname, subject)
char *board, *filename, *userid, *nickname, *subject;
{
	time_t mtime;
	soverview_t sover;

	if (!userid || !*userid) {
		return;
	}
	mtime = -1;
	strncpy(FROM_BUF, fileglue("%s.bbs@%s (%s)", userid, MYADDR, nickname),
		sizeof FROM_BUF);
	FROM_BUF[sizeof(FROM_BUF) - 1] = 0;
	FROM = FROM_BUF;
	sover.from = FROM;
	sover.board = board;
	sover.subject = subject;
	PATH = MYBBSID;
	sover.path = MYBBSID;
	save_outgoing(&sover, filename, userid, userid, -1);
}

int
open_link(hostname, hostprot, hostport)
char *hostname, *hostprot, *hostport;
{
	USEIHAVE = 1;
	USEPOST = 0;
	USEDATA = 0;
	FEEDTYPE = ' ';
	if (Verbose)
		printf("<OPEN_link> %s %s %s\n", hostname, hostprot, hostport);
	if (strncasecmp(hostprot, "IHAVE", 5) != 0) {
		USEIHAVE = 0;
		USEPOST = 1;
		if (strncasecmp(hostprot, "POST", 4) == 0) {
			USEPOST = 1;
		} else if (strncasecmp(hostprot, "DATA", 4) == 0) {
			USEPOST = 0;
			USEDATA = 1;
		}
	}

	FEEDTYPE = hostname[0];
	if (!USEDATA) {
		char *atsign;

		if (FEEDTYPE == '-' || FEEDTYPE == '+') {
			hostname = hostname + 1;
		}
		atsign = strchr(hostname, '@');
		if (atsign != NULL) {
			hostname = atsign + 1;
		}
		if (!NoAction) {
			if (Verbose)
				printf("<inetclient> %s %s\n", hostname,
				       hostport);
			if ((NNTP = inetclient(hostname, hostport, "tcp")) < 0) {
				bbslog
				    ("<bbslink> :Err: server %s %s error: cant connect\n",
				     hostname, hostport);
				if (Verbose)
					printf
					    (":Err: server %s %s error: cant connect\n",
					     hostname, hostport);
				return 0;
				/* exit( 0 ); */
				/* return; */
			}
			NNTPrfp = fdopen(NNTP, "r");
			NNTPwfp = fdopen(NNTP, "w");
			fgets(NNTPbuffer, sizeof NNTPbuffer, NNTPrfp);
			if (atoi(NNTPbuffer) != 200) {
				bbslog("<bbslink> :Err: server error: %s",
				       NNTPbuffer);
				if (Verbose)
					printf(":Err: server error: %s",
					       NNTPbuffer);
				return 0;
				/* exit( 0 ); */
			}
		} else {
			if (Verbose)
				printf("<inetclient> %s %s\n", hostname,
				       hostport);
		}
	} else {
		if (!NoAction) {
			if (Verbose)
				printf("<inetclient> localhost %s\n", hostport);
			if ((NNTP = inetclient("localhost", hostport, "tcp")) <
			    0) {
				bbslog
				    ("<bbslink> :Err: server %s port %s error: cant connect\n",
				     hostname, hostport);
				if (Verbose)
					printf
					    (":Err: server error: cant connect");
				return 0;
				/* exit( 0 ); */
				/* return; */
			}
			NNTPrfp = fdopen(NNTP, "r");
			NNTPwfp = fdopen(NNTP, "w");
			fgets(NNTPbuffer, sizeof NNTPbuffer, NNTPrfp);
			if (strncmp(NNTPbuffer, "220", 3) != 0) {
				bbslog("<bbslink> :Err: server error: %s",
				       NNTPbuffer);
				if (Verbose)
					printf(":Err: server error: %s",
					       NNTPbuffer);
				return 0;
				/* exit( 0 ); */
			}
			if (strncmp(NNTPbuffer, "220-", 4) == 0) {
				fgets(NNTPbuffer, sizeof NNTPbuffer, NNTPrfp);
			}
		} else {
			if (Verbose)
				printf("<inetclient> %s %s\n", hostname,
				       hostport);
		}
	}
	return 1;
}

int
send_outgoing(node, site, hostname, sover, textline)
nodelist_t *node;
soverview_t *sover;
char *hostname, *site;
char *textline;
{
	int status;
	char *board, *filepath, *msgid;
	int returnstatus = 0;

	board = sover->board;
	filepath = sover->filename;
	msgid = sover->msgid;

	if (Verbose)
		printf("<send_outgoing> %s:%s:%s:%s\n", site, board, filepath,
		       msgid);
	if (BODY != NULL && !NoAction) {
		if (USEIHAVE) {
			status = tcpcommand("IHAVE <%s>", msgid);
			if (status == 335) {
				returnstatus =
				    post_article(node, site, sover, textline);
			} else if (status == 435) {
				bbslog("<bbslink> :Warn: %d %s, IHAVE <%s>\n",
				       status, (char *) tcpmessage(), msgid);
				if (Verbose)
					printf(":Warn: %d %s, IHAVE <%s>\n",
					       status, (char *) tcpmessage(),
					       msgid);
				returnstatus = 0;
			} else {
				bbslog("<bbslink> :Err: %d %s, IHAVE <%s>\n",
				       status, (char *) tcpmessage(), msgid);
				if (Verbose)
					printf(":Err: %d %s, IHAVE <%s>\n",
					       status, (char *) tcpmessage(),
					       msgid);
				queuefeed(node, textline);
				returnstatus = 0;
			}
		} else if (USEPOST) {
			tcpcommand("MODE READER");
			status = tcpcommand("POST");
			if (status == 340) {
				returnstatus =
				    post_article(node, site, sover, textline);
			} else if (status == 441) {
				bbslog("<bbslink> :Warn: %d %s, POST <%s>\n",
				       status, (char *) tcpmessage(), msgid);
				if (Verbose)
					printf(":Warn: %d %s, POST <%s>\n",
					       status, (char *) tcpmessage(),
					       msgid);
				returnstatus = 0;
			} else {
				bbslog("<bbslink> :Err: %d %s, POST <%s>\n",
				       status, (char *) tcpmessage(), msgid);
				if (Verbose)
					printf(":Err: %d %s, POST <%s>\n",
					       status, (char *) tcpmessage(),
					       msgid);
				queuefeed(node, textline);
				returnstatus = 0;
			}
		} else {
			tcpcommand("HELO");
			tcpcommand("MAIL FROM: bbs");
			tcpcommand("RCPT TO: %s", hostname);
			status = tcpcommand("DATA");
			if (status == 354) {
				returnstatus =
				    post_article(node, site, sover, textline);
			} else {
				bbslog("<bbslink> :Err: %d %s, DATA <%s>\n",
				       status, (char *) tcpmessage(), msgid);
				if (Verbose)
					printf(":Err: %d %s, DATA <%s>\n",
					       status, (char *) tcpmessage(),
					       msgid);

				queuefeed(node, textline);
				returnstatus = 0;
			}
		}
	} else if (NoAction) {
		returnstatus = post_article(node, site, sover, textline);
	}
	return returnstatus;
}

int
save_nntplink(node, overview)
nodelist_t *node;
char *overview;
{
	FILE *POSTS;
	char buffer[1024];

	POSTS = fopen(overview, "r");
	if (POSTS == NULL)
		return 0;
	openfeed(node);
	/* if (node->feedfp == NULL) return 0; */
	flock(fileno(node->feedfp), LOCK_EX);
	while (fgets(buffer, sizeof buffer, POSTS) != NULL) {
		fputs(buffer, node->feedfp);
		fflush(node->feedfp);
	}
	flock(fileno(node->feedfp), LOCK_UN);
	fclose(POSTS);
	if (Verbose)
		printf("<Unlinking> %s\n", overview);
	if (!NoAction)
		unlink(overview);
	return 1;
}

char *
get_tmpfile(tmpfile)
char *tmpfile;
{
	FILE *FN;
	static char result[256];

	FN = fopen(tmpfile, "r");
	fgets(result, sizeof result, FN);
	fclose(FN);
	unlink(tmpfile);
	return (result);
}

/* cancel moderating posts */

int
cancel_outgoing(board, filename, from, subject)
char *board, *filename, *from, *subject;
{
	char filepath[MAXPATHLEN];
	FILE *FN;
	char *result;
	char TMPFILE[MAXPATHLEN];

	if (Verbose) {
		printf("<cancel_outgoing> %s %s %s %s\n", board, filename, from,
		       subject);
	}

	sprintf(TMPFILE, "%s/cancel_outgoing.%d.%d", INNDTMPFS, getuid(),
		getpid());

	bbslog
	    ("<cancel_outgoing> Try to move moderated post from %s to deleted\n",
	     board);
	if (Verbose)
		printf("Try to move moderated post from %s to deleted\n",
		       board);
	FN =
	    popen(fileglue
		  ("%s/bbspost post %s/boards/deleted > %s", INNDBINHOME,
		   BBSHOME, TMPFILE), "w");
	if (FN == NULL) {
		bbslog("<cancel_outgoing> can't run %s/bbspost\n", INNDBINHOME);
		if (Verbose)
			printf("<cancel_outgoing> can't run %s/bbspost\n",
			       INNDBINHOME);
		return 0;
	}
	fprintf(FN, "%s\n", from);
	fprintf(FN, "%s\n", subject);
/*  fprintf(FN, "������: %s, ����: %s\n", from, board);
  fprintf(FN, "��  ��: %s\n", subject);
  fprintf(FN, "����վ: %s (%s)\n", MYSITE, DATE);
  fprintf(FN, "ת��վ: %s\n", MYBBSID);*/
	fprintf(FN, "%s%s, %s%s\n", FromTxt, from, BoardTxt, board);
	fprintf(FN, "%s%s\n", SubjectTxt, subject);
	fprintf(FN, "%s%s (%s)\n", OrganizationTxt, SITE_PTR, DATE);
	fprintf(FN, "%s%s\n", PathTxt, MYBBSID);
	fputs("\n", FN);
	fputs(BODY, FN);
	pclose(FN);
	result = (char *) get_tmpfile(TMPFILE);
	if (strncmp(result, "post to ", 8) == 0) {
		/* try to remove it */
		strncpy(filepath,
			fileglue("%s/boards/%s/%s", BBSHOME, board, filename),
			sizeof filepath);
		if (isfile(filepath)) {
			rename(filepath, fileglue("%s.cancel", filepath));
		}
		FN = fopen(filepath, "w");

/*    fprintf(FN, "������: %s, ����: %s\n", from, board);
    fprintf(FN, "��  ��: <article cancelled and mailed to the moderator\n");
    fprintf(FN, "����վ: %s (%s)\n", MYSITE, DATE);
    fprintf(FN, "ת��վ: %s\n", MYBBSID);*/
		fprintf(FN, "%s%s, %s%s\n", FromTxt, from, BoardTxt, board);
		fprintf(FN,
			"%s<article cancelled and mailed to the moderator\n",
			SubjectTxt);
		fprintf(FN, "%s%s (%s)\n", OrganizationTxt, SITE_PTR, DATE);
		fprintf(FN, "%s%s\n", PathTxt, MYBBSID);
		fprintf(FN, "\n");
		fputs("\n", FN);
//    fprintf(FN, "������� \"%s\" �Ѿ����������. ��ȴ��ظ�.\n", subject);
		fprintf(FN, ModerationTxt, subject);

		fputs("\n", FN);
		fputs("Your post has been sent to the moderator and move\n",
		      FN);
		fputs
		    ("into the deleted board. If the post accepted by the moderator,\n",
		     FN);
		fputs("it will be posted in this board again. Please wait.\n",
		      FN);

	} else {
		bbslog("%s", result);
	}

	bbslog("%s/bbspost cancel %s %s %s moderate\n", INNDBINHOME, BBSHOME,
	       board, filename);
	if (Verbose)
		printf("%s/bbspost cancel %s %s %s moderate\n", INNDBINHOME,
		       BBSHOME, board, filename);
	system(fileglue
	       ("%s/bbspost cancel %s %s %s moderate", INNDBINHOME, BBSHOME,
		board, filename));
	return 1;
}

/*
 * send_nntplink open_link read_outgoing send_outgoing post_article
 * cancel_outgoing
 */
int
send_nntplink(node, site, hostname, hostprot, hostport, overview, nlcount)
nodelist_t *node;
char *site, *hostname, *hostprot, *hostport, *overview;
int nlcount;
{
	FILE *POSTS;
	char textline[4096];
	char baktextline[4096];

	if (Verbose) {
		printf("<send nntplink> %s %s %s %s\n", site, hostname,
		       hostprot, hostport);
		printf("    ==> %s\n", overview);
	}
	if (!open_link(hostname, hostprot, hostport)) {
		save_nntplink(node, overview);
		return 0;
	}
	POSTS = fopen(overview, "r");
	if (POSTS == NULL) {
		if (Verbose)
			printf("open %s failed\n", overview);
		return 0;
	}
	while (fgets(textline, sizeof textline, POSTS) != NULL) {
		char *linebreak = strchr(textline, '\n');
		char *ptr;
		char *board, *filename, *subject, *group, *mtime, *from;
		char *outgoingtype;
		char *msgid, *path;
		soverview_t soverview;

		strcpy(baktextline, textline);
		if (linebreak)
			*linebreak = '\0';

		board = "", filename = "", mtime = "", group = "", from =
		    "", subject = "";
		outgoingtype = "", msgid = "", path = "";
		/* get board field */
		board = textline;
		ptr = strchr(textline, '\t');
		if (ptr == NULL)
			continue;
		*ptr++ = '\0';

		/* filename field */
		filename = ptr;
		ptr = strchr(ptr, '\t');
		if (ptr == NULL)
			continue;
		*ptr++ = '\0';

		/* group field */
		group = ptr;
		ptr = strchr(ptr, '\t');
		if (ptr == NULL)
			continue;
		*ptr++ = '\0';

		/* mtime field */
		mtime = ptr;
		ptr = strchr(ptr, '\t');
		if (ptr == NULL)
			continue;
		*ptr++ = '\0';

		/* from field */
		from = ptr;
		ptr = strchr(ptr, '\t');
		if (ptr == NULL)
			continue;
		*ptr++ = '\0';

		/* subject */
		subject = ptr;
		ptr = strchr(ptr, '\t');
		if (ptr == NULL)
			goto try_read_outgoing;
		*ptr++ = '\0';

		/* outgoing type field */
		outgoingtype = ptr;
		ptr = strchr(ptr, '\t');
		if (ptr == NULL)
			goto try_read_outgoing;
		*ptr++ = '\0';

		/* msgid */
		msgid = ptr;
		ptr = strchr(ptr, '\t');
		if (ptr == NULL)
			goto try_read_outgoing;
		*ptr++ = '\0';

		/* path */
		path = ptr;
		ptr = strchr(ptr, '\t');
		if (ptr == NULL)
			goto try_read_outgoing;

	      try_read_outgoing:

		NEWSFEED = LOCAL;
		if (outgoingtype && msgid && path && *outgoingtype && *msgid
		    && *path) {
			char *left, *right;

			NEWSFEED = REMOTE;
			left = strchr(msgid, '<');
			right = strrchr(msgid, '>');
			if (left)
				msgid = left + 1;
			if (right)
				*right = '\0';
		}
		soverview.board = board;
		soverview.filename = filename;
		soverview.group = group;
		soverview.mtime = atol(mtime);
		soverview.from = from;
		soverview.subject = subject;
		soverview.outgoingtype = outgoingtype;
		soverview.msgid = msgid;
		soverview.path = path;
		if (read_outgoing(&soverview) == 0) {
			int sendresult =
			    send_outgoing(node, site, hostname, &soverview,
					  baktextline);
			int sendfailed = 1 - sendresult;

			if (NEWSFEED == REMOTE) {
				BBSLINK_STAT[nlcount].remotesendout +=
				    sendresult;
				BBSLINK_STAT[nlcount].remotefailed +=
				    sendfailed;
			} else {
				BBSLINK_STAT[nlcount].localsendout +=
				    sendresult;
				BBSLINK_STAT[nlcount].localfailed += sendfailed;
			}
			if (node->feedtype == '-') {
				if (!NoAction && sendresult)
					cancel_outgoing(board, filename, from,
							subject);
			}
		}
	}
	fclose(POSTS);
	close_link();
	if (Verbose)
		printf("<Unlinking> %s\n", overview);
	if (!NoAction)
		unlink(overview);
	return 0;
}

void
close_link()
{
	int status;

	if (Verbose)
		printf("<close_link>\n");
	if (NoAction)
		return;
	status = tcpcommand("QUIT");
	if (status != 205 && status != 221) {
		bbslog("<bbslink> :Err: Cannot quit message '%d %s'\n", status,
		       (char *) tcpmessage());
		if (Verbose)
			printf(":Err: Cannot quit message '%d %s'\n", status,
			       (char *) tcpmessage());
	}
	fclose(NNTPwfp);
	fclose(NNTPrfp);
	close(NNTP);
}

/*
 * send_article() send_nntplink() read_outgoing()
 * 
 */

int
saverename(file1, file2)
char *file1, *file2;
{
	FILE *dest, *src;
	char buf[1024];
	if (isfile(file2) && !iszerofile(file2)) {
		dest = fopen(file2, "a");
		src = fopen(file1, "r");
		if (src && dest) {
			while (fgets(buf, sizeof buf, src) != NULL) {
				fputs(buf, dest);
				fflush(dest);
			}
			fclose(dest);
			fclose(src);
			unlink(file1);
			return 0;
		} else {
			if (src)
				fclose(src);
			if (dest)
				fclose(dest);
			return rename(file1, file2);
		}
	} else {
		return rename(file1, file2);
	}
}

void
send_article()
{
	char *site, *op;
	char *nntphost;
	int nlcount;

	chdir(INNDHOME);

	for (nlcount = 0; nlcount < NLCOUNT; nlcount++) {
		nodelist_t *node;
		char linkfile[MAXPATHLEN];
		char sendfile[MAXPATHLEN];
		char feedfile[MAXPATHLEN];
		char feedingfile[MAXPATHLEN];
		char protocol[MAXBUFLEN], port[MAXBUFLEN];

		node = NODELIST + nlcount;
		site = node->node;
		nntphost = node->host;
		op = node->protocol;

		if (DefaultFeedSite && *DefaultFeedSite) {
			if (strcmp(node->node, DefaultFeedSite) != 0)
				continue;
		}

		if (op && (strncasecmp(op, "ihave", 5) == 0 ||
			   strncasecmp(op, "post", 4) == 0 ||
			   strncasecmp(op, "data", 4) == 0)) {
			char *left, *right;

			left = strchr(op, '('), right = strrchr(op, ')');
			if (left && right) {
				*left = '\0';
				*right = '\0';
				strncpy(protocol, op, sizeof protocol);
				protocol[sizeof(protocol) - 1] = 0;
				strncpy(port, left + 1, sizeof port);
				port[sizeof(port) - 1] = 0;
				*left = '(';
				*right = ')';
			} else {
				strncpy(protocol, op, sizeof protocol);
				protocol[sizeof(protocol) - 1] = 0;
				strncpy(port, "nntp", sizeof port);
				port[sizeof(port) - 1] = 0;
			}
		} else {
			strcpy(protocol, "IHAVE");
			strcpy(port, "nntp");
		}
		sprintf(linkfile, "%s/%s.link", INNDTMPFS, site);
		sprintf(sendfile, "%s/%s.sending", INNDTMPFS, site);
		sprintf(feedfile, "%s/%s.feed", INNDTMPFS, site);
		sprintf(feedingfile, "%s/%s.feeding", INNDTMPFS, site);
		if (isfile(sendfile) && !iszerofile(sendfile)) {
			if (bbslink_get_lock(sendfile)) {
				send_nntplink(node, site, nntphost, protocol,
					      port, sendfile, nlcount);
				bbslink_un_lock(sendfile);
			}
		}
		if (isfile(linkfile) && !iszerofile(linkfile)) {
			if (!NoAction) {
				if (bbslink_get_lock(sendfile)
				    && bbslink_get_lock(linkfile)
				    && bbslink_get_lock(feedingfile)) {
					if (isfile(sendfile)
					    && !iszerofile(sendfile)) {
						save_nntplink(node, sendfile);
					}
					if (node->feedfp) {
						fclose(node->feedfp);
						node->feedfp = NULL;
					}
					saverename(linkfile, sendfile);
					send_nntplink(node, site, nntphost,
						      protocol, port, sendfile,
						      nlcount);
					bbslink_un_lock(linkfile);
					bbslink_un_lock(sendfile);
					bbslink_un_lock(feedingfile);
				}
			} else {
				send_nntplink(node, site, nntphost, protocol,
					      port, linkfile, nlcount);
			}
		}
		if (isfile(feedingfile) && !iszerofile(feedingfile)) {
			if (bbslink_get_lock(feedingfile)) {
				send_nntplink(node, site, nntphost, protocol,
					      port, feedingfile, nlcount);
				bbslink_un_lock(feedingfile);
			}
		}
		if (isfile(feedfile) && !iszerofile(feedfile)) {
			if (!NoAction) {
				if (bbslink_get_lock(feedfile)
				    && bbslink_get_lock(feedingfile)) {
					if (isfile(feedingfile)
					    && !iszerofile(feedingfile)) {
						save_nntplink(node,
							      feedingfile);
						if (node->feedfp) {
							fclose(node->feedfp);
							node->feedfp = NULL;
						}
					}
					saverename(feedfile, feedingfile);
					system(fileglue
					       ("%s/ctlinnbbsd reload > /dev/null",
						INNDBINHOME));
					send_nntplink(node, site, nntphost,
						      protocol, port,
						      feedingfile, nlcount);
					bbslink_un_lock(feedfile);
					bbslink_un_lock(feedingfile);
				}
			} else {
				send_nntplink(node, site, nntphost, protocol,
					      port, feedfile, nlcount);
			}
		}
	}
}

/* bntplink() bbspost() process_article() process_cancel() send_article() */

void
show_usage(argv)
char *argv;
{
	fprintf(stderr, "%s initialization failed or improper options !!\n",
		argv);
	fprintf(stderr, "Usage: %s [options] bbs_home\n", argv);
	fprintf(stderr, "       -v (show transmission status)\n");
	fprintf(stderr,
		"       -n (dont send out articles and leave queue untouched)\n");
	fprintf(stderr,
		"       -s site (only process articles sent to site)\n");
	fprintf(stderr, "       -V (visit only: bbspost visit)\n");
	fprintf(stderr, "       -N (no visit, and only process batch queue)\n");
	fprintf(stderr,
		"       -k (kill the former bbslink process before started)\n\n");
	/*fprintf(stderr, "����ʽҪ����ִ�б��뽫���µ������� %s/innd ��:\n", BBSHOME);
	   fprintf(stderr, "bbsname.bbs   �趨��վ�� BBS ID (�뾡�����)\n");
	   fprintf(stderr, "nodelist.bbs  �趨��·�� BBS վ�� ID, Address �� fullname\n");
	   fprintf(stderr, "newsfeeds.bbs �趨��·�ż��� newsgroup board nodelist ...\n"); */
	fprintf(stderr, bbslinkUsage1, BBSHOME);
	fputs(bbslinkUsage2, stderr);
	fputs(bbslinkUsage3, stderr);
	fputs(bbslinkUsage4, stderr);
}

int
bntplink(argc, argv)
int argc;
char **argv;
{
	char OUTING[MAXPATHLEN];
	nodelist_t *nl;
	char result[4096];
	char cancelfile[MAXPATHLEN], cancelpost[MAXPATHLEN];
	char bbslink_lockfile[MAXPATHLEN];
	FILE *NEWPOST;
	char *left, *right;
	int nlcount;

	BBSHOME = MY_BBS_HOME;
	if (initial_bbs("link") == 0) {
		return -1;
	}

	sprintf(OUTING, "%s/.outing", INNDTMPFS);
	BBSLINK_STAT = (stat_t *) malloc(sizeof (stat_t) * (NLCOUNT + 1));
	for (nlcount = 0; nlcount < NLCOUNT; nlcount++) {
		BBSLINK_STAT[nlcount].localsendout = 0;
		BBSLINK_STAT[nlcount].remotesendout = 0;
		BBSLINK_STAT[nlcount].localfailed = 0;
		BBSLINK_STAT[nlcount].remotefailed = 0;
	}

	nl = (nodelist_t *) search_nodelist_bynode(MYBBSID);
	if (nl == NULL) {
		*MYADDR = '\0';
		*MYSITE = '\0';
		*LINKPROTOCOL = '\0';
	} else {
		strncpy(MYADDR, nl->host, sizeof MYADDR);
		MYADDR[sizeof(MYADDR) - 1] = 0;
		strncpy(LINKPROTOCOL, nl->protocol, sizeof LINKPROTOCOL);
		LINKPROTOCOL[sizeof(LINKPROTOCOL) - 1] = 0;
		strncpy(MYSITE, nl->comments, sizeof MYSITE);
		MYSITE[sizeof(MYSITE) - 1] = 0;
	}
	if (Verbose) {
		printf("MYADDR: %s\n", MYADDR);
		printf("MYSITE: %s\n", MYSITE);
	}
	left = strchr(nl->protocol, '(');
	right = strrchr(nl->protocol, ')');
	if (left && right) {
		*right = '\0';
		strncpy(LINKPROTOCOL, nl->protocol, sizeof LINKPROTOCOL);
		LINKPROTOCOL[sizeof(LINKPROTOCOL) - 1] = 0;
		LINKPORT = atoi(left + 1);
		*right = ')';
	}

	if (!NoVisit) {
		sprintf(bbslink_lockfile, "%s/.bbslink.visit", INNDTMPFS);
		if (!rename(fileglue("%s/out.bntp", INNDTMPFS), OUTING)
		    && bbslink_get_lock(bbslink_lockfile)) {
			/* When try to visit new post, try to lock it */
			NEWPOST = fopen(OUTING, "r");
			if (NEWPOST == NULL) {
				bbslog("<bbslink> Err: can't open %s\n",
				       OUTING);
				bbslink_un_lock(bbslink_lockfile);
				return -1;
			}
			while (fgets(result, sizeof result, NEWPOST)) {
				/* chop( $_ ); */
				char *board, *filename, *userid, *nickname,
				    *subject;
				char *ptr;

				ptr = strchr(result, '\n');
				if (ptr)
					*ptr = '\0';

				board = filename = userid = nickname = subject =
				    NULL;
				/* board field */
				board = result;
				ptr = strchr(result, '\t');
				if (ptr == NULL)
					continue;
				*ptr++ = '\0';

				/* filename field */
				filename = ptr;
				ptr = strchr(ptr, '\t');
				if (ptr == NULL)
					continue;
				*ptr++ = '\0';

				/* userid field */
				userid = ptr;
				ptr = strchr(ptr, '\t');
				if (ptr == NULL)
					continue;
				*ptr++ = '\0';

				/* nickname field */
				nickname = ptr;
				ptr = strchr(ptr, '\t');
				if (ptr == NULL)
					continue;
				*ptr++ = '\0';

				/* subject field */
				subject = ptr;
				/* ptr = strchr(ptr, '\t'); if (ptr == NULL) continue; ptr++ = '\0'; */

				process_article(board, filename, userid,
						nickname, subject);
			}
			fclose(NEWPOST);
			unlink(OUTING);
			bbslink_un_lock(bbslink_lockfile);
		}		/* getlock */
	}
	/* if NoVisit is false */
	sprintf(cancelpost, "%s/cancel.bntp", INNDTMPFS);
	if (isfile(cancelpost)) {
		FILE *CANCELFILE;

		if (bbslink_get_lock(cancelpost)
		    && bbslink_get_lock(cancelfile)) {
			sprintf(cancelfile, "%s.%d", cancelpost, getpid());
			rename(cancelpost, cancelfile);
			CANCELFILE = fopen(cancelfile, "r");
			while (fgets(result, sizeof result, CANCELFILE) != NULL) {
				/* chop( $_ ); */
				char *board, *filename, *userid, *nickname,
				    *subject;
				char *ptr;

				ptr = strchr(result, '\n');
				if (ptr)
					*ptr = '\0';
				board = filename = userid = nickname = subject =
				    NULL;

				/* board field */
				board = result;
				ptr = strchr(result, '\t');
				if (ptr == NULL)
					continue;
				*ptr++ = '\0';

				/* filename field */
				filename = ptr;
				ptr = strchr(ptr, '\t');
				if (ptr == NULL)
					continue;
				*ptr++ = '\0';

				/* userid field */
				userid = ptr;
				ptr = strchr(ptr, '\t');
				if (ptr == NULL)
					continue;
				*ptr++ = '\0';

				/* nickname field */
				nickname = ptr;
				ptr = strchr(ptr, '\t');
				if (ptr == NULL)
					continue;
				*ptr++ = '\0';

				/* subject field */
				subject = ptr;
				/* ptr = strchr(ptr, '\t'); if (ptr == NULL) continue; ptr++ = '\0'; */
				process_cancel(board, filename, userid,
					       nickname, subject);
			}
			fclose(CANCELFILE);
			if (Verbose)
				printf("Unlinking %s\n", cancelfile);
			if (!NoAction)
				unlink(cancelfile);
			bbslink_un_lock(cancelfile);
			bbslink_un_lock(cancelpost);
		}
	}

	for (nlcount = 0; nlcount < NLCOUNT; nlcount++) {
		if (NODELIST[nlcount].feedfp != NULL) {
			fclose(NODELIST[nlcount].feedfp);
			NODELIST[nlcount].feedfp = NULL;
		}
	}

	send_article();
	for (nlcount = 0; nlcount < NLCOUNT; nlcount++) {
		int localsendout, remotesendout, localfailed, remotefailed;

		localsendout = BBSLINK_STAT[nlcount].localsendout;
		remotesendout = BBSLINK_STAT[nlcount].remotesendout;
		localfailed = BBSLINK_STAT[nlcount].localfailed;
		remotefailed = BBSLINK_STAT[nlcount].remotefailed;
		if (localsendout || remotesendout || localfailed
		    || remotefailed)
			    bbslog
			    ("<bbslink> [%s]%s lsend:%d rsend:%d lfail:%d rfail:%d\n",
			     NODELIST[nlcount].node, NoAction ? "NoAction" : "",
			     localsendout, remotesendout, localfailed,
			     remotefailed);
		if (NODELIST[nlcount].feedfp != NULL) {
			fclose(NODELIST[nlcount].feedfp);
			NODELIST[nlcount].feedfp = NULL;
		}
	}
	if (BBSLINK_STAT) ;
	free(BBSLINK_STAT);
	return 0;
}

void
termbbslink(sig)
int sig;
{
	bbslog("kill signal received %d, terminated\n", sig);
	if (Verbose)
		printf("kill signal received %d, terminated\n", sig);
	exit(0);
}

char *REMOTEUSERNAME = "";
char *REMOTEHOSTNAME = "";

extern char *optarg;
extern int opterr, optind;

int
main(argc, argv)
int argc;
char **argv;
{
	int c, errflag = 0;

	CONTROL = CONTROL_BUF;
	MSGID = MSGID_BUF;

	initial_lang();

	NoAction = 0;
	Verbose = 0;
	VisitOnly = 0;
	NoVisit = 0;
	DefaultFeedSite = "";

	while ((c = getopt(argc, argv, "s:hnvVNk")) != -1)
		switch (c) {
		case 's':
			DefaultFeedSite = optarg;
			break;
		case 'n':
			NoAction = 1;
			break;
		case 'v':
			Verbose = 1;
			break;
		case 'V':
			VisitOnly = 1;
			break;
		case 'N':
			NoVisit = 1;
			break;
		case 'k':
			KillFormerBBSLINK = 1;
			break;
		case 'h':
		default:
			errflag++;
			break;
		}
	if (errflag > 0) {
		show_usage(argv[0]);
		return (1);
	}
	if (argc - optind < 1) {
		show_usage(argv[0]);
		exit(1);
	}
	signal(SIGTERM, (void *) termbbslink);
	signal(SIGALRM, SIG_DFL);
	alarm(250);
	if (bntplink(argc - optind, argv + optind) != 0) {
		show_usage(argv[0]);
		exit(1);
	}
	return 0;
}

void
readNCMfile()
{
}
