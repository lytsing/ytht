/*
    POP3D server for Firebird BBS.
    Revision [29 Oct 1997] By Peng-Piaw Foong, ppfoong@csie.ncu.edu.tw
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "bbs.h"
#include "ythtbbs.h"

/* Edit these lines to fit the configuration of your server */

#define POP3D_PORT  110
#define BUFSIZE     1024

#ifndef TIOCNOTTY
#define TIOCNOTTY 0x5422
#endif

/*  You needn't modify any lines below unless you know what you're doing */

struct bbsinfo bbsinfo;
struct fileheader currentmail;
struct userec *currentuser;

char LowUserid[20];
char genbuf[BUFSIZE];
char hostname[80];

#define QLEN            5

#define S_CONNECT       1
#define S_LOGIN         2

#define STRN_CPY(d,s,l) { strncpy((d),(s),(l)); (d)[(l)-1] = 0; }
#define ANY_PORT        0
#define RFC931_PORT     113
#define RFC931_TIMEOUT  5
#define POP3_TIMEOUT    60

//static jmp_buf timebuf;

int State;
int msock, sock;		/* master server socket */
static int reaper();
char fromhost[STRLEN];
char inbuf[100];
FILE *cfp;
char *msg, *cmd;
int fd;
struct fileheader *fcache;
char (*fowner)[STRLEN];
int totalnum, totalbyte, markdel, idletime;
int *postlen;

void log_usies();
int Quit(), User(), Pass(), Noop(), Stat(), List(), Retr(), Rset();
int Last(), Dele(), Uidl(), Top();

struct commandlist {
	char *name;
	int (*fptr) (void);
};

struct commandlist cmdlists[] = {
	{"retr", Retr}, {"dele", Dele}, {"user", User}, {"pass", Pass},
	{"stat", Stat}, {"list", List}, {"uidl", Uidl}, {"quit", Quit},
	{"rset", Rset}, {"last", Last}, {"noop", Noop}, {"top", Top},
	{NULL, NULL}
};

static int
abort_server()
{
	log_usies("ABORT SERVER");
	close(msock);
	close(sock);
	exit(1);
}

int
dokill()
{
	return kill(0, SIGKILL);
}

#if 0
static FILE *
fsocket(domain, type, protocol)
int domain;
int type;
int protocol;
{
	int s;
	FILE *fp;

	if ((s = socket(domain, type, protocol)) < 0) {
		return (0);
	} else {
		if ((fp = fdopen(s, "r+")) == 0) {
			close(s);
		}
		return (fp);
	}
}
#endif

void
outs(str)
char *str;
{
	fprintf(cfp, "%s\r\n", str);
}

//add by gluon for filter ansi control code
void
filter(char *line)
{
	char temp[256];
	int i, stat, j;
	stat = 0;
	j = 0;
	for (i = 0; line[i] && i < 255; i++) {
		if (line[i] == '\033')
			stat = 1;
		if (!stat)
			temp[j++] = line[i];
		if (stat && ((line[i] > 'a' && line[i] < 'z')
			     || (line[i] > 'A' && line[i] < 'Z')
			     || line[i] == '@'))
			stat = 0;
	}
	temp[j] = 0;
	strcpy(line, temp);
}

