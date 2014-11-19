/* get mime type */
#include "file.h"
#include "misc.h"

static file_t mimefile;

char *
get_mime_by_extension (ext)
	char *ext;
{
	char *s;
	int i, len;

	if (!mimefile.len) return (NULL);
	s = mimefile.start;
	len = mimefile.len;

	for (i=0;i<len;) {
		int x = 0;
		if (s[i] == '#') { while (i<len&&s[i++]!='\n'); continue; }
		if (s[i] <= ' ') { while (i<len&&s[i]<=' ')i++; continue; }
		for (;i<len&&(s[i]|0x20)==(ext[x]|0x20);x++,i++);
		if (s[i]<=' ') {
			while (i<len&&s[i]<=' ') i++;
			return (s+i);
		}
		while (i<len&&s[i++]!='\n');
	}
	return (NULL);
}

int
init_mime ()
{
	return (open_file ("mimetypes", &mimefile, NULL));
}

int
reinit_mime ()
{
	close_file (&mimefile);
	mimefile.len = 0;
	return (init_mime ());
}
