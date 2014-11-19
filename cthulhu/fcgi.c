#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include "fastcgi.h"
#include "xalloc.h"
#include "client.h"
#include "auth.h"
#include "http.h"
#include "fcgi.h"

static char *buffer;
static size_t buflen;
static size_t bufp;

static void
fcgi_puts (str, len)
	char *str;
	size_t len;
{
	if (bufp+len > buflen) buffer = xresize (buffer, (buflen += len+0x200));
	memcpy (buffer+bufp, str, len);
	bufp += len;
}

static int
fcgi_flush (fd)
	int fd;
{
	int ret = write (fd, buffer, bufp);
	bufp = 0;
	return (ret);
}

static void
fcgi_begin_request (type)
	char type;
{
	static char req[] = {
		FCGI_VERSION_1, FCGI_BEGIN_REQUEST,
		0, 1, 0, 8, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0
	};
	req[9] = type;
	fcgi_puts (req, sizeof (req));
}

static void
fcgi_end_request (type)
	char type;
{
	static char req[] = {
		FCGI_VERSION_1, 0, 0, 1, 0, 0, 0, 0
	};
	req[1] = type;
	fcgi_puts (req, sizeof (req));
}

static void
fcgi_setenv (name, value, namelen, vlen)
	char *name, *value;
	size_t namelen, vlen;
{
	static char req[] = {
		FCGI_VERSION_1, FCGI_PARAMS, 0, 1, 0, 0, 0, 0, 0, 0
	};
	size_t xlen = vlen + namelen + 2;
	req[4] = xlen/100; req[5] = xlen%100;
	req[8] = namelen;  req[9] = vlen;
	fcgi_puts (req, sizeof (req));
	fcgi_puts (name, namelen);
	fcgi_puts (value, vlen);
}

static void
fcgi_init ()
{
	if (!buffer) buffer = xalloc ((buflen = 0x200));
	bufp = 0;
}

static int
do_write (sockfd, buffer, len)
	int sockfd, len;
	char *buffer;
{
	struct iovec iov[2];
	static char req[] = {
		FCGI_VERSION_1, FCGI_STDIN, 0, 1, 0, 0, 0, 0
	};
	int ret;

	req[4] = len/0x100; req[5] = len%0x100;

	iov[0].iov_base = req;
	iov[0].iov_len  = sizeof (req);
	iov[1].iov_base = buffer;
	iov[1].iov_len  = len;

	while ((ret = writev (sockfd, iov, 2)) == -1 && errno == EINTR);
	return (ret != -1);
}

static int
do_read (cl)
	client_t *cl;
{
	unsigned char buffer[0x200];
	ssize_t ret = read (cl->xfd, buffer, sizeof (buffer));
	size_t cp = 0;

	if (!ret && (cl->xop & 0x10))
		return (send_err (cl, ERR500));

	if (ret <= 0) return (ret && (EINTR == errno || EAGAIN == errno));

	if (cl->rem) {
		size_t len = cl->rem>(size_t)ret?ret:cl->rem;
		ssize_t xret = write (cl->sockfd, buffer, len);
		cl->rem -=len;
		if (xret == -1 || xret < len ) {
			//Fix me, we have lost some output data ...
			return (EINTR == errno || EAGAIN == errno);
		}
		if(!cl->rem)
			cp = len;
		else
			return 1;
	}
	while (cp < (size_t) ret) {
		ssize_t xlen = buffer[cp+4]<<8|buffer[cp+5];
		if (xlen <= 8 || xlen > 0xffff) break;
		if (FCGI_VERSION_1 != buffer[cp]) return (0);
		if (FCGI_STDOUT == buffer[cp+1]) {
			if (cl->xop & 0x10) {
			   cl->xop &=~0x10;
			   if (!memcmp (buffer+cp+8, "Status: ", 8)) {
				entry_t e;
				size_t len = 8;
				if (cl->xop & 0x20) {
					cl->xop &= ~20;
					if (buffer[cp+16]=='2') {
					  cl->hook = NULL;
					  return (schedule_pathconfs (cl));
					}
				}
				e.str = buffer+cp+7;
				memcpy ((char *) e.str, "HTTP/1.0 ", 8);
				while (ret-cp>len&&e.str[len++]!='\n');
				e.len = len;
				cp += len-1;
				xlen -= len-1;
				send_err (cl, &e);
			     } else send_answer (cl, OK200);
			     shutdown (cl->xfd, SHUT_WR); /*XXX*/
			}
			if (cp+xlen > (size_t) ret) {
				write (cl->sockfd, buffer+cp+8, ret-cp-8);
				cl->rem = xlen-(ret-cp-8);
				cl->writing = 1;
				cl->xop = ((cl->xop | 1) &~ 2);
				break;
			} else
			if (-1 == write (cl->sockfd, buffer+cp+8, xlen)) {
#if 0
			   if (errno == EAGAIN || errno == EINTR) {
				memcpy (cl->iobuffer, buffer+cp+8, ret-cp-8);
				cl->rem = xlen;
				cl->xop |= 0x00ff;
				break;
			   }
#endif
			   return (0);
			}
		}
		if ((cp += xlen) > (size_t) ret) return (0);
	}
	return (1);
}

