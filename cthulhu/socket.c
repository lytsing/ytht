#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "domain.h"
#include "client.h"
#include "string.h"
#include "xalloc.h"
#include "log.h"
#include "http.h"
#include "misc.h"
#include "sendfile.h"
#include "socket.h"

client_t *clients, *lcl;
int filenum = 1;
int killconns;
time_t curtime;

client_t *
add_client (sockfd)
	int sockfd;
{
	client_t *cl = xmalloc (client_t);
	unsigned int sz = sizeof (cl->saddr);
	memset (cl, 0, sizeof (*cl));

	if ((cl->sockfd = accept (sockfd, (struct sockaddr *) &cl->saddr,
		&sz)) < 0) {
		xfree (cl, sizeof (*cl));
		log_intern_err ("accept failed");
		return (NULL);
	}
	if (-1 == fcntl (cl->sockfd, F_SETFL, O_NONBLOCK)) {
		xfree (cl, sizeof (*cl));
		log_intern_err ("could not set socket to nonblocking");
		return (NULL);
	}
#if MAX_CONN_PER_HOST
	{
		client_t *c = clients;
		int cnt = 0;
		if (c)
		 do if (c->saddr.sin_addr.s_addr == cl->saddr.sin_addr.s_addr) {
			if (MAX_CONN_PER_HOST == ++cnt) {
				close (cl->sockfd);
				xfree (cl, sizeof (*cl));
				return (NULL);
			}
		} while ((c = c->next));
	}
#endif
	if (!clients) clients = lcl = cl;
	else {
		lcl->next = cl;
		cl->prev = lcl;
		lcl = cl;
	}
	cl->lastop = curtime;
	filenum++;
	return (cl);
}

void
remove_client (cl)
	client_t *cl;
{
	if (cl->file) http_close_file (cl->file);
	close (cl->sockfd);
	if (cl->xfd) close (cl->xfd);
	if (lcl == cl) lcl = lcl->prev;
	if (clients == cl) clients = clients->next;
	if (cl->next) cl->next->prev = cl->prev;
	if (cl->prev) cl->prev->next = cl->next;
	if (cl->buf) xfree (cl->buf, cl->len);
	if (cl->iobuffer) xfree (cl->iobuffer, PIPE_BUF_SIZE);
	xfree (cl, sizeof (*cl));
	filenum--;
}

int
handle_socket (cl)
	client_t *cl;
{
	ssize_t ret;
	size_t i;

	if (cl->writing) {
		if (-1 == (ret = xwrite (cl))||!ret)
			return (EINTR==errno);
		if (!(cl->rem -= ret)) {
			http_close_file (cl->file);
			cl->file = NULL;
			cl->writing = 0;
			return (cl->keepalive);
		}
		return (1);
	}
	if (cl->ilen+10 >= cl->len) {
		if (cl->len > MAX_SOCKBUFFER) return (0);
		cl->buf = xresize (cl->buf, cl->len += SOCKBUFSZ);
	}
	while ((ret = read (cl->sockfd, cl->buf+cl->ilen,
		cl->len-cl->ilen)) == -1 && EINTR == errno);
	if (ret <= 0) return (0);
#ifdef WANT_CLIENT_REWRITE
	if (!cl->ilen && sizeof (cl->saddr) <= (size_t) ret &&
		cl->saddr.sin_addr.s_addr == htonl (0x7f000001))
		if (((struct sockaddr_in *) cl->buf)->sin_family == AF_INET) {
			memcpy (&cl->saddr, cl->buf, sizeof (cl->saddr));
			ret -= sizeof (cl->saddr);
			memmove (cl->buf, cl->buf+sizeof (cl->saddr), ret);
		}
#endif
	cl->ilen += ret;
	for (i=0;(ssize_t)i<(ssize_t)cl->ilen-1;i++)
	  if (cl->buf[i]=='\n' && (cl->buf[i+1]=='\n' ||
		(cl->buf[i+1]=='\r'&& i<cl->ilen-2&& cl->buf[i+2]=='\n'))) {
		   parse_http_request (cl);
		   return (cl->keepalive | cl->writing | cl->xop);
	  }
	return (1);
}

void
do_kill_conns ()
{
	client_t *cl = clients;
	killconns = 0;
	while (cl) {
		client_t *c = cl->next;
		remove_client (cl);
		cl = c;
	}
}

void
kill_conns ()
{
	killconns = 1;
}