void
outfile(char *filename, int ln)
//ln < 0 means out ulimit lines, ln >=0 means out most ln lines.
{
	FILE *fin;
	char *attach;
	char b64_in[256];
	char *buf;
	char *mime_type;
	int len;
	char tail_buf[3];
	int tail_len = 0;
	int out_count;

	if ((fin = fopen(filename, "r")) == NULL) {
		outs("\r\n.");
		return;
	}
	f_b64_ntop_init(tail_buf, &tail_len, &out_count);
	while (fgets(b64_in, sizeof (b64_in), fin) != NULL && ln--) {
		if (NULL != (attach = checkbinaryattach(b64_in, fin, &len))) {
			if (len <= 0)
				continue;
			buf = malloc(len);
			if (buf == NULL)
				continue;
			fread(buf, len, 1, fin);
			f_b64_ntop_fini(cfp, tail_buf, &tail_len);
			fprintf(cfp, "\r\n--" MY_MIME_BOUNDARY "\r\n");
			mime_type = get_mime_type(attach);
			fprintf(cfp, "Content-Type: %s; name=%s\r\n", mime_type,
				attach);
			fprintf(cfp,
				"Content-Transfer-Encoding: Base64\r\n\r\n");
			f_b64_ntop_init(tail_buf, &tail_len, &out_count);
			f_b64_ntop(cfp, buf, len, tail_buf, &tail_len,
				   &out_count);
			f_b64_ntop_fini(cfp, tail_buf, &tail_len);
			fprintf(cfp, "\r\n--" MY_MIME_BOUNDARY "\r\n");
			fprintf(cfp,
				"Content-Type: text/plain; charset=gb2312\r\n");
			fprintf(cfp,
				"Content-Transfer-Encoding: Base64\r\n\r\n");
			f_b64_ntop_init(tail_buf, &tail_len, &out_count);
			free(buf);
			continue;
		}
		filteransi(b64_in);
		f_b64_ntop(cfp, b64_in, strlen(b64_in), tail_buf, &tail_len,
			   &out_count);
	}
	f_b64_ntop_fini(cfp, tail_buf, &tail_len);
	fprintf(cfp, "\r\n--" MY_MIME_BOUNDARY "--\r\n.\r\n");
	fclose(fin);
}

#if 0
/* timeout - handle timeouts */
static void
timeout(sig)
int sig;
{
	longjmp(timebuf, sig);
}

void
rfc931(rmt_sin, our_sin, dest)
struct sockaddr_in *rmt_sin;
struct sockaddr_in *our_sin;
char *dest;
{
	unsigned rmt_port;
	unsigned our_port;
	struct sockaddr_in rmt_query_sin;
	struct sockaddr_in our_query_sin;
	char user[256];
	char buffer[512];
	char *cp;
	FILE *fp;
	char *result = "unknown";

	/*
	 * Use one unbuffered stdio stream for writing to and for reading from
	 * the RFC931 etc. server. This is done because of a bug in the SunOS 
	 * 4.1.x stdio library. The bug may live in other stdio implementations,
	 * too. When we use a single, buffered, bidirectional stdio stream ("r+"
	 * or "w+" mode) we read our own output. Such behaviour would make sense
	 * with resources that support random-access operations, but not with   
	 * sockets.
	 */
	if ((fp = fsocket(AF_INET, SOCK_STREAM, 0)) != 0) {
		setbuf(fp, (char *) 0);

		/*
		 * Set up a timer so we won't get stuck while waiting for the server.
		 */

		if (setjmp(timebuf) == 0) {
			signal(SIGALRM, (void *) timeout);
			alarm(RFC931_TIMEOUT);

			/*
			 * Bind the local and remote ends of the query socket to the same
			 * IP addresses as the connection under investigation. We go
			 * through all this trouble because the local or remote system
			 * might have more than one network address. The RFC931 etc.  
			 * client sends only port numbers; the server takes the IP    
			 * addresses from the query socket.
			 */

			our_query_sin = *our_sin;
			our_query_sin.sin_port = htons(ANY_PORT);
			rmt_query_sin = *rmt_sin;
			rmt_query_sin.sin_port = htons(RFC931_PORT);

			if (bind(fileno(fp), (struct sockaddr *) &our_query_sin,
				 sizeof (our_query_sin)) >= 0 &&
			    connect(fileno(fp),
				    (struct sockaddr *) &rmt_query_sin,
				    sizeof (rmt_query_sin)) >= 0) {

				/*
				 * Send query to server. Neglect the risk that a 13-byte
				 * write would have to be fragmented by the local system and
				 * cause trouble with buggy System V stdio libraries.
				 */

				fprintf(fp, "%u,%u\r\n",
					ntohs(rmt_sin->sin_port),
					ntohs(our_sin->sin_port));
				fflush(fp);

				/*
				 * Read response from server. Use fgets()/sscanf() so we can
				 * work around System V stdio libraries that incorrectly
				 * assume EOF when a read from a socket returns less than
				 * requested.
				 */

				if (fgets(buffer, sizeof (buffer), fp) != 0
				    && ferror(fp) == 0 && feof(fp) == 0
				    && sscanf(buffer,
					      "%u , %u : USERID :%*[^:]:%255s",
					      &rmt_port, &our_port, user) == 3
				    && ntohs(rmt_sin->sin_port) == rmt_port
				    && ntohs(our_sin->sin_port) == our_port) {

					/*
					 * Strip trailing carriage return. It is part of the
					 * protocol, not part of the data.
					 */

					if ((cp = strchr(user, '\r')))
						*cp = 0;
					result = user;
				}
			}
			alarm(0);
		}
		fclose(fp);
	}
	STRN_CPY(dest, result, 60);

	if (strcmp(dest, "unknown") == 0)
		strcpy(dest, "");
	else
		strcat(dest, "@");

	/*hp = gethostbyaddr((char *) &rmt_sin->sin_addr, sizeof (struct in_addr),
	   rmt_sin->sin_family);
	   if (hp)
	   strcat(dest, hp->h_name);
	   else *///�ƺ��а�ȫ����, removed by ylsdd
	strcat(dest, (char *) inet_ntoa(rmt_sin->sin_addr));

}
#endif

