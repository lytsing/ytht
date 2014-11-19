#ifndef PATHCONF_H
#define PATHCONF_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PCONF_PREFIX "pathconf-"

enum {
	PC_CGI = 1, PC_FCGI, PC_AUTH, PC_FILTER, PC_REWRITE, PC_REDIR,
	PC_FCGI_AUTH, PC_FCGI_FILTER
};

typedef struct {
	int       type;
	char     *path;
	char     *p1, *p2;
	size_t    pathlen, plen1;
	struct    sockaddr *saddr;
	unsigned  int socksize;
} pathconf_t;

extern pathconf_t *pc_generic_wildmat __P ((int, char *, struct __domain *));
extern int read_pathconf __P ((struct __domain *));

#endif /* PATHCONF_H */
