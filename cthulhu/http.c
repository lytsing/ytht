/*
   Copyright (C) 2002, 2003 Thomas M. Ogrisegg

   Cthulhu HTTP main module
 */

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include "config.h"
#include "client.h"
#include "file.h"
#include "domain.h"
#include "string.h"
#include "misc.h"
#include "log.h"
#include "http.h"
#include "xalloc.h"
#include "sendfile.h"
#include "quota.h"
#include "auth.h"
#include "cgi.h"
#include "string.h"

#define ENTRY(x) { HTTP_VER" "x"\r\n", sizeof (x) + 10 }

entry_t err_entries[] = {
	ENTRY ("200 OK"),
	ENTRY ("204 No content"),
	ENTRY ("206 Partial Content"),
	ENTRY ("301 Permanently moved"),
	ENTRY ("304 Not modified"),
	ENTRY ("400 Bad request"),
	ENTRY ("401 Authorization required"),
	ENTRY ("403 Forbidden"),
	ENTRY ("404 Not found"),
	ENTRY ("405 Method not allowed"),
	ENTRY ("406 Requested range not satisfiable"),
	ENTRY ("500 Internal Server error"),
	ENTRY ("503 Service unavailable")
};

#undef ENTRY

#define NENTRY { NULL, 0 }
#define ENTRY(x,y) { "<html><head><title>"x"</title></head>\r\n<body>"y"</body></html>\r\n", 61+sizeof(x)+sizeof(y)-5 }

entry_t err_bodies[] = {
	NENTRY,
	NENTRY,
	NENTRY,
	NENTRY,
	NENTRY,
	ENTRY ("400 Bad Request", "Your browser issued an request which could not be understand by this server"),
	ENTRY ("401 Unauthorized", "You have to be properly authentificated before you can access the requested pages"),
	ENTRY ("403 Forbidden", "You are not allowed to view the requested page/s"),
	ENTRY ("404 Not found", "The URL you requested could not be found on this server"),
	ENTRY ("405 Method not allowed", "Your browser is stupid and mixed it's headers up"),
	ENTRY ("406 Range not satisfiable", "Your HTTP client is fucking stupid and requested an incorrect range"),
	ENTRY ("500 Internal server error", "Your request could not be satisfied due to an internal server error. Consult the logfiles or the administrator of the server to fix this"),
	ENTRY ("503 service unavailable", "Server overload. Try again later.")
};

#undef ENTRY
#define ENTRY(x) { x, sizeof (x) - 1 }

static entry_t headers[] = {
	ENTRY ("Host: "),
	ENTRY ("Connection: "),
	ENTRY ("Range: "),
	ENTRY ("Accept-Encoding: "),
	ENTRY ("If-Modified-Since: "),
	ENTRY ("Authorization: "),
	ENTRY ("Content-Length: ")
};

#define CONN_CLOSE "Connection: close\r\n"
#define CONN_KEEPA "Connection: Keep-Alive\r\n"
#define HDR_SERVER "Server: "SERVER"\r\n"
#define HDR_DATE   "Date: "

static string_t obuf;

#define xputs(x) as_puts (&obuf, x, sizeof (x) - 1)
#define xput(x,y) as_puts (&obuf, x, y)
#define flush_answer(cl) write (cl->sockfd, obuf.str, obuf.off);

static int retr_file __P ((client_t *));

static void
put_answer (cl, entry)
	client_t *cl;
	entry_t *entry;
{
	obuf.off = 0;
	xput (entry->str, entry->len);
	xputs (HDR_SERVER);
	xputs (HDR_DATE);
	__date (&obuf, curtime);
	if (cl->keepalive) xputs (CONN_KEEPA); else xputs (CONN_CLOSE);
//	log_req (cl->domain, cl->buf, obuf.str+9, &cl->saddr, cl->ilen);
}

int
send_err (cl, entry)
	client_t *cl;
	entry_t *entry;
{
	entry_t *body = NULL;
	if (entry > err_entries && entry < err_entries + 13)
		body = err_bodies + (entry - err_entries);
	cl->keepalive = 0;
	cl->hook = NULL;
	put_answer (cl, entry);
	xputs ("Content-Type: text/html\r\n\r\n");
	if (body) xput (body->str, body->len);
	flush_answer (cl);
	return (0);
}

int
send_answer (cl, entry)
	client_t *cl;
	entry_t *entry;
{
	put_answer (cl, entry);
	if(OK200!=entry)
		xput ("\r\n", 2);
	flush_answer (cl);
	return (1);
}

