/*
   CGI execution Daemon -- Copyright (C) 2003 Thomas M. Ogrisegg

   This file is part of Cthulhu distribution.
   Refer to cgi-exec(8) for documentation about how cgi-exec works

   Revision history:

    02/14/03: misc interpreter fixes
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#define ENTRY(x) { x, sizeof (x) - 1 }
#define err2(x) { perror (x); return; }
#define die2(x) { perror (x); exit (23); }
#define prints(x) write (1, x, sizeof (x) - 1)
#define MAX_CONNS 128

static struct {
	char *name;
	size_t len;
} headers[] = {
	ENTRY ("Pragma: "),
	ENTRY ("Content-Type: "),
	ENTRY ("Authentication: "),
	ENTRY ("Content-Length: "),
	ENTRY ("Connection: "),
};

enum { READING, WRITING, WAITING };

/* alloc.c */
extern void *xxalloc __P ((size_t));
extern void alloc_reset __P ((void));

/* config.c */
extern void read_domain_config __P ((char *, char **, char **));

/* env.c */
extern int read_environ __P ((void));
extern void set_global_env __P ((void));

/* w.c */
extern int wildmat __P ((char *, char *));

static char **environ;
static int sockfd;
static int proccnt;
static int need_reinit;
extern size_t envlinenum;

static size_t
slen (str)
	char *str;
{
	size_t res = 0;
	while (str[res]&&str[res]!='\n'&&str[res]!='\r') res++;
	return (res);
}

static int cp;

#define put_env(a,b) _put_env(a,b,sizeof(a)-1,slen(b))

void
_put_env (name, value, s1, s2)
	char *name, *value;
	size_t s1, s2;
{
	environ[cp] = xxalloc (s1+s2+3);
	memcpy (environ[cp], name, s1);
	environ[cp][s1] = '=';
	memcpy (environ[cp]+s1+1, value, s2);
	environ[cp][s1+s2+1] = 0;
	cp++;
}

static void
put_http_env (name)
	char *name;
{
	size_t len = slen (name), i, x;
	char *newenv = xxalloc (len+9), *str = newenv;
	memcpy (newenv, "HTTP_", 5);
	for (i=0;name[i]!=':'&&name[i]>' ';i++) {
		if (name[i]>='a'&&name[i]<='z')
			str[i+5] = name[i]&~0x20;
		else if (name[i]=='-') str[i+5]='_';
		else str[i+5] = name[i];
	}
	if (name[i] != ':') return;
	x = i+5;
	str[x++] = '=';
	while (name[++i]<=' '&&name[i]!='\n'&&name[i]!='\r'&&name[i]);
	for (;name[i]!='\n'&&name[i]!='\r';i++,x++)
		str[x] = name[i];
	str[x] = 0;
	environ[cp++] = newenv;
}

static char *
_extract_header (base, str, blen, slen)
	char *base, *str;
	size_t blen, slen;
{
	size_t i;
	for (i=0;i<blen;) {
		if (*base == *str && !memcmp (base, str, slen))
			return (base+slen);
		while (i++<blen&&*base++!='\n');
	}
	return (NULL);
}

static int
good_header (hdr)
	char *hdr;
{
	size_t i;
	for (i=0;i<sizeof (headers) / sizeof (headers[0]);i++)
		if (headers[i].name[0] == *hdr &&
			!memcmp (headers[i].name, hdr, headers[i].len))
			return (1);
	return (0);
}

static size_t
count_params (str)
	char *str;
{
	size_t res;
	for (res=0;*str>' ';str++) if ('+'==*str) res++;
	return (res);
}

static int
_exit_error (str, fd)
	char str;
	int fd;
{
	char c = str;
	write (fd, &c, sizeof (c));
	return (0);
}

static void
reinit ()
{
	if (need_reinit) { config_alloc_reset (); need_reinit = 0; }
	alloc_reset ();
	cp = 0;
}

static size_t
count_nls (str,len)
	char *str;
	size_t len;
{
	size_t i, res = 0;
	for (i=0;i<len;) if (str[++i]=='\n') res++;
	return (str[i]=='\n'?res:0);
}

static size_t
putipaddr (addr, dst)
	unsigned int addr;
	char *dst;
{
	unsigned char *ipaddr = (unsigned char *) &addr;
	int i;
	char *orig = dst;

	for (i=0;i<4;i++) {
		unsigned char cad = ipaddr[i], f;
		if ((f=(cad>=100))) { *dst++ = (cad/100+'0'); cad %= 100; }
		if (f||cad>=10) *dst++ = (cad/10+'0');
		*dst++ = (cad%10+'0');
		if (i != 3) *dst++ = ('.');
	}
	*dst = 0;
	return (dst-orig);
}

