#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "config.h"
#include "domain.h"
#include "file.h"
#include "misc.h"
#include "log.h"
#include "xalloc.h"
#include "string.h"
#include "pathconf.h"

domain_t *domains, *ldom;
domain_link_t *dlinks, *ldlnk;

domain_t default_domain;

int
casecmp (s1, s2, len)
	const char *s1, *s2;
	size_t len;
{
	while (len--) {
		if (*s1 == ':') break;
		if ((*s1++|0x20) != (*s2++|0x20)) return (0);
	}
	return (1);
}

domain_t *
find_domain (name, len)
	char *name;
	size_t len;
{
	domain_t *dom;
	domain_link_t *dl;

	if (!name || !*name) return (&default_domain);

	for (dom=domains;dom;dom=dom->next)
		if (casecmp (name, dom->name, len))
			return (dom);
	for (dl=dlinks;dl;dl=dl->next)
		if (casecmp (name, dl->name, len))
			return (dl->link);
	return (&default_domain);
}

static xfile_t *
open_dom_file (name)
	char *name;
{
	xfile_t *file = xmalloc (xfile_t);
	memset (file, 0, sizeof (*file));
	if (!xopen_file (name, file)) {
		xfree (file, sizeof (file_t));
		return (NULL);
	}
	set_header (file);
	return (file);
}

xfile_t *
find_file (name, domain, url)
	char *name;
	domain_t *domain;
	char *url;
{
	errno = 0;
	if (!domain->files) {
		if (!url) return (NULL);
		return (open_dom_file (url));
	}
	else {
		long hash = hasch (name);
		size_t i;
		for (i=0;i<domain->filenum;i++)
			if (hash == (domain->files+i)->hash &&
				!strcmp (name, (domain->files+i)->filename)) {
				(domain->files+i)->count++;
				if (xreopen_file (url, (domain->files+i)))
					set_header (domain->files+i);
				return (domain->files+i);
			}
		if (url) return (open_dom_file (url));
	}
	return (NULL);
}

#define xputs(x) as_puts (&file->header, x, sizeof (x) - 1)

void
set_header (file)
	xfile_t *file;
{
	char *ext = file->filename, *last = NULL;
	while (*ext) if ('.' == *ext++) last = ext;
	ext = (last?get_mime_by_extension (last):NULL);
	xputs ("Last-Modified: ");
	if (file->header.off+32 > file->header.len)
		expand_string (&file->header, 32);
	__date (&file->header, file->mtime);
	xputs ("Content-Type: ");
	if (ext) as_putline (&file->header, ext);
	else xputs ("text/plain\r\n");
	xputs ("Content-Length: ");
	as_putlong (&file->header, file->len);
	xputs ("\r\n\r\n");
	fix_string (&file->header);
}

static void
read_domain (dom, name)
	domain_t *dom;
	char *name;
{
	file_t file;
	size_t lines = 0, line = 0, len, i, ret;
	char *str;

	file.len = 1;
	if (!(ret = open_file (name, &file, &dom->mfilef)) && file.len) {
		log_err ("Could not open domain list file ", name);
		return;
	}
	if (ret) {
		str = file.start;
		len = file.len;
		for (i=0;i<len;i++)
			if (str[i]=='\n') { lines++; str[i]=0; }
		dom->filenum = lines;
		dom->files = (xfile_t *) xalloc (lines * sizeof (xfile_t));
		memset (dom->files, 0, lines * sizeof (xfile_t));
		chdir (dom->name);
		for (i=0;i<len&&line<lines;line++) {
			if (!xopen_file (str+i, dom->files+line)) {
				log_err ("Fatal: Could not open file ", str+i);
				exit (23);
			} else {
				set_header (dom->files+line);
				(dom->files+line)->count = 1;
			}
			while (i<len&&str[i++]);
		}
		chdir ("..");
		close_file (&file);
	}
	read_pathconf (dom);
}