static size_t
slen (str)
	char *str;
{
	size_t res = 0;
	while (str[res]>' ') res++;
	return (res);
}

static int
rewrite_url (cl)
	client_t *cl;
{
	pathconf_t *pc = cl->domain->pathconfs;
	size_t i;
	char *dst  = cl->url + cl->domain->dlen;
	if (pc)
	 for (i=0;i<cl->domain->pcount;i++,pc++) {
	   if (pc->type == PC_REWRITE && pc->path && pc->pathlen < cl->urlp)
		if (!memcmp (pc->path, dst, pc->pathlen)) {
			if (pc->plen1+cl->urlp > sizeof (cl->url))
				continue;
			if (pc->pathlen == pc->plen1)
				memcpy (dst, pc->p1, pc->plen1);
			else if (pc->pathlen > pc->plen1) {
				memmove (dst, dst+pc->pathlen-pc->plen1,
					cl->urlp-pc->pathlen-pc->plen1+1+
					cl->domain->dlen);
				memcpy (dst, pc->p1, pc->plen1);
				return (1);
			} else {
				memmove (dst+pc->plen1, dst+pc->pathlen,
					cl->urlp-pc->pathlen);
				memcpy (dst, pc->p1, pc->plen1);
			}
		}
	 }
	return (0);
}

static int
do_redirect (cl, url)
	client_t *cl;
	char *url;
{
	put_answer (cl, ERR301);
	xputs ("Location: ");
	xput (url, strlen (url));
	xputs ("\r\n\r\n");
	flush_answer (cl);
	return (1);
}

static int
redirect (cl)
	client_t *cl;
{
	pathconf_t *pc = pc_generic_wildmat (PC_REDIR, cl->url+cl->domain->dlen,
		cl->domain);
	if (!pc) return (0);
	cl->keepalive = 0;
	do_redirect (cl, pc->p1);
	return (1);
}

int
no_auth (cl, body)
	client_t *cl;
	char *body;
{
	cl->keepalive = 0;
	cl->hook = NULL;
	put_answer (cl, ERR401);
	xputs ("WWW-Authenticate: Basic realm=\"");
	if (body) xput (body, strlen (body));
	xputs ("\"\r\n\r\n");
	flush_answer (cl);
	return (1);
}


enum { AUTH, REDIR, CGI };

int
schedule_pathconfs (cl)
	client_t *cl;
{
	if (cl->xfd) { close (cl->xfd); cl->xfd = cl->xop = 0; }
	if (cl->status == AUTH) {
		cl->status++;
		if (need_auth (cl, cl->domain->dlen)) return (1);
	}
	if (cl->status == REDIR) {
		cl->status++;
		if (redirect (cl, cl->domain->dlen)) return (1);
	}
	if (cl->status == CGI) {
		cl->status++;
		if (exec_cgi (cl, cl->domain->dlen)) return (1);
	}
	cl->ilen = 0;
	return (retr_file (cl));
}

static void
reset_client (cl)
	client_t *cl;
{
	obuf.off = 0;
	cl->keepalive = 0;
#ifdef WANT_GZIP_ENCODING
	cl->gzip = 0;
#endif
	cl->writing = 0;
	cl->file_start = 0;
	cl->file_end = 0;
	cl->pc = NULL;
	cl->lastmod = 0;
	return;
}

#define ccmp3(x) ((req[0]|0x20)==(x[0]|0x20) && (req[1]|0x20)==(x[1]|0x20) && (req[2]|0x20) == (x[2]|0x20))
#define ccmp4(x) ((req[0]|0x20)==(x[0]|0x20) && (req[1]|0x20)==(x[1]|0x20) && (req[2]|0x20) == (x[2]|0x20) && (req[3]|0x20) == (x[3]|0x20))

#define invalid_request() return (send_err (cl, ERR400))
#define scmp(x,y) !memcmp (x,y,sizeof(y)-1)

static inline char
GET_HEX(x) char x;
{
	return (x>'9'?((x|0x20)-'a'+10):(x-'0'));
}

#define ERR(x) (-1 == (ssize_t)(x))

