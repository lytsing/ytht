#ifndef DOMAIN_H
#define DOMAIN_H

#include "file.h"

struct __domain;

#if 0
typedef struct __xlink {
	char *name;
	long  hash;
	xfile_t *link;
	struct __xlink *next;
} xlink_t;

typedef struct __hlink {
	char *name;
	char *url;
	long  hash;
	struct __hlink *next;
} hlink_t;
#endif
/*#include "htaccess.h"
#include "fcgi.h"*/
#include "pathconf.h"

typedef struct __domain {
	char    *name;
	long    hash;
	size_t  dlen;
	size_t  filenum;
	xfile_t *files;
/*	xlink_t *xlinks;
	hlink_t *hlinks;
	time_t   mlinkf;*/
	time_t   mfilef;
	time_t   mpconf;
	int      logfd;
	pathconf_t *pathconfs;
	size_t   pcount;
/*#ifdef WANT_HTACCESS
	size_t   htcount;
	htaccess_t *hta;
#endif
#ifdef WANT_FASTCGI
	size_t   fcgicount;
	fcgi_socket_t *fsockets;
#endif*/
	struct __domain *next;
} domain_t;

typedef struct __domain_link {
	char    *name;
	long    hash;
	domain_t *link;
	struct __domain_link *next;
} domain_link_t;

extern domain_t *find_domain __P ((char *, size_t));
extern xfile_t *find_file __P ((char *, domain_t *, char *));
/*extern hlink_t *find_hlink __P ((char *, domain_t *));*/
extern void set_header __P ((xfile_t *));
extern int reinit_domain __P ((domain_t *));
extern int init_domains __P ((void));

extern domain_t default_domain;

#endif /* DOMAIN_H */
