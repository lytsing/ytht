#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <fcntl.h>

#include <netdb.h>

#include "config.h"
#include "conn.h"
#include "sv_core.h"

int
bindport(int port)
{
	int s, val;
	struct linger ld;
	struct sockaddr_in sin;

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
		return -1;

	val = 1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof (val));
	ld.l_onoff = ld.l_linger = 0;
	setsockopt(s, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof (ld));

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	if (bind(s, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
		close(s);
		return -1;
	}

	if (listen(s, LISTEN_BACKLOG) < 0) {
		close(s);
		return -1;
	}
	return s;
}

void
dopipesig(int s)
{
	signal(SIGPIPE, dopipesig);
}

void
dosigbus(int s)
{
	signal(SIGBUS, dosigbus);
}

static int
getcl(int sv)
{
	int cl;
	socklen_t sl;
	struct sockaddr_in sin;
	sl = sizeof (sin);
	while ((cl = accept(sv, (struct sockaddr *) &sin, &sl)) == -1) {
		if (errno != EINTR)
			return -1;
	}
	return cl;
}

/* sv_core_httpd - wait on file descriptors */

void
sv_core_httpd(void)
{
	int sv;

	signal(SIGPIPE, dopipesig);
	signal(SIGBUS, dosigbus);
	
	if ((sv = bindport(SERVER_PORT)) == -1) 
		return;
	while (1) {
		int maxfd;
		fd_set rfs, wfs;
		FD_ZERO(&rfs);
		FD_ZERO(&wfs);
		FD_SET(sv, &rfs);

		maxfd = conn_cullselect(&rfs, &wfs);
		if (sv > maxfd)
			maxfd = sv;

		if (select(maxfd + 1, &rfs, &wfs, NULL, NULL) == -1) {
			if (errno == EINTR)
				continue;
			break;
		}
		if (FD_ISSET(sv, &rfs)) {
			int cl;

			if ((cl = getcl(sv)) == -1)
				break;
			if (fcntl(cl, F_SETFL, O_NONBLOCK) >= 0)
				conn_insert(cl);
			else
				close(cl);
		}
		conn_upkeep(&rfs, &wfs);
	}
	return;
}
