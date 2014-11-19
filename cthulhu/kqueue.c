#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include "config.h"
#include "client.h"
#include "log.h"
#include "socket.h"

#define KEV_FETCH 1

int
select_loop (sockfd)
	int sockfd;
{
	static int kqd;
	struct kevent ke[KEV_FETCH];
	client_t *cl = clients;
	size_t i;

	if (killconns) do_kill_conns ();

	if (kqd <= 0) {
		struct kevent k;
		if ((kqd = kqueue ()) <= 0) return (-1);
		EV_SET (&k, sockfd, EVFILT_READ, EV_ADD, 0, 0, 0);
		if (-1 == kevent (kqd, &k, 1, NULL, 0, NULL))
		  return (-1);
	}

	if (-1 == kevent (kqd, NULL, 0, ke, KEV_FETCH, NULL))
	  return (-1);

	for (i=1;i<filenum;i++) {
		client_t *c = cl->next;
		if (ke[0].ident == cl->sockfd)
		  if (!handle_socket (cl)) {
			struct kevent k;
			EV_SET (&k, cl->sockfd, EVFILT_READ, EV_DELETE, 0, 0, 0);
			kevent (kqd, &k, 1, NULL, 0, NULL);
			remove_client (cl);
		  }
		cl = c;
	}
	if (ke[0].ident == sockfd) {
		struct kevent k;
		client_t *c = add_client (sockfd);
		EV_SET (&k, c->sockfd, EVFILT_READ, EV_ADD, 0, 0, 0);
		kevent (kqd, &k, 1, NULL, 0, NULL);
	}
	return (0);
}
