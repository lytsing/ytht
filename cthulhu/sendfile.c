#ifdef __linux__
# include <sys/sendfile.h>
#endif
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "file.h"
#include "client.h"
#include "config.h"
#include "sendfile.h"

#ifdef WANT_SENDFILE
ssize_t
arch_sendfile (cl)
	client_t *cl;
{
	off_t off = cl->file->len - cl->rem;
#ifdef __linux__
	return (sendfile (cl->sockfd, cl->file->fd, &off, cl->rem));
#endif
#ifdef __hpux
	return (sendfile (cl->sockfd, cl->file->fd, off, cl->rem, NULL, 0));
#endif
#ifdef __FreeBSD__
	ssize_t res = 0;
	if (sendfile (cl->file->fd, cl->sockfd, off, cl->rem, NULL, &res, 0) &&
		errno != EAGAIN) res = -1;
	return (res);
#endif
}
#endif

ssize_t
xwrite (cl)
	client_t *cl;
{
#ifdef WANT_SENDFILE
	return (arch_sendfile (cl));
#else
	return (write (cl->sockfd, cl->file->start+cl->file->len-cl->rem,
		cl->rem));
#endif
}

#ifdef WANT_SENDFILE
ssize_t
xsendfile (cl, start, size, hdr, hdrcount)
	client_t *cl;
	off_t   start;
	size_t  size;
	struct  iovec *hdr;
	size_t  hdrcount;
{
	ssize_t ret, ret2;
#ifdef __linux__
	int tmp = 1;
	setsockopt (cl->sockfd, SOL_TCP, TCP_CORK, &tmp, sizeof (tmp));
	if (1 == hdrcount)
		ret = write (cl->sockfd, hdr[0].iov_base, hdr[0].iov_len);
	else ret = writev (cl->sockfd, hdr, hdrcount);
	if (-1 == ret) return (-1);
	if (-1 == (ret2 = sendfile (cl->sockfd, cl->file->fd, &start, size)))
		return (-1);
	return (ret+ret2);
#endif
#ifdef __hpux
	hdr[2].iov_base = NULL;
	if (hdrcount == 2) {
	  if (-1 == (ret = write (cl->sockfd, hdr[0].iov_base, hdr[0].iov_len)))
		return (-1);
	} else ret = 0;
	ret2 = sendfile (cl->sockfd, cl->file->fd, start, size, hdr+1, 0);
	if (-1 == ret2) return (-1);
	return (ret+ret2);
#endif
#ifdef __FreeBSD__
	struct sf_hdtr shdr;
	shdr.headers = hdr;
	shdr.hdr_cnt = hdrcount;
	shdr.trailers = NULL;
	shdr.trl_cnt = 0;
	if (-1 == sendfile (cl->file->fd, cl->sockfd, start, size, &shdr,
		&ret, 0)) return (-1);
	return (ret);
#endif
}
#endif