static int
have_domain (name)
	char *name;
{
	long hash = hasch (name);
	domain_t *dom = domains;
	if (dom)
	  while ((dom = dom->next))
		if (dom->hash == hash && !strcmp (dom->name, name))
			return (1);
	return (0);
}

static void
add_domain (name)
	char *name;
{
	domain_t *domain;
	size_t s1;
	char *logfile;
	if (have_domain (name+9)) return;
	logfile = alloca ((s1=strlen (name+9))+5);
	if (!strcmp (name, "filelist-default")) domain = &default_domain;
	else domain = xmalloc (domain_t);
	memset (domain, 0, sizeof (*domain));
	domain->name = strdup (name+9);
	domain->hash = hasch (name+9);
	domain->dlen = s1;
	memcpy (logfile, name+9, s1);
	memcpy (logfile+s1, ".log", 5);
	if (-1 == (domain->logfd = open (logfile, O_RDWR | O_CREAT | O_APPEND,
		0600))) log_intern_err (logfile);
	read_domain (domain, name);
	if (!ldom) domains = ldom = domain;
	else {
		ldom->next = domain;
		ldom = domain;
	}
}

static void
add_domain_link (dname, name)
	char *dname, *name;
{
	domain_link_t *dl;
	domain_t *dom = find_domain (dname, strlen (dname));
	if (!dom || dom == &default_domain) return;
	dl = xmalloc (domain_link_t);
	dl->name = strdup (name);
	dl->hash = hasch (name);
	dl->link = dom;
	dl->next = NULL;
	if (!dlinks) dlinks = ldlnk = dl;
	else {
		ldlnk->next = dl;
		ldlnk = dl;
	}
}

static int
new_file (fn, t)
	char *fn;
	time_t t;
{
	struct stat st;
	if (stat (fn, &st)) return (0);
	return (st.st_mtime != t);
}

int
reinit_domain (dom)
	domain_t *dom;
{
	size_t s1 = strlen (dom->name);
	char *buf = alloca (s1+23);
	memcpy (buf, "filelist-", 9);
	memcpy (buf+9, dom->name, s1+1);
	if (new_file (buf, dom->mfilef)) {
		xfree (dom->files, dom->filenum * sizeof (xfile_t));
		read_domain (dom, buf);
	} else {
		size_t i;
		chdir (dom->name);
		for (i=0;i<dom->filenum;i++)
		  if (xreopen_file ((dom->files+i)->filename, (dom->files+i)))
			set_header (dom->files+i);
		chdir ("..");
	}
	memcpy (buf, "pathconf", 8);
	if (new_file (buf, dom->mpconf)) {
		xfree (dom->pathconfs, 0);
		dom->pathconfs = NULL;
		read_pathconf (dom);
	}
	return (1);
}

int
init_domains ()
{
	DIR *dir = opendir (".");
	struct dirent *dent;
	struct stat st;
	char buffer[0x200];

	if (!dir) return (0);

	default_domain.name = "default";
	default_domain.hash = 0;
	default_domain.dlen = sizeof ("default") - 1;
	default_domain.logfd = 2;

	while ((dent = readdir (dir))) {
		if ('.' == dent->d_name[0] && ('.' == dent->d_name[1] ||
			!dent->d_name[1]))
			continue;
		if (lstat (dent->d_name, &st)) {
			log_err ("Could not stat file ", dent->d_name);
			continue;
		}
		if (S_ISLNK (st.st_mode)) continue;
		if (!memcmp (dent->d_name, "filelist-", 9))
			add_domain (dent->d_name);
	}
	rewinddir (dir);
	while ((dent = readdir (dir))) {
	  if ('.' == dent->d_name[0]) continue;
	  if (lstat (dent->d_name, &st)) continue;
	  if (S_ISLNK (st.st_mode)) {
		ssize_t ret = readlink (dent->d_name, buffer, sizeof (buffer));
		if (ret == -1) continue;
		if (buffer[ret-1] == '/') buffer[ret-1] = 0;
		buffer[ret] = 0;
		add_domain_link (buffer, dent->d_name);
	  }
	}
	closedir (dir);
	return (1);
}
