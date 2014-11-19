#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits.h>
#include "string.h"
#include "file.h"
#include "domain.h"
#include "pathconf.h"
#include "config.h"

enum { KEEPALIVE = 1, WRITING = 2, READING = 4, POSTING = 8, WAITING = 16, LOCALPORT = 32 };

#undef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 0xff

typedef struct __client {
	int      sockfd;
	struct   sockaddr_in saddr;
	/***********************/
	unsigned int      rt:3;
	unsigned int      keepalive:1;
	unsigned int      gzip:1;
	unsigned int      writing:1;
	unsigned int      xop;
	unsigned int      httpver;
	unsigned int      clength;
	size_t   status;
	size_t   toread;
	char    *iobuffer;
	int      xfd;
	int      (*hook) __P ((struct __client *));
	pathconf_t *pc;
	size_t   file_start;
	size_t   file_end;
	time_t   lastmod;
	domain_t *domain;
	/***********************/
	int      flags;
	int      timeout;
	pid_t    waiting;
/*	int      toread;*/ /* when posting */
/*	string_t string;*/
	size_t   urlp;
	size_t   urllen;
	unsigned char    *buf;
	size_t   len;
	size_t   ilen;
	size_t   rem;
	xfile_t	*file;
	time_t   lastop;
	struct   __client *next;
	struct   __client *prev;
	unsigned char     url [_POSIX_PATH_MAX];
} client_t;

extern int add_proc __P ((client_t *, char *, char *, size_t));
extern void kill_conns __P ((void));

#endif /* CLIENT_H */
