#include "config.h"
#include "domain.h"
#include "misc.h"
#include "log.h"
#include "client.h"
#ifdef DEBUG
# include "sdb.h"
#endif

#define prints(x) write (2, x, sizeof (x) - 1)
#define die(x) exit (prints (x))
#define warn(x) prints(x)

#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

pid_t mypid;

static void
reinit_domains ()
{
	extern domain_t *domains;
	domain_t *d = domains;
	while (d) {
	  reinit_domain (d);
	  d = d->next;
	}
	init_domains ();
}

static void
ct_sigaction (sig)
	int sig;
{
	switch (sig) {
	  case SIGHUP:  reinit_domains (); reinit_mime (); break;
	  case SIGTERM: kill_conns (); exit (0);
	  case SIGINT:  kill_conns (); break;
	  case SIGQUIT: exit (0);
	}
}

static int
init_signals ()
{
	struct sigaction sa;

	memset (&sa, 0, sizeof (sa));
	sa.sa_sigaction = ct_sigaction;
	if (sigaction (SIGINT, &sa, NULL) == -1) return (0);
	if (sigaction (SIGHUP, &sa, NULL) == -1) return (0);
	if (sigaction (SIGQUIT, &sa, NULL) == -1) return (0);
	if (sigaction (SIGTERM, &sa, NULL) == -1) return (0);
	if (sigaction (SIGUSR1, &sa, NULL) == -1) return (0);
	if (sigaction (SIGUSR2, &sa, NULL) == -1) return (0);
	sa.sa_sigaction = (void *) SIG_IGN;
	if (sigaction (SIGPIPE, &sa, NULL) == -1) return (0);
	return (1);
}

unsigned int addr = 0, port = 80;

static int
bind_sockets ()
{
	int sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in saddr;

	saddr.sin_family       =   AF_INET;
	saddr.sin_port         =   htons (port);
	saddr.sin_addr.s_addr  =   htonl (addr);

	if (bind (sockfd, (struct sockaddr *) &saddr, sizeof (saddr)) < 0)
		return (0);
	if (listen (sockfd, 25) < 0) return (0);
	return (sockfd);
}

static char *
getarg (argv, idx)
	char **argv;
	int *idx;
{
	if (argv[*idx][2]) return (argv[*idx]+2);
	else if (argv[++(*idx)]) return (argv[*idx]);
	else return ("");
}

int
main (argc, argv)
	int argc;
	char **argv;
{
	int i, sockfd = 0, ipinit = 0;

	mypid = getpid ();

	for (i=1;i<argc;i++) {
		if (argv[i][0] != '-') continue;
		switch (argv[i][1]) {
		  case 'l':
			if ((loglevel = atoi (getarg (argv, &i))) > 5)
				die ("Invalid loglevel\n");
			break;
		  case 'p':
			if (!(port = atoi (getarg (argv, &i))) ||
				port > 0xffff)
				die ("Invalid port\n");
			break;
		  case 'i':
			ipinit = 1;
			sockfd = 3;
			break;
		  default:
			die ("Invalid option specified\n");
		}
	}
	if (!ipinit && chdir (BASEDIR) == -1)
		die ("Could not chdir to basedirectory ("BASEDIR")\n");
		
	if (!init_mime ())
		die ("Could not initialize mimetypes\n");
	if (!sockfd && !(sockfd = bind_sockets ()))
		die ("Could not bind socket\n");
	if (!init_domains ())
		die ("No available domains found. I will exit now\n");
	if (!init_signals ())
		die ("Internal error: Signal\n");

	while (1) select_loop (sockfd);
	return (0);
}