int
parse_http_request (cl)
	client_t *cl;
{
	char *req = cl->buf;
	size_t len = cl->ilen, i = 4, x;
	int qm = 0;

	reset_client (cl);
	cl->clength = 0;
	cl->domain = &default_domain;

	if (ccmp3 ("GET")) { cl->rt = GET; i--; }
	else if (ccmp4 ("HEAD")) cl->rt = HEAD;
	else if (ccmp4 ("POST")) cl->rt = POST;
	else invalid_request ();

	if (req[i]>' '||len<6) invalid_request ();
	while (i<len&&req[i]!='\n'&&req[i]<=' ') i++;
	if ((req[i]|0x20) == 'h' && casecmp (req+i, "http://", 7)) {
	  size_t l = i+7;
	  while (req[l] != ':' && req[l] != '/' && req[l] > ' ') l++;
	  cl->domain = find_domain (req+i+7, l-i-7);
	  i = l;
	}
	if (req[i] != '/') invalid_request ();

	for (x=i;x<len;) {
	  size_t z;
	  if (req[x]<=' ') break;
	   for (z=0;z<sizeof (headers) / sizeof (headers[0]);z++)
		if (req[x]==headers[z].str[0] &&
			!memcmp (headers[z].str, req+x, headers[z].len)) {
		  x += headers[z].len;
		  switch (z) {
		   case 0:
			cl->domain = find_domain (req+x, slen (req+x));
			break;
		   case 1:
			cl->keepalive = casecmp (req+x, "Keep-Alive", 10);
			break;
		   case 2:
			if (scmp (req+x, "bytes=")) {
				x += 6;
				if (ERR(cl->file_start = scanstr (req+x))) {
					cl->file_start = 0;
					break;
				}
				while (req[x]!='-'&&req[x]!='\n') x++;
				if (ERR(cl->file_end = scanstr (req+x+1)))
					cl->file_end = 0;
			}
			break;
#ifdef WANT_GZIP_ENCODING
		   case 3:
			while (x<len) {
			  if (req[x]=='g' && scmp (req+x, "gzip")) {
				  cl->gzip = 1;
				  break;
			  }
			  while (req[x]!='\n'&&req[x++]!=',');
			  if (req[x]=='\n') break;
			  x++;
			}
			break;
#endif
		   case 4:
			if (ERR(cl->lastmod = parse_time_date (req+x)))
				cl->lastmod = 0;
			break;
		   case 6:
			if (ERR(cl->clength = scanstr (req+x)))
				cl->clength = 0;
			break;
		   }
		  break;
		}
	   while (x<len&&req[x++]!='\n');
	}

	if ((!cl->clength && cl->rt == POST) ||
		(cl->clength && cl->rt != POST))
		invalid_request ();

	if (cl->rt == POST && !may_post (cl))
		return (send_err (cl, ERR503));

	x = cl->domain->dlen;
	if (x >= sizeof (cl->url)) return (send_err (cl, ERR500)); /*XXX*/
	memcpy (cl->url, cl->domain->name, x);
	for (;x<sizeof (cl->url)-1 && i<len && req[i]>' ';x++,i++) {
		if (req[i] == '?' && !qm) { cl->url[x] = 0; qm=1; }
		else if ('%' == req[i])
			cl->url[x] = (GET_HEX(req[++i])<<4)+GET_HEX(req[++i]);
		else cl->url[x] = req[i];
		if (cl->url[x]=='.'&&cl->url[x-1]=='.'&&cl->url[x-2]=='/')
			invalid_request ();
	}

	if (x == sizeof (cl->url) - 1)
		invalid_request ();

	cl->url[(cl->urlp = x)] = 0;

	while (i<len&&req[i]<=' '&&req[i]!='\n') i++;
	if (memcmp (req+i, "HTTP/", 5)) invalid_request ();
	i += 5;
	if ((cl->httpver = (req[i]-'0')*10+req[i+2]-'0') >= 11)
		cl->keepalive = 1;

	if (cl->domain->pathconfs) {
		rewrite_url (cl);
		cl->status = 0;
		schedule_pathconfs (cl);
		return (1);
	}
	return (retr_file (cl));
}

char ct_sum[] = {
	0x50, 0x68, 0x27, 0x6e, 0x67, 0x6c, 0x75, 0x69, 0x20, 0x6d, 0x67, 0x6c,
	0x77, 0x27, 0x6e, 0x61, 0x66, 0x68, 0x20, 0x43, 0x74, 0x68, 0x75, 0x6c,
	0x68, 0x75, 0x20, 0x52, 0x27, 0x6c, 0x79, 0x65, 0x68, 0x20, 0x77, 0x67,
	0x61, 0x68, 0x27, 0x6e, 0x61, 0x67, 0x6c, 0x20, 0x66, 0x68, 0x74, 0x61,
	0x67, 0x6e
};

