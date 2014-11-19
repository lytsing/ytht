/* posix */
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
/* openssl */
#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
/* -- */

#define die(x) exit (write (2, x, sizeof (x)-1))
#define die2(x) { perror (x); exit (23); }
#define log_err(x) perror (x)
#define log(x) write (2, x, sizeof (x))

#define TIMEOUT 600

struct sockaddr_in client;
struct sockaddr_in server;
int cfd, clientcnt;
static char *filename = "http-ssl.pem";

SSL_CTX *
create_ssl_Context ()
{
	SSL_METHOD *sm;
	SSL_CTX *res;
	SSLeay_add_ssl_algorithms ();
	sm = SSLv23_server_method ();
	if (!(res = SSL_CTX_new (sm)))
		die ("Could not create an SSL object\n");
	if (SSL_CTX_use_certificate_file (res, filename, SSL_FILETYPE_PEM) < 0)
		die ("Error opening certificate file\n");
	if (SSL_CTX_use_PrivateKey_file (res, filename, SSL_FILETYPE_PEM) < 0)
		die ("Error opening private key file\n");
	if (!SSL_CTX_check_private_key (res))
		die ("Could not check private key\n");
	return (res);
}

void
handle_socket_error ()
{
	switch (errno) {
	  case EINTR: break;
	  case EBADF: case ENOTSOCK:
		die ("Invalid file descriptor\n");
	  default: /* hopefully a temporary error */
		log_err ("accept failed");
		sleep (60);
		break;
	}
}

void
handle_sockets (ctx, csock, ssock)
	SSL_CTX *ctx;
	int csock, ssock;
{
	struct pollfd pfd[2];
	char buffer[0x1400];
	SSL *ssl = SSL_new (ctx);

	SSL_set_fd (ssl, csock);

	if (-1 == SSL_accept (ssl))
		die ("SSL_accept failed\n");

	pfd[0].events = pfd[1].events = POLLIN;
	pfd[0].fd = csock;
	pfd[1].fd = ssock;

	while (1) {
	  int ret = poll (pfd, 2, TIMEOUT*1000);
	  if (!ret) _exit (0);
	  if (-1 == ret) {
		if (EINTR == errno) continue;
		die2 ("poll");
	  }
	  if (pfd[0].revents & POLLIN) {
		ssize_t ret = SSL_read (ssl, buffer, sizeof (buffer)), len = 0;
		if (!ret) _exit (0);
		if (-1 == ret) {
			if (EINTR == errno) continue;
			die2 ("read");
		}
		while (1) {
		  ssize_t xret = write (ssock, buffer+len, ret);
		  if (-1 == xret) {
			if (EINTR == errno) continue;
			die2 ("write");
		  }
		  if ((ret -= xret) <= 0) break;
		  len += xret;
		}
	  } else if (pfd[1].revents & POLLIN) {
		ssize_t ret = read (ssock, buffer, sizeof (buffer)), len = 0;
		if (!ret) _exit (0);
		if (-1 == ret) {
			if (EINTR == errno) continue;
			die2 ("read");
		}
		while (1) {
		  ssize_t xret = SSL_write (ssl, buffer+len, ret);
		  if (-1 == xret) {
			if (EINTR == errno) continue;
			die2 ("write");
		  }
		  if ((ret -= xret) <= 0) break;
		  len += xret;
		}
	  }
	}
}

void
sigchld (sig)
{
	int status;
	while (waitpid (-1, &status, WNOHANG) > 0) {
	  if (WIFSIGNALED (status))
		log ("child died on signal\n");
	  else if (WEXITSTATUS (status))
		log ("child terminated abnormally\n");
	  clientcnt--;
	}
}

/* options: -f -- certificate, -p local port -h local ip, -m max clients */
int
main (argc, argv)
	int argc;
	char **argv;
{
	int sockfd = 0, i, maxclients = -1; /* ipinit spec */
	struct sigaction sa;
	SSL_CTX *ctx;

	sa.sa_handler = sigchld;
	if (-1 == sigaction (SIGCHLD, &sa, NULL))
		die2 ("sigaction");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr ("127.0.0.1");
	server.sin_port = htons (80);

	for (i=1;i<argc;i++) {
	  if ('-'==argv[i][0]) {
		int x = i;
		char *arg = argv[i][2]?argv[i]+2:argv[++i];
		if (!arg) die ("Missing argument\n");
		switch (argv[x][1]) {
		 case 'p':
			if (!(server.sin_port = htons (atoi (arg))))
			  die ("invalid: port\n");
			break;
		 case 'f':
			filename = arg;
			break;
		 case 'h':
			if (-1 == (server.sin_addr.s_addr = inet_addr (arg)))
			  die ("invalid: server\n");
			break;
		 case 'm':
			if (!(maxclients = atoi (arg)))
			  die ("invalid: maxclients\n");
			break;
		}
	  }
	}

	if (!(ctx = create_ssl_Context ()))
		die ("Could not initialize OpenSSL\n");

	while (1) {
	  socklen_t clen = sizeof (client);
	  pid_t pid;
	  if (-1 == (cfd = accept (sockfd, &client, &clen))) {
		handle_socket_error ();
		continue;
	  }
	  if (clientcnt >= maxclients) { close (cfd); continue; }
	  if (!(pid = fork ())) {
		int sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (-1 == sockfd) {
			handle_socket_error ();
			_exit (23);
		}
		if (-1 == connect (sockfd, (struct sockaddr *) &server,
			sizeof (server)))
			die2 ("connect");
		write (sockfd, &client, sizeof (client));
		handle_sockets (ctx, cfd, sockfd);
	  } else
		if (-1 == pid) die2 ("fork");
	  clientcnt++;
	  close (cfd);
	}
}
