#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "domain.h"
#include "client.h"
#include "config.h"
#include "pathconf.h"
#include "log.h"
#include "http.h"
#include "xalloc.h"
#include "cgi.h"

static size_t
have_double_nl (cl)
	client_t *cl;
{
	char *str = cl->iobuffer;
	size_t i;
	for (i=0;i<cl->rem;) {
		if (str[i] <= ' ') return (i+1);
		while (i<cl->rem&&str[i++]!='\n');
	}
	return (0);
}

static int
cgi_hook (cl)
	client_t *cl;
{
	ssize_t ret;

	if (!cl->iobuffer) cl->iobuffer = xalloc (PIPE_BUF_SIZE);
	if (cl->clength) {
	   if (!cl->writing) {
		cl->rem = read (cl->sockfd, cl->iobuffer, PIPE_BUF_SIZE);
		if (!cl->rem) return (0);
		if (-1 == (ssize_t) cl->rem)
			return (EINTR == errno || EAGAIN == errno);
		if (cl->clength < cl->rem) cl->clength = 0;
		else cl->clength -= cl->rem;
		cl->writing = 1;
		if (cl->clength) cl->xop &= ~2;
	   }
	   if (cl->writing) {
		ret = write (cl->xfd, cl->iobuffer, cl->rem);
		if (-1 == ret) {
			if (EINTR == errno || EAGAIN == errno) return (1);
			return (send_err (cl, ERR500));
		}
		cl->writing = 0;
		if (cl->clength) cl->xop &= ~2;
		else cl->xop |= 2;
	   }
	} else
	if (!cl->clength) {
	   if (!cl->writing) {
		size_t xlen;
		ret = read (cl->xfd, cl->iobuffer + cl->rem,
			PIPE_BUF_SIZE - cl->rem - 1);
		if (cl->xop & 16) {
		  if (cl->rt == POST && !ret && !cl->rem) {
			cl->hook = NULL;
			send_answer (cl, OK204);
			return (0);
		  }
		  if (!cl->rem && ret <= 1) {
			entry_t *err = ERR500;
			if (ret == 1)
			  switch (cl->iobuffer[0]) {
				case 1:  err = ERR400; break;
				case 2:  err = ERR403; break;
				case 3:  err = ERR404; break;
				case 4:  break;
				default: goto out;
			  }
			  return (send_err (cl, err));
		   }
out:
		   cl->rem += ret;
		   if ((xlen = have_double_nl (cl)) || !ret) {
			size_t i, x301 = 0;
			char *status = NULL;
			entry_t e, *ex = OK200;
			for (i=0;i<cl->rem;i++) {
				if (cl->iobuffer[i] <= ' ') break;
				if (!memcmp (cl->iobuffer+i, "Status: ", 8))
					status = cl->iobuffer+i+7;
				else
				if (!memcmp (cl->iobuffer+i, "Location: ", 11))
					x301 = 1;
				while (i<cl->rem&&cl->iobuffer[i++]!='\n');
			}
			if (status) {
				size_t len = snlen (status);
				e.str = alloca (e.len=(len+sizeof(HTTP_VER)-1));
				memcpy ((char *) e.str, HTTP_VER, sizeof(HTTP_VER)-1);
				memcpy ((char *) (e.str+8), status, len);
				ex = &e;
				memmove (status-7, status+len, cl->rem-(status-cl->iobuffer+7));
				cl->rem -= len+7;
				xlen    -= len+7;
			} else if (x301) ex = ERR301;
			send_answer (cl, ex);
			if (cl->rt == HEAD) {
				write (cl->sockfd, cl->iobuffer, xlen);
				return (0);
			}
			cl->xop &= ~18;
			cl->writing = 1;
			shutdown (cl->xfd, SHUT_WR);
		   }
		   return (1);
		} else cl->rem = ret;
		if (!cl->rem) return (0);
		if (-1 == (ssize_t) cl->rem)
			return (EINTR == errno || EAGAIN == errno);
		cl->writing = 1;
		cl->xop &= ~2;
	   }
	   if (cl->writing) {
		ret = write (cl->sockfd, cl->iobuffer, cl->rem);
		if (-1 == (ssize_t) ret)
			 return (EINTR == errno || EAGAIN == errno);
		cl->xop |= 2;
		cl->writing = 0;
		cl->rem = 0;
	   }
	}
	return (1);
}

/*XXX: This is complete bullshit. */
static void
try_init_write (cl)
	client_t *cl;
{
	char *str = cl->buf;
	size_t len = cl->ilen, i = 0;
	while (i<len) {
		if (str[i]=='\n' && str[i+1]<= ' ') {
			i++;
			while (i<len&&str[i++] != '\n');
			if (i<len) {
				usleep (10);
				write (cl->xfd, cl->buf+i, len-i);
				cl->clength -= len-i;
			}
			return;
		}
		while (i<len&&str[++i]!='\n');
	}
}

static int
send_cgi (cl)
	client_t *cl;
{
	size_t i = 0, s1 = (cl->pc->p1?strlen (cl->pc->p1):0), x, cp;
	char *buffer = alloca (cl->ilen + sizeof (cl->url) + 12 + s1 +
		sizeof (cl->saddr));

	memcpy (buffer, &cl->saddr, sizeof (cl->saddr));
	i += sizeof (cl->saddr);
	buffer[i++] = cl->rt | (cl->pc->p1?4:0);
	if (cl->pc->p1) {
		memcpy (buffer+i, cl->pc->p1, s1);
		i += s1;
		buffer[i++] = ' ';
	}

	memcpy (buffer+i, cl->url, cl->urlp); i+=cl->urlp;
	buffer[i++] = '\r'; buffer[i++] = '\n';
	for (x=(cl->rt==GET?4:5);x<cl->ilen&&cl->buf[x]>' ';)
		buffer[i++] = cl->buf[x++];
	while (x<cl->ilen&&cl->buf[x]!='\r'&&cl->buf[x]!='\n') x++;
	x += cl->buf[x]=='\r'?2:1;
	cp = x;
	while (x<cl->ilen-1) {
		if (cl->buf[x]<=' ') {
			x += cl->buf[x]=='\r'?2:1;
			break;
		}
		while (x<cl->ilen&&cl->buf[x++]!='\n');
	}
	memcpy (buffer+i, cl->buf+cp, x-cp);

	if (-1 == write (cl->xfd, buffer, i+x-cp))
		return (send_err (cl, ERR500));

	if (cl->clength) try_init_write (cl);
	if (cl->clength) cl->xop &= ~2;
	cl->writing = 0;
	cl->rem  = 0;
	cl->hook = cgi_hook;
	return (1);
}

extern int send_fcgi __P ((client_t *));

int
exec_cgi (cl, len)
	client_t *cl;
	size_t len;
{
	if (!(cl->pc = pc_generic_wildmat (PC_CGI, cl->url+len, cl->domain)) &&
	    !(cl->pc = pc_generic_wildmat (PC_FCGI, cl->url+len, cl->domain)))
		return (0);
	if (!connect_to (cl, cl->pc)) {
		log_intern_err ("connect to cgi/fcgi daemon failed");
		send_err (cl, ERR500);
		return (1);
	}
	cl->xop = 18;
	cl->writing = 1;
	cl->hook = ((cl->pc->type == PC_FCGI) ? send_fcgi : send_cgi);
	return (1);
}