static int
retr_file (cl)
	client_t *cl;
{
	size_t dlen = cl->domain->dlen;

	if (cl->rt == POST)
		return (send_err (cl, ERR405));

	if (cl->url[cl->urlp-1] == '/' && cl->urlp+15 < sizeof (cl->url)) {
		memcpy (cl->url+cl->urlp, "index.html", 10);
		cl->urlp += 10;
	}
#ifdef WANT_GZIP_ENCODING
	if (cl->gzip && cl->urlp+4 < sizeof (cl->url)) {
		cl->url[cl->urlp++] = '.'; cl->url[cl->urlp++] = 'g';
		cl->url[cl->urlp++] = 'z'; cl->url[cl->urlp] = 0;
	}
#endif

retry_open:
	if (!(cl->file = find_file (cl->url+dlen+1, cl->domain, cl->url))) {
	  entry_t *e;
	  switch (errno) {
	   case EPERM: case EACCES:
		e = ERR403;
		break;
	   case ENOENT:
		e = ERR404;
#ifdef WANT_GZIP_ENCODING
		if (cl->gzip) {
			cl->url [cl->urlp-=3] = cl->gzip = 0;
			goto retry_open;
		}
#endif
		break;
	   case EISDIR:
		if (cl->urlp+2 < sizeof (cl->url)) {
			cl->url[cl->urlp] = '/'; cl->url[cl->urlp+1] = 0;
			return (do_redirect (cl, cl->url+dlen));
		}
	   default:
		e = ERR500;
		break;
	  }
	  return (send_err (cl, e));
	} else {
		struct iovec iov[3];
		ssize_t ret, rem = 0;
		size_t iovhdr = 2;

		cl->flags = 0;
		if (cl->file->mtime <= cl->lastmod) {
			http_close_file (cl->file);
			cl->file = NULL;
			return (send_answer (cl, ERR304));
		}
		if (!cl->file_end) cl->file_end = cl->file->len;
		if (cl->file_start || cl->file_end != cl->file->len) {
		  char *s;
		  size_t cllen = cl->file->header.off;
		  if (cl->file_start >= cl->file->len ||
			cl->file_end >  cl->file->len ||
			cl->file_start > cl->file_end) {
			http_close_file (cl->file);
			cl->file = NULL;
			return (send_err (cl, ERR406));
		  } else put_answer (cl, OK206);
		  xputs ("Accept-Range: bytes\r\nContent-Range: bytes ");
		  as_putlong (&obuf, cl->file_start);
		  as_putc (obuf, '-');
		  as_putlong (&obuf, cl->file_end);
		  as_putc (obuf, '/');
		  as_putlong (&obuf, cl->file->len);
		  /*XXX: Do not send multiple Content-Length header */
		  xputs ("\r\nContent-Length: ");
/*		  cl->file_end++;*/
		  as_putlong (&obuf, cl->file_end - cl->file_start);
		  xputs ("\r\n");
		  /*UGLY: We need to parse our selfmade headers ... */
		  s = cl->file->header.str;
		  while (cllen > 0 && s[cllen] != 'C') cllen--;
		  as_puts (&obuf, s, cllen);
		  iovhdr--;
		  xputs ("\r\n");
		} else put_answer (cl, OK200);
#ifdef WANT_GZIP_ENCODING
		if (cl->gzip) xputs ("Content-Encoding: gzip\r\n");
#endif
		iov[0].iov_base = obuf.str; rem = iov[0].iov_len = obuf.off;
		if (iovhdr == 2) {
			iov[1].iov_base = cl->file->header.str;
			rem += (iov[1].iov_len = cl->file->header.off);
		}
		if (cl->rt == HEAD) {
			writev (cl->sockfd, iov, iovhdr);
			cl->ilen = 0;
			http_close_file (cl->file);
			cl->file = NULL;
			return (1);
		}
#ifdef WANT_SENDFILE
		if (-1 == (ret = xsendfile (cl, cl->file_start,
			cl->file_end - cl->file_start, iov, iovhdr))) {
			log_intern_err ("sendfile failed");
			return (0);
		}
		rem += cl->file_end - cl->file_start;
#else
		iov[iovhdr].iov_base = cl->file->start + cl->file_start;
		rem += iov[iovhdr].iov_len = cl->file_end - cl->file_start;
		if (-1 == (ret = writev (cl->sockfd, iov, iovhdr+1))) {
			log_intern_err ("writev failed");
			return (0);
		}
#endif

		if (!(cl->rem = rem-ret)) {
			http_close_file (cl->file);
			cl->file = NULL;
		} else cl->writing = 1;

		cl->ilen = 0;
		return (1);
	}
}
