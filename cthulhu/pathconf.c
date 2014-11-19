/*
   Copyright (C) 2002, 2003 Thomas M. Ogrisegg <tom-cthulhu@fnord.at>

 Directives:
	cgi
	filter
	auth
	fcgi
	fcgi-auth
	fcgi-filter
	rewrite
	redir

	/foobar/ *	cgi	$
	*.php3		cgi	/usr/local/bin/php
	/rotfl/
*/

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include "config.h"
#include "domain.h"
#include "xalloc.h"
#include "client.h"
#include "file.h"
#include "misc.h"
#include "log.h"
#include "cgi.h"

#define ENTRY(x) { x, sizeof (x) - 1 }

static entry_t pc_entries[] = {
	ENTRY ("cgi"),
	ENTRY ("fcgi"),
	ENTRY ("auth"),
	ENTRY ("filter"),
	ENTRY ("rewrite"),
	ENTRY ("redir"),
	ENTRY ("fcgi-auth"),
	ENTRY ("fcgi-filter")
};

static int
get_entry_type (str)
	char *str;
{
	size_t i;
	for (i=0;i<sizeof (pc_entries) / sizeof (pc_entries[0]);i++)
	  if (pc_entries[i].str[0]==*str &&
		!memcmp (pc_entries[i].str, str, pc_entries[i].len) &&
		str[pc_entries[i].len]<=' ')
		return (i+1);
	return (0);
}

int
connect_to (cl, pc)
	client_t *cl;
	pathconf_t *pc;
{
	int sockfd = socket (((struct sockaddr *) pc->saddr)->sa_family,
		SOCK_STREAM, 0);
	if (-1 == sockfd) return (0);
	if (-1 == fcntl (sockfd, F_SETFL, O_NONBLOCK)) goto err;
	if (-1 == connect (sockfd, (struct sockaddr *) pc->saddr,
		pc->socksize)) if (EINPROGRESS != errno) goto err;
	cl->xfd = sockfd;
	return (1);
err:
	close (sockfd);
	return (0);
}

pathconf_t *
pc_generic_wildmat (type, str, dom)
	int type;
	char *str;
	domain_t *dom;
{
	pathconf_t *pc = dom->pathconfs;
	size_t i;
	if (!pc) return (NULL);
	for (i=0;i<dom->pcount;i++,pc++)
	  if (pc->type == type)
		if (wildmat (str, pc->path)) return (pc);
	return (NULL);
}

static char *
pcstrdup (str, i, len, plen)
	char *str;
	size_t *i, len, *plen;
{
	size_t x = *i, tmp = x;
	char *res;
	if (str[x]=='"') { tmp = ++x; while (x<len&&str[x]!='"') x++; }
	else while (x<len&&str[x]>' ') x++;
	res = xalloc (x-tmp+1);
	*plen = x-tmp;
	memcpy (res, str+tmp, x-tmp);
	res[x-tmp] = 0;
	*i = x+1;
	return (res);
}

#define is_num(x) ((x)<='9'&&(x)>='0')

static unsigned int
scan_inet_addr (s)
	char **s;
{
	unsigned int res = 0;
	int i;
	char *str = *s;
	for (i=0;i<4;i++) {
		int x, tmp = 0;
		for (x=0;x<3&&is_num(*str);x++,str++)
			tmp = (*str-'0')+tmp*10;
		if (i!=3&&'.'!=*str++) return (-1);
		res = res<<8|tmp;
	}
	*s = str;
	return (htonl (res));
}

static int
get_sock_type (pc, s, i, len)
	pathconf_t *pc;
	char *s;
	size_t *i, len;
{
	size_t x = *i;
	if (!is_num (s[x])) { /* Assume Unix socket */
		struct sockaddr_un *xsun = xmalloc (struct sockaddr_un);
		size_t maxlen = sizeof (xsun->sun_path), tmp = 0;	
		pc->saddr = (struct sockaddr *) xsun;
		xsun->sun_family = AF_UNIX;
		while (tmp<maxlen && x<len)
			if ((xsun->sun_path[tmp++] = s[x++])<=' ') break;
		if (tmp <= 0) goto error;
		xsun->sun_path[tmp-1] = 0;
		pc->socksize = tmp + sizeof (xsun->sun_family);
		return (1);
	} else { /* Assume Internet socket */
		struct sockaddr_in *sin = xmalloc (struct sockaddr_in);
		char *str = s+x;
		pc->saddr = (struct sockaddr *) sin;
		sin->sin_family = AF_INET;
		if (-1 == (int)(sin->sin_addr.s_addr = scan_inet_addr (&str)))
			goto error;
		x = str-s+1;
		if (!(sin->sin_port = htons (atoi (s+x))))
			goto error;
		pc->socksize = sizeof (*sin);
		return (1);
	}
error:
	xfree (pc->saddr, pc->socksize); pc->socksize = 0;
	return (0);
}

int
read_pathconf (dom)
	domain_t *dom;
{
	size_t slen = strlen (dom->name), len;
	char *filename = alloca (slen + sizeof (PCONF_PREFIX)+1), *s;
	int ret = 0;
	size_t lines, i, pcp = 0;
	file_t file;

	memcpy (filename, PCONF_PREFIX, sizeof (PCONF_PREFIX) - 1);
	memcpy (filename+sizeof (PCONF_PREFIX) - 1, dom->name, slen+1);

	if (!open_file (filename, &file, &dom->mpconf))
		return (0);
	s = file.start;
	len = file.len;

	for (i=0,lines=0;i<len;lines++) {
		while (i<len&&s[i]>' ') i++;
		while (i<len&&s[i]!='\n'&&s[i]<=' ') i++;
		if (s[i]=='\n' || !get_entry_type (s+i)) goto parse_error;
		while (i<len&&s[i++]!='\n');
	}
	dom->pcount = lines;
	dom->pathconfs = (pathconf_t *) xalloc (sizeof (pathconf_t) * lines);
	memset (dom->pathconfs, 0, sizeof (pathconf_t) * lines);
	for (i=0;i<len&&pcp<lines;pcp++) {
		int type;
		dom->pathconfs[pcp].path = pcstrdup (s, &i, len,
			&dom->pathconfs[pcp].pathlen);
		while (i<len&&s[i]<=' ') if ('\n' == s[i++]) goto parse_error;
		type = dom->pathconfs[pcp].type = get_entry_type (s+i);
		while (i<len&&s[i]>' ') i++;
		while (i<len&&s[i]<=' ') if ('\n' == s[i++]) goto parse_error;
		if (s[i] == '$') i++;
		else dom->pathconfs[pcp].p1 = pcstrdup (s, &i, len,
			&dom->pathconfs[pcp].plen1);
		if (type != PC_REDIR && type != PC_REWRITE) {
			while (i<len&&s[i]<=' ')
				if ('\n' == s[i++]) goto parse_error;
			if (!get_sock_type (dom->pathconfs+pcp, s, &i, len))
				goto parse_error;
			while (i<len&&s[i++]!='\n');
		}
	}
	ret = 1;
parse_error:
	close_file (&file);
	if (!ret) {
		log_err ("pathconf: Parse error in ", filename);
		if (dom->pcount) {
		   dom->pcount = 0;
		   xfree (dom->pathconfs, 0);
		}
	}
	return (ret);
}