#define exit_error(x) _exit_error(x, fd)
#define err400() exit_error ('\1')
#define err403() exit_error ('\2')
#define err404() exit_error ('\3')
#define err500() exit_error ('\4')
#define extract_header(a) _extract_header (req+i,a,len-i-1,sizeof(a)-1)
/* TODO: SCRIPT_NAME, REMOTE_HOST, REMOTE_PORT, REMOTE_ADDR, SERVER_NAME,
   SERVER_PORT, SERVER_ADMIN, SERVER_ADDR */
static int
exec_request (req, saddr, len, fd)
	char *req;
	struct sockaddr_in *saddr;
	size_t len;
{
	unsigned char type = *req++;
	char *url = req, *tmp, *meth, *ruri = NULL;
	size_t i = 1, linecount = 0, dlen, ulen;
	char **args, *cd = NULL, *root = NULL;
	char *qstring = NULL, *interpreter = NULL, *sname = NULL, *droot;
	struct stat st;
	pid_t pid;
	char tmpaddr[0x20];
	char *arg0;

	reinit ();

	if (len > 0xffff || len < 8 || req[len-2] != '\n' || (type &~4) > 3)
		return (err400 ());
	len--;
	if ((linecount = count_nls (req, len-1)) < 3)
		return (err400 ());
	if (type & 4) {
		type &= ~4;
		interpreter = req;
		for (;i<len&&req[i]>' ';i++);
		req[i++] = 0;
		url = req+i;
	}
	for (sname=url;*sname>' '&&*sname!='/';sname++);
	if ('/'!=*sname) return (err400 ()); /*XXX*/

	for (dlen=0;!sname[dlen]||sname[dlen]>' ';dlen++);
	if (dlen > 3) {
		ruri = alloca (dlen+1);
		memcpy (ruri, sname, dlen);
		ruri[dlen--] = 0;
		if (dlen!=-1) do if (!ruri[dlen]) ruri[dlen]='?'; while(--dlen);
	}

	dlen = sname-url+sizeof(BASEDIR);
	droot = alloca (dlen+1);
	memcpy (droot, BASEDIR, sizeof (BASEDIR) - 1);
	memcpy (droot+sizeof (BASEDIR)-1, url, dlen-sizeof(BASEDIR));
	droot[dlen-1] = 0;

	for (;i<len&&req[i]&&req[i]!='\r'&&req[i]!='\n';i++);
	if (i+2>len) return (err400 ());
	qstring = tmp = (!req[i]?req+i+1:NULL);
	req[i++] = 0;
	if (tmp) while (i<len&&req[i++]!='\n');
	while (i<len&&req[i++]!='\n');
	environ = alloca ((envlinenum+linecount+23) * sizeof (char *));
	set_global_env ();
	ulen = strlen (url);
	arg0 = alloca (sizeof (BASEDIR) + ulen + 1);
	memcpy (arg0, BASEDIR, sizeof (BASEDIR) - 1);
	memcpy (arg0+sizeof(BASEDIR)-1, url, ulen);
	arg0[ulen+sizeof(BASEDIR)-1] = 0;
	if (tmp && !strchr (tmp, '=')) {
		size_t ccnt = count_params (tmp) + 1, x = 0, xlen;
		for (xlen=0;tmp[xlen]!='\r'&&tmp[xlen]!='\n';xlen++);
		tmp = alloca (xlen+1);
		memcpy (tmp, qstring, xlen);
		tmp[xlen] = 0;
		args = alloca ((3+ccnt) * sizeof (char *));
		if (interpreter) args[x++] = interpreter;
		args[x++] = arg0;
		while (ccnt--) {
			args[x++] = tmp;
			while (*tmp != '+' && *tmp > ' ') tmp++;
			*tmp++ = 0;
		}
		args[x] = NULL;
	} else {
		size_t x = 0;
		args = alloca (3 * sizeof (char *));
		if (interpreter) args[x++] = interpreter;
		args[x++] = arg0;
		args[x] = NULL;
	}
	if (interpreter && -1 == access (interpreter, X_OK))
	 switch (errno) {
	  case ENOENT: case EISDIR: return (err404 ());
	  case EACCES: case EPERM:  return (err403 ());
	  default: return (err500 ());
	 }
	if (-1 == stat (url, &st)) {
		size_t len = strlen (url), old = 0;
		int x = len;
		char *pi, *pt;
		if (x)
		 do {
			while (url[x] != '/' && --x > 0);
			if (x <= 0) break;
			if (url[x] == '/') {
				if (old) url[old] = '/';
				old = x;
				url[x] = 0;
			}
		} while (-1 == stat (url, &st));
		if (!interpreter && S_ISDIR (st.st_mode)) return (err404 ());
		if (!interpreter) args[0] = url;
		if (x <= 0)
		 switch (errno) {
			 case ENOENT: case EISDIR: return (err404 ());
			 case EACCES: case EPERM:  return (err403 ());
			 default: return (err500 ());
		 }
		pi = alloca (len-old+1);
		*pi = '/';
		memcpy (pi+1, url+x+1, len-old-1);
		pi[len-old] = 0;
		pt = alloca (dlen+len-old+1);
		memcpy (pt, droot, dlen);
		memcpy (pt+dlen-1, pi, len-old);
		pt[len-old+dlen-1] = 0;
		put_env ("PATH_INFO", pi);
		put_env ("PATH_TRANSLATED", pt);
	}
	if (!(interpreter||((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP))))
		return (err403 ());
	if (interpreter && -1 == stat (interpreter, &st)) return (err500 ());
	if ((tmp = extract_header ("Content-Length: ")) &&
			(*tmp>='0'&&*tmp<='9'))
		put_env ("CONTENT_LENGTH", tmp);
	if ((tmp = extract_header ("Content-Type: ")))
		put_env ("CONTENT_TYPE", tmp);
	put_env ("SERVER_SOFTWARE", SERVER);
	if (qstring) put_env ("QUERY_STRING", qstring);
	if (ruri && *ruri) put_env ("REQUEST_URI", ruri);
	if (sname) put_env ("SCRIPT_NAME", sname);
	switch (type) {
	 case 0: meth = "SAV"; break;
	 case 1: meth = "GET"; break;
	 case 2: meth = "POST"; break;
	 case 3: meth = "HEAD"; break;
	}
	put_env ("REQUEST_METHOD", meth);
	put_env ("DOCUMENT_ROOT", droot);
	put_env ("SERVER_NAME", droot+sizeof(BASEDIR)-1);
	putipaddr (saddr->sin_addr.s_addr, tmpaddr);
	put_env ("REMOTE_ADDR", tmpaddr);
	while (i<len) {
		if (req[i]>' ' && !good_header (req+i))
			put_http_env (req+i);
		while (i<len&&req[i++]!='\n');
	}
	environ[cp] = NULL;

	read_domain_config (url, &cd, &root);

	if (!(pid = vfork ())) {
		if (root && !chdir (root)) chroot (".");
		else if (cd) chdir (cd);
		dup2 (fd, 0);
		dup2 (fd, 1);
		dup2 (fd, 2);
		if (st.st_gid) setregid (st.st_gid, st.st_gid);
		if (st.st_uid) setreuid (st.st_uid, st.st_uid);
		execve (args[0], args, environ);
		_exit (23);
	}
	if (-1 == pid)
		return (err500 ());
	return (1);
}

