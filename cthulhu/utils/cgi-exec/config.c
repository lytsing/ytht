#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "config.h"

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

typedef struct __domain_entry {
	char *cginame;
	char *cwd;
	char *root;
} domain_entry_t;

typedef struct __domain {
	char *name;
	long hash;
	size_t dentrycnt;
	time_t modtime;
	domain_entry_t *dentries;
	struct __domain *next;
} domain_t;

#define cxmalloc(x) (x *) cxalloc (sizeof (x))

extern void *cxalloc __P ((size_t));
static domain_t *domains;

static char *
cxstrdup (str)
	char *str;
{
	size_t len = 0;
	char *res;
	while (str[len]) len++;
	res = cxalloc (len+1);
	memcpy (res, str, len);
	res[len] = 0;
	return (res);
}

static domain_t *
init_domain_config (filename, name)
	char *filename, *name;
{
	size_t i = 0, linecount = 0;
	char *s;
	int fd;
	ssize_t flen;
	domain_t *new;
	domain_entry_t *cur;

	if (-1 == (fd = open (filename, O_RDONLY))) return (NULL);
	if (-1 == (flen = lseek (fd, 0, SEEK_END)) || !flen ||
	 (MAP_FAILED == (s = mmap (NULL, flen, PROT_READ, MAP_SHARED, fd, 0))))
		{ close (fd); return (NULL); }

	new = cxmalloc (domain_t);

	new->hash = hasch (name);
	new->name = cxstrdup (name);

	if (!domains) domains = new;
	else {
		domain_t *tmp = domains;
		while (tmp->next) tmp = tmp->next;
		tmp->next = new;
	}

	while (i<flen)
		if (s[i++]=='\n') linecount++;

	cur = new->dentries = (domain_entry_t *) cxalloc (linecount *
		sizeof (domain_entry_t));

	memset (cur, 0, sizeof (domain_entry_t) * linecount);

	new->dentrycnt = linecount;

	for (i=0;i<flen;cur++) {
		size_t len = i;
		while (len<flen&&s[len]!=':') len++;
		cur->cginame = cxalloc (++len-i);
		memcpy (cur->cginame, s+i, len-i-1);
		cur->cginame[len-i-1] = 0;
		i = len;
		while (len<flen&&s[len]!=':') len++;
		if (i != len) {
			cur->cwd = cxalloc (len-i+1);
			memcpy (cur->cwd, s+i, len-i);
			cur->cwd[len-i] = 0;
		}
		len++;
		i = len;
		while (len<flen&&s[len]!='\n') len++;
		if (i != len) {
			cur->root = cxalloc (len-i+1);
			memcpy (cur->root, s+i, len-i);
			cur->root[len-i] = 0;
		}
		i = ++len;
	}
	close (fd);
	munmap (s, flen);
	return (new);
}

static domain_t *
find_hashed_domain (name)
	char *name;
{
	long hash = hasch (name);
	domain_t *dom = domains;
	while (dom) {
		if (dom->hash == hash && !strcmp (dom->name, name))
			return (dom);
		dom = dom->next;
	}
	return (NULL);
}

void
read_domain_config (name, cd, root)
	char *name, **cd, **root;
{
	size_t i, len = 0;
	domain_entry_t *dent;
	domain_t *dom;
	char *domain;
	while (name[len]>' '&&name[len]!='/') len++;
	domain = alloca (len+sizeof(CGI_CONFIG_DIR));
	memcpy (domain, CGI_CONFIG_DIR, sizeof (CGI_CONFIG_DIR)-1);
	memcpy (domain+sizeof(CGI_CONFIG_DIR)-1, name, len);
	domain[len+sizeof(CGI_CONFIG_DIR)-1] = 0;
	if (!(dom = find_hashed_domain (domain+sizeof(CGI_CONFIG_DIR)-1)))
		if (!(dom = init_domain_config (domain, domain+sizeof(CGI_CONFIG_DIR)-1)))
			return;

	for (dent=dom->dentries,i=0;i<dom->dentrycnt;i++,dent++)
	  if (wildmat (name+len, dent->cginame)) {
		*cd = dent->cwd;
		*root = dent->root;
		return;
	  }
}