static int
fcgi_read_write (cl)
	client_t *cl;
{
	if (!cl->iobuffer) cl->iobuffer = xalloc (PIPE_BUF_SIZE);

	if (cl->clength) {
	   if (!cl->writing) {
		cl->rem = read (cl->sockfd, cl->iobuffer, PIPE_BUF_SIZE);
		if (!cl->rem) return (0);
		if (-1 == (ssize_t) cl->rem)
			return (EINTR == errno || EAGAIN == errno);
		cl->writing = 1;
		cl->xop |= 2;
	   }
	   if (cl->writing) {
		if (-1 == do_write (cl->xfd, cl->iobuffer, cl->rem))
			return (EINTR == errno || EAGAIN == errno);
		if (cl->clength < cl->rem) cl->clength = 0;
		else cl->clength -= cl->rem;
		cl->rem     = 0;
		cl->writing = 0;
		if(cl->clength)
			cl->xop &= ~2;
		else
			cl->xop |= 2;

	   }
	}
	if (!cl->clength) {
		return (do_read (cl));
	}
	return (1);
}

#define xfcgi_setenv(a,b,c) fcgi_setenv (a,b,sizeof(a)-1,c)
#define extract_header(a,b) _extract_header (a,b,sizeof (b) - 1)

static int
try_get_post (cl)
	client_t *cl;
{
	char *str = cl->buf;
	size_t len = cl->ilen, i = 0;
	
	if (!cl->iobuffer) cl->iobuffer = xalloc (PIPE_BUF_SIZE);
	
	while (i<len) {
		if (str[i]=='\n' && str[i+1]<= ' ') {
			i++;
			while (i<len&&str[i++] != '\n');
			if (i<len) {
				if(len-i > PIPE_BUF_SIZE)
					return -1;
				memcpy (cl->iobuffer,cl->buf+i, len-i);
				cl->rem = len-i;
				return 1;
			}
			return 0;
		}
		while (i<len&&str[++i]!='\n');
	}
	return 0;
}

int
send_fcgi (cl)
	client_t *cl;
{
	size_t x;
	char *meth, *droot = alloca (sizeof (BASEDIR) + cl->domain->dlen);

	fcgi_init ();
	fcgi_begin_request (FCGI_RESPONDER);

	/* Set environment variables */
	if (cl->clength) {
		char buf[0x20], *s = buf;
		size_t len = cl->clength, i = sizeof (buf);
		do (s[--i] = len%10+'0'); while ((len/=10)&&i);
		xfcgi_setenv ("CONTENT_LENGTH", s+i, sizeof (buf) - i + 1);
	}
	for (x=0;x<cl->urlp;x++)
	   if (!cl->url[x]) {
		xfcgi_setenv ("QUERY_STRING", cl->url+x+1, cl->urlp-x-1);
		break;
	   }
	xfcgi_setenv ("SCRIPT_URL", cl->url+cl->domain->dlen, x-cl->domain->dlen+1);
	xfcgi_setenv ("SERVER_SOFTWARE", SERVER, sizeof (SERVER) - 1); /*XXX*/
	switch (cl->rt) {
	  case HEAD: meth = "HEAD"; break;
	  case GET:  meth = "GET";  break;
	  case POST: meth = "POST"; break;
	  default:   meth = "UNKNOWN"; break;
	}
	xfcgi_setenv ("REQUEST_METHOD", meth, strlen (meth)); /*STUPID*/

	memcpy (droot, BASEDIR, sizeof (BASEDIR) - 1);
	memcpy (droot + sizeof (BASEDIR) - 1, cl->domain->name,
		cl->domain->dlen);
	xfcgi_setenv ("DOCUMENT_ROOT", droot, sizeof (BASEDIR)+cl->domain->dlen);
	xfcgi_setenv ("SERVER_PROTOCOL", "HTTP/1.0", 8);
	fcgi_end_request (FCGI_PARAMS);

	if (-1 == fcgi_flush (cl->xfd))
		return (send_err (cl, ERR500));
	cl->writing = 0;
	cl->hook = fcgi_read_write;
	if(cl->clength){
		if(try_get_post(cl)>=0)
			cl->writing = 1;
		else
			return (send_err (cl, ERR500));
	}
	return (1);
}

int
do_fcgi_auth (cl)
	client_t *cl;
{
	char *str = extract_header (cl, "Authorization: ");
	char *buf = NULL;
	size_t len = 0, x = 0;

	if (str) {
		while (str[len]>' ') len++;
		if (len < 7) return (send_err (cl, ERR400));
		buf = alloca (len+1);
		if (!rdec (str+6, buf)) return (send_err (cl, ERR400));
		while (x<len&&buf[x]!=':') x++;
		if (buf[x] != ':') return (send_err (cl, ERR400));
		buf[x] = 0;
	}
	fcgi_init ();
	fcgi_begin_request (FCGI_AUTHORIZER);
	if (buf) {
		xfcgi_setenv ("REMOTE_PASSWD", buf+x+1, strlen (buf+x+1));
		xfcgi_setenv ("REMOTE_USER", buf, x);
	}
	fcgi_end_request (FCGI_PARAMS);
	if (-1 == fcgi_flush (cl->xfd))
		return (send_err (cl, ERR500));
	cl->writing = 0;
	cl->xop |= 0x20;
	cl->hook = fcgi_read_write;
	return (1);
}