void
sigchld (sig)
{
	while ((waitpid (-1, NULL, WNOHANG)) > 0);
}

static struct pollfd pfds[MAX_CONNS];

static void
add_proc ()
{
	int i;
	for (i=0;i<MAX_CONNS;i++)
		if (-1 == pfds[i].fd) break;
	if (-1 == (pfds[i].fd = accept (sockfd, NULL, NULL)))
		err2 ("accept");
	pfds[i].events = POLLIN;
	proccnt++;
}

static void
select_loop ()
{
	size_t i, pc = proccnt;
	char buffer[0x1000];

	if (proccnt < MAX_CONNS) {
		pfds[0].fd = sockfd;
		pfds[0].events = POLLIN;
	}

	while (1) {
		if (poll (pfds, proccnt+1, -1) <= 0) return;

		for (i=1;i<pc+1;i++) {
		 if (pfds[i].fd != -1 && pfds[i].revents & POLLIN) {
			ssize_t ret = read (pfds[i].fd, buffer, sizeof(buffer));
			if (ret > sizeof (struct sockaddr_in)) {
			  struct sockaddr_in *saddr =
				(struct sockaddr_in *) buffer;
			  exec_request (buffer+sizeof(*saddr), saddr,
				ret-sizeof (*saddr), pfds[i].fd);
			}
			proccnt--;
			close (pfds[i].fd);
			pfds[i].fd = -1;
		 }
		}
		if (pfds[0].revents & POLLIN)
			add_proc ();
		break;
	}
}

static void
do_reinit ()
{
	need_reinit = 1;
}

int
main (argc, argv)
	int argc;
	char **argv;
{
#ifdef DEBUG
	struct sockaddr_in saddr = { AF_INET, htons (1235), { 0 } };
	static char request[] = "\1fnord.at/cgi-bin/test-cgi\r\nHost: fnord.at\r\n\r\n";
#endif
	struct sigaction sa;

	sa.sa_handler = sigchld;
	sigaction (SIGCHLD, &sa, NULL);
	sa.sa_handler = do_reinit;
	sigaction (SIGHUP, &sa, NULL);
	sa.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &sa, NULL);
#ifdef DEBUG
	sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind (sockfd, (struct sockaddr *) &saddr, sizeof (saddr));
	if (-1 == listen (sockfd, 25))
		die2 ("listen");
#else
	sockfd = 0;
#endif
	read_environ ();
#ifndef DEBUG
	memset (&pfds, -1, sizeof (pfds));
	while (1)
		select_loop ();
#else
	chdir (BASEDIR);
	printf ("%d\n", exec_request (request, NULL, sizeof (request) - 1, 1));
#endif
	return (0);
}