int
Isspace(ch)
char ch;
{
	return (ch == ' ' || ch == '\t' || ch == 10 || ch == 13);
}

char *
nextwordlower(str)
char **str;
{
	char *p;

	while (Isspace(**str))
		(*str)++;
	p = (*str);

	while (**str && !Isspace(**str)) {
		**str = tolower(**str);
		(*str)++;
	}

	if (**str) {
		**str = '\0';
		(*str)++;
	}
	return p;
}

char *
nextword(str)
char **str;
{
	char *p;

	while (Isspace(**str))
		(*str)++;
	p = (*str);

	while (**str && !Isspace(**str))
		(*str)++;

	if (**str) {
		**str = '\0';
		(*str)++;
	}
	return p;
}

void
Init()
{
	State = S_CONNECT;
	LowUserid[0] = '\0';
	markdel = 0;
	idletime = 0;
	(void) gethostname(hostname, 80);
}

void
Login_init()
{
	int fd, i;
	struct stat st;

	totalnum = totalbyte = 0;
	sprintf(genbuf, "mail/%c/%s/.DIR", mytoupper(*LowUserid), LowUserid);
	if (stat(genbuf, &st) == -1 || st.st_size == 0) {
		return;
	}
	totalnum = st.st_size / sizeof (struct fileheader);
	fcache = (struct fileheader *) malloc(st.st_size);
	fowner = malloc(STRLEN * totalnum);
	postlen = (int *) malloc(sizeof (int) * totalnum);
	fd = open(genbuf, O_RDONLY);
	read(fd, fcache, st.st_size);
	close(fd);

	for (i = 0; i < totalnum; i++) {
		setmailfile(genbuf, LowUserid, fh2fname(&fcache[i]));
		strcpy(fowner[i], fh2owner(&fcache[i]));
		if (strchr(fowner[i], '.') == NULL) {
			strcat(fowner[i], ".bbs@" MY_BBS_DOMAIN);
		} else {
			getdocauthor(genbuf, fowner[i], STRLEN);
		}
		if (stat(genbuf, &st) == -1)
			st.st_size = 0;
		postlen[i] =
		    st.st_size + strlen(fowner[i]) + 10 +
		    strlen(fcache[i].title) + 10 + 40;
		totalbyte += postlen[i];
	}
}

void
pop3_timeout()
{
	idletime++;
	if (idletime > 5) {
		log_usies("ABORT - TIMEOUT");
		fclose(cfp);
		close(sock);
		exit(1);
	}
	alarm(POP3_TIMEOUT);
}

