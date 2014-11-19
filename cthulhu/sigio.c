#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "client.h"
#include "socket.h"

/*XXX: check errors */

static int mysockfd;

static void
sigio (sig, sinfo, p)
	int sig;
	siginfo_t *sinfo;
	void *p;
{
	time (&curtime);
	if (sinfo->si_fd == mysockfd) {
		client_t *new = add_client (mysockfd);
		if (new) {
			fcntl (new->sockfd, F_SETFL, O_NONBLOCK | O_ASYNC | O_RDWR);
			fcntl (new->sockfd, F_SETSIG, SIGPOLL);
			fcntl (new->sockfd, F_SETOWN, mypid);
		}
	} else {
		client_t *c = clients;
		while (c && c->sockfd != sinfo->si_fd)
			c = c->next;
		if (c) if (!handle_socket (c)) remove_client (c);
	}
}

int
select_loop (sockfd)
	int sockfd;
{
	int flags = fcntl (sockfd, F_GETFL);
	struct sigaction sa;

	mysockfd = sockfd;

	memset (&sa, 0, sizeof (sa));
	sa.sa_sigaction = sigio;
	sa.sa_flags    |= SA_SIGINFO;

	if (flags == -1) exit (0x35);
	fcntl (sockfd, F_SETFL, O_NONBLOCK | O_ASYNC | flags);
	fcntl (sockfd, F_SETSIG, SIGPOLL);
	fcntl (sockfd, F_SETOWN, mypid);
	if (-1 == sigaction (SIGPOLL, &sa, NULL)) exit (0x36);
	while (1) {
		if (killconns) do_kill_conns ();
		pause ();
	}
}
