#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <errno.h>
#include <fcntl.h>
#include "config.h"
#include "client.h"
#include "xalloc.h"
#include "log.h"
#include "http.h"
#include "misc.h"
#include "socket.h"

int
select_loop (sockfd)
	int sockfd;
{
	struct pollfd *pfd = alloca (filenum * sizeof (struct pollfd));
	client_t *cl = clients;
	int i, ret, timeout = filenum!=1?HTTP_TIMEOUT:-1;

	if (killconns) { do_kill_conns (); return (0); }

	pfd[0].fd = sockfd;
	pfd[0].events = POLLIN;

	for (i=1;i<filenum;i++) {
		pfd[i].events = cl->writing?POLLOUT:POLLIN;
		pfd[i].fd = (cl->xop&2)?cl->xfd:cl->sockfd;
		if (HTTP_TIMEOUT-(curtime-cl->lastop) < timeout)
			timeout = HTTP_TIMEOUT-(curtime-cl->lastop);
		cl = cl->next;
	}

	if (timeout != -1) timeout *= 1000;

	while ((ret = poll (pfd, filenum, timeout)) < 0 && EINTR == errno);

	if (ret < 0) {
		log_intern_err ("poll failed");
		return (0);
	}

	curtime = time (NULL);

	for (i=1,cl=clients;i<filenum&&cl;i++) {
		client_t *c = cl->next;
		if (!((cl->writing && (pfd[i].revents & POLLOUT)) ||
			(!cl->writing && (pfd[i].revents & POLLIN)) ||
			(pfd[i].revents & (POLLHUP|POLLNVAL|POLLERR)))) {
			if (cl->lastop + HTTP_TIMEOUT <= curtime)
				remove_client (cl);
			cl = c; continue;
		}
		cl->lastop = curtime;
		if ((!cl->hook && !handle_socket (cl)) ||
			(cl->hook && !cl->hook (cl)))
			remove_client (cl);
		cl = c;
	}

	if (pfd[0].revents & POLLIN)
		add_client (sockfd);
	else if (pfd[0].revents & (POLLNVAL|POLLERR)) {
		log_intern_err ("socket descriptor is invalid");
		exit (23);
	}

	return (0);
}