int
main(argc, argv)
int argc;
char **argv;
{

	struct sockaddr_in fsin, our;
	int on, alen, len, i, n;
	char *str;
	char *IPaddr = NULL;
	int portnum = POP3D_PORT;
	int childpid;

	if (initbbsinfo(&bbsinfo) < 0)
		exit(1);
	if (uhash_uptime() == 0)
		exit(1);

	if (argc == 2) {
		if (!strchr(argv[1], '.'))
			portnum = atoi(argv[1]);
		else
			IPaddr = argv[1];
	}
	if (argc > 2) {
		IPaddr = argv[1];
		portnum = atoi(argv[2]);
	}

	if (fork())
		exit(0);
	setsid();
	if (fork())
		exit(0);
	for (n = 0; n < 10; n++)
		close(n);
	open("/dev/null", O_RDONLY);
	dup2(0, 1);
	dup2(0, 2);
	if ((n = open("/dev/tty", O_RDWR)) > 0) {
		ioctl(n, TIOCNOTTY, 0);
		close(n);
	}

	if ((msock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		exit(1);
	}
	setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof (on));
	bzero((char *) &fsin, sizeof (fsin));
	fsin.sin_family = AF_INET;
	if (!IPaddr) {
		fsin.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		if (inet_aton(IPaddr, &fsin.sin_addr) == 0)
			exit(1);
	}
	fsin.sin_port = htons(portnum);

	if (bind(msock, (struct sockaddr *) &fsin, sizeof (fsin)) < 0) {
		exit(1);
	}
	if (setgid(BBSGID) < 0 || setuid(BBSUID) < 0)
		exit(1);

	signal(SIGHUP, (void *) abort_server);
	signal(SIGCHLD, (void *) reaper);
	signal(SIGINT, (void *) dokill);
	signal(SIGTERM, (void *) dokill);

	listen(msock, QLEN);

	while (1) {

		alen = sizeof (fsin);
		sock = accept(msock, (struct sockaddr *) &fsin, &alen);
		if ((sock < 0) && (errno == EINTR))
			continue;

		if ((childpid = fork()) < 0) {
			exit(1);
		}
		if (childpid == 0)
			break;
		close(sock);
	}

	close(msock);

	strcpy(fromhost, (char *) inet_ntoa(fsin.sin_addr));
	if (checkbansite(fromhost))
		exit(0);
	len = sizeof our;
	getsockname(sock, (struct sockaddr *) &our, &len);

	Init();

//      rfc931(&fsin, &our, remote_userid);

	cfp = fdopen(sock, "r+");

	sprintf(genbuf, "+OK InternetBBS Pop server at %s starting.", hostname);
	outs(genbuf);
	fflush(cfp);

	chdir(MY_BBS_HOME);

	log_usies("CONNECT");
	alarm(0);
	signal(SIGALRM, (void *) pop3_timeout);
	alarm(POP3_TIMEOUT);

	while (fgets(inbuf, sizeof (inbuf), cfp) != 0) {

		idletime = 0;

		msg = inbuf;

		inbuf[strlen(inbuf) - 1] = '\0';
		if (inbuf[strlen(inbuf) - 1] == '\r')
			inbuf[strlen(inbuf) - 1] = '\0';
		cmd = nextwordlower(&msg);

		if (*cmd == 0)
			continue;

		i = 0;
		while ((str = cmdlists[i].name) != NULL) {
			if (strcmp(cmd, str) == 0)
				break;
			i++;
		}

		if (str == NULL) {
			sprintf(genbuf, "-ERR Unknown command: \"%s\".", cmd);
			outs(genbuf);
			fflush(cfp);
		} else {
			(*cmdlists[i].fptr) ();
			fflush(cfp);
		}

	}

	if (State == S_LOGIN) {
		free(fcache);
		free(postlen);
		free(fowner);
	}
	log_usies("ABORT");
	fclose(cfp);
	close(sock);
	exit(0);
}

