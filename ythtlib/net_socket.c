#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "ythtlib.h"

//Bind to address:port
//If address==NULL or address=="", then bind to INADDR_ANY:port
int
bindSocket(char *address, int port, int queueLength)
{
	struct sockaddr_in addr;
	struct linger ld;
	time_t val;
	int sock;

	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		return -1;
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	if (!address || !*address) {
		addr.sin_addr.s_addr = INADDR_ANY;
	} else {
		if (inet_aton(address, &addr.sin_addr) == 0)
			return -1;
	}

	val = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof (val));
	ld.l_onoff = ld.l_linger = 0;
	setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof (ld));

	if (bind(sock, (struct sockaddr *) &addr, sizeof (addr)) < 0) {
		close(sock);
		return -1;
	}
	if (listen(sock, queueLength)) {
		close(sock);
		return -1;
	}
	return sock;
}

int
connectSocket(char *address, int port)
{
	struct sockaddr_in addr;
	int sock;

	sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		return -1;
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	if (inet_aton(address, &addr.sin_addr) == 0)
		return -1;

	if ((connect(sock, (struct sockaddr *) &addr, sizeof (addr)))) {
		close(sock);
		return -1;
	}
	return sock;
}