static int
reaper()
{
	int state, pid;

	signal(SIGCHLD, (void *) reaper);
	signal(SIGINT, (void *) dokill);
	signal(SIGTERM, (void *) dokill);

	while ((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0) ;
	return 0;
}

int
Noop()
{
	outs("+OK");
	return 0;
}

int
get_userdata(user)
char *user;
{
	int n;

	n = getuser(user, &currentuser);
	if (n <= 0) {
		currentuser = NULL;
		return -1;
	}
	strcpy(user, currentuser->userid);
	return 1;
}

int
User()
{
	char *ptr;

	if (State == S_LOGIN) {
		outs("-ERR Unknown command: \"user\".");
		return -1;
	}
	LowUserid[0] = '\0';

	cmd = nextwordlower(&msg);
	if (*cmd == 0) {
		outs("-ERR Too few arguments for the user command.");
		return -1;
	}
	if (strstr(cmd, ".bbs") != NULL) {
		ptr = strchr(cmd, '.');
		*ptr = '\0';
	}
	if (get_userdata(cmd) != 1) {
		sprintf(genbuf, "-ERR Unknown user: \"%s\".", cmd);
		outs(genbuf);
		return -1;
	}
	if (userbansite(cmd, fromhost)) {
		sprintf(genbuf,
			"-ERR Connection from %s is not allowed for user %s.",
			fromhost, cmd);
		outs(genbuf);
		return -1;
	}
	strcpy(LowUserid, cmd);
	sprintf(genbuf, "+OK Password required for %s.bbs.", cmd);
	outs(genbuf);
	return 0;
}

void
log_usies(buf)
char *buf;
{
	FILE *fp;

	if ((fp = fopen("reclog/pop3d.log", "a")) != NULL) {
		time_t now;
		struct tm *p;

		time(&now);
		p = localtime(&now);
		fprintf(fp, "%02d/%02d/%02d %02d:%02d:%02d [%s]() %s\n",
			p->tm_year, p->tm_mon + 1, p->tm_mday, p->tm_hour,
			p->tm_min, p->tm_sec,
			(currentuser != NULL) ? currentuser->userid : "", buf);
		fflush(fp);
		fclose(fp);
	}
}

int
Retr()
{
	int num;
	char *ptr;

	if (State != S_LOGIN) {
		outs("-ERR Unknown command: \"retr\".");
		return -1;
	}

	cmd = nextword(&msg);

	if (*cmd == 0) {
		outs("-ERR Too few arguments for the retr command.");
		return -1;
	}

	num = atoi(cmd);
	if (num <= 0 || totalnum < num) {
		sprintf(genbuf, "-ERR Message %d does not exist.", num);
		outs(genbuf);
		return -1;
	} else if (fcache[num - 1].accessed & FH_DIGEST) {
		sprintf(genbuf, "-ERR Message %d has been deleted.", num);
		outs(genbuf);
		return -1;
	}
	num--;
	sprintf(genbuf, "+OK %d octets", postlen[num]);
	outs(genbuf);
//copy from smth pop3d, it should set the date in the mail received from bbspop3d
	ptr = ctime((time_t *) & fcache[num]);
	sprintf(genbuf,
		"Received: from insidesmtp (unknown [127.0.0.1]); %3.3s, %2.2s %3.3s %4.4s %8.8s +0800",
		ptr + 0, ptr + 8, ptr + 4, ptr + 20, ptr + 11);
	outs(genbuf);
	sprintf(genbuf, "Date: %3.3s, %2.2s %3.3s %4.4s %8.8s +0800", ptr + 0,
		ptr + 8, ptr + 4, ptr + 20, ptr + 11);
	outs(genbuf);
//copy end
	sprintf(genbuf, "From: %s", fowner[num]);
	outs(genbuf);
	sprintf(genbuf, "To: %s.bbs@" MY_BBS_DOMAIN, currentuser->userid);
	outs(genbuf);
	sprintf(genbuf, "Subject: %s", fcache[num].title);
	outs(genbuf);
	fprintf(cfp, "Content-Type: multipart/mixed;\r\n");
	fprintf(cfp, "              boundary=" MY_MIME_BOUNDARY "\r\n");
	fprintf(cfp, "Content-Transfer-Encoding: 7bit\r\n\r\n");
	fprintf(cfp, "This is a multi-part message in MIME format.\r\n");

	fprintf(cfp, "--" MY_MIME_BOUNDARY "\r\n");
	fprintf(cfp, "Content-Type: text/plain; charset=gb2312\r\n");
	fprintf(cfp, "Content-Transfer-Encoding: Base64\r\n\r\n");

	setmailfile(genbuf, LowUserid, fh2fname(&fcache[num]));
	outfile(genbuf, -1);
	return 0;
}

int
Stat()
{
	if (State != S_LOGIN) {
		outs("-ERR Unknown command: \"stat\".");
		return -1;
	}
	sprintf(genbuf, "+OK %d %d", totalnum, totalbyte);
	outs(genbuf);
	return 0;
}

int
Rset()
{
	int i;

	if (State != S_LOGIN) {
		outs("-ERR Unknown command: \"rset\".");
		return -1;
	}

	for (i = 0; i < totalnum; i++) {
		fcache[i].accessed &= ~FH_DIGEST;
	}
	markdel = 0;
	sprintf(genbuf, "+OK Maildrop has %d messages (%d octets)", totalnum,
		totalbyte);
	outs(genbuf);
	return 0;
}

int
List()
{
	int i;

	if (State != S_LOGIN) {
		outs("-ERR Unknown command: \"list\".");
		return -1;
	}

	cmd = nextword(&msg);

	if (*cmd == 0) {
		sprintf(genbuf, "+OK %d messages (%d octets)", totalnum,
			totalbyte);
		outs(genbuf);
		for (i = 0; i < totalnum; i++) {
			if (!(fcache[i].accessed & FH_DIGEST)) {
				sprintf(genbuf, "%d %d", i + 1, postlen[i]);
				outs(genbuf);
			}
		}
		outs(".");
	} else {
		i = atoi(cmd);
		if (i <= 0 || totalnum < i) {
			sprintf(genbuf, "-ERR Message %d does not exist.", i);
			outs(genbuf);
			return -1;
		} else if (fcache[i - 1].accessed & FH_DIGEST) {
			sprintf(genbuf, "-ERR Message %d has been deleted.", i);
			outs(genbuf);
			return -1;
		}
		sprintf(genbuf, "+OK %d %d", i, postlen[i - 1]);
		outs(genbuf);
	}
	return 0;
}

int
Top()
{				/* Leeward adds, 98.01.21 ,modified and add to ytht by lepton 2003.09.12 */
	int num;
	int ln;
	char *ptr;

	if (State != S_LOGIN) {
		outs("-ERR Unknown command: \"top\".");
		return -1;
	}

	cmd = nextword(&msg);

	if (*cmd == 0) {
		outs("-ERR Too few arguments for the top command.");
		return -1;
	}

	num = atoi(cmd);
	if (num <= 0 || totalnum < num) {
		sprintf(genbuf, "-ERR Message %d does not exist.", num);
		outs(genbuf);
		return -1;
	} else if (fcache[num - 1].accessed & FH_DIGEST) {
		sprintf(genbuf, "-ERR Message %d has been deleted.", num);
		outs(genbuf);
		return -1;
	}

	cmd = nextword(&msg);

	if (*cmd == 0) {
		outs("-ERR Too few arguments for the top command.");
		return -1;
	}

	ln = atoi(cmd);
	if (ln < 0) {
		sprintf(genbuf, "-ERR Line %d does not exist.", ln);
		outs(genbuf);
		return -1;
	}

	num--;
	sprintf(genbuf, "+OK %d octets", postlen[num]);
	outs(genbuf);
	ptr = ctime((time_t *) & fcache[num]);
	/*
	 * Wed Jan 21 17:42:14 1998            -- ctime returns
	 * 012345678901234567890123            -- offsets
	 * Date: Wed, 21 Jan 1998 17:54:33 +0800     -- RFC wants     
	 */
	sprintf(genbuf,
		"Received: from insidesmtp (unknown [127.0.0.1]); %3.3s, %2.2s %3.3s %4.4s %8.8s +0800",
		ptr + 0, ptr + 8, ptr + 4, ptr + 20, ptr + 11);
	outs(genbuf);
	sprintf(genbuf, "Date: %3.3s, %2.2s %3.3s %4.4s %8.8s +0800", ptr + 0,
		ptr + 8, ptr + 4, ptr + 20, ptr + 11);
	outs(genbuf);
	sprintf(genbuf, "From: %s", fowner[num]);
	outs(genbuf);
	sprintf(genbuf, "To: %s.bbs@" MY_BBS_DOMAIN, currentuser->userid);
	outs(genbuf);
	sprintf(genbuf, "Subject: %s", fcache[num].title);
	outs(genbuf);
	outs("");
	setmailfile(genbuf, LowUserid, fh2fname(&fcache[num]));
	outfile(genbuf, ln);
	return 0;
}

int
Uidl()
{
	int i;

	if (State != S_LOGIN) {
		outs("-ERR Unknown command: \"uidl\".");
		return -1;
	}

	cmd = nextword(&msg);

	if (*cmd == 0) {
		outs("+OK");
		for (i = 0; i < totalnum; i++) {
			if (!(fcache[i].accessed & FH_DIGEST)) {
				sprintf(genbuf, "%d %s", i + 1,
					fh2fname(&fcache[i]));
				outs(genbuf);
			}
		}
		outs(".");
	} else {
		i = atoi(cmd);
		if (i <= 0 || totalnum < i) {
			sprintf(genbuf, "-ERR Message %d does not exist.", i);
			outs(genbuf);
			return -1;
		} else if (fcache[i - 1].accessed & FH_DIGEST) {
			sprintf(genbuf, "-ERR Message %d has been deleted.", i);
			outs(genbuf);
			return -1;
		}
		sprintf(genbuf, "+OK %d %s", i, fh2fname(&fcache[i - 1]));
		outs(genbuf);
	}
	return 0;
}

int
Pass()
{
	if (State == S_LOGIN) {
		outs("-ERR Unknown command: \"pass\".");
		return -1;
	}

	cmd = msg;		//Conform to RFC 1939, PASSWORD may contain space...

	if (*cmd == 0) {
		outs("-ERR Too few arguments for the pass command.");
		return -1;
	}

	if (LowUserid[0] == '\0') {
		outs("-ERR need a USER");
		return -1;
	}

	if (!checkpasswd(currentuser->passwd, currentuser->salt, cmd)) {
		time_t t = time(0);
		logattempt(currentuser->userid, fromhost, "POP3", t);
		sprintf(genbuf,
			"-ERR Password supplied for \"%s.bbs\" is incorrect.",
			LowUserid);
		outs(genbuf);
		LowUserid[0] = '\0';
		log_usies("ERROR PASSWD");
		return -1;
	}

	if (-2 == get_mailsize(currentuser)) {
		sprintf(genbuf, "-ERR MailBox size error.");
		outs(genbuf);
		errlog("strange user %s", currentuser->userid);
		return -1;
	}

	if (State == S_CONNECT) {
		log_usies("ENTER");
		State = S_LOGIN;
	}

	Login_init();
	sprintf(genbuf, "+OK %s has %d message(s) (%d octets).", LowUserid,
		totalnum, totalbyte);
	outs(genbuf);
	return 0;
}

int
Last()
{
	if (State != S_LOGIN) {
		outs("-ERR Unknown command: \"last\".");
		return -1;
	}

	sprintf(genbuf, "+OK %d is the last message seen.", totalnum);
	outs(genbuf);
	return 0;
}

int
Dele()
{
	int num;

	if (State != S_LOGIN) {
		outs("-ERR Unknown command: \"dele\".");
		return -1;
	}

	cmd = nextword(&msg);

	if (*cmd == 0) {
		outs("-ERR Too few arguments for the dele command.");
		return -1;
	}

	num = atoi(cmd);
	if (num <= 0 || totalnum < num) {
		sprintf(genbuf, "-ERR Message %d does not exist.", num);
		outs(genbuf);
		return -1;
	} else if (fcache[num - 1].accessed & FH_DIGEST) {
		sprintf(genbuf, "-ERR Message %d has already been deleted.",
			num);
		outs(genbuf);
		return -1;
	}

	num--;

/*modified by KCN for not delete marked mail. 98.11    	*/
	fcache[num].accessed |= FH_DIGEST;
	markdel++;
	sprintf(genbuf, "+OK Message %d has been deleted.%x", num,
		fcache[num].accessed);
	outs(genbuf);
	return 0;
}

int
do_delete()
{
	int i;
	char fpath[80], buf[80];

	sprintf(fpath, "mail/%c/%s/.DIR", mytoupper(*LowUserid), LowUserid);
	for (i = 0; i < totalnum; i++) {
		if (!fcache[i].accessed & FH_DIGEST
		    || fcache[i].accessed & FH_MARKED)
			continue;
		sprintf(buf, "mail/%c/%s/%s", mytoupper(*LowUserid), LowUserid,
			fh2fname(&(fcache[i])));
		if (!delete_dir_callback
		    (fpath, i + 1, fcache[i].filetime,
		     (void *) update_mailsize_down, LowUserid)) {
			deltree(buf);
		}
	}
	return 0;
}

int
Quit()
{
	if (State == S_LOGIN) {
		if (markdel)
			do_delete();
		free(fcache);
		free(postlen);
	}
	log_usies("EXIT");
	sprintf(genbuf, "+OK InternetBBS Pop server at %s signing off.",
		hostname);
	outs(genbuf);
	fclose(cfp);
	close(sock);
	exit(0);
}
