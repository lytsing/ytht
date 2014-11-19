#include "bbslib.h"

int
bbsattach_main()
{
	char *ptr, *path_info;
	struct cgi_applet *a;
	path_info = getsenv("SCRIPT_URL");
	path_info = strchr(path_info + 1, '/');
	if (NULL == path_info)
		http_fatal("������ļ���");
	if (!strncmp(path_info, "/attach/", 8))
		path_info += 8;
	else
		http_fatal("������ļ���");
	ptr = strchr(path_info, '/');
	if (NULL == ptr)
		http_fatal("������ļ���");
	*ptr = 0;
	a = get_cgi_applet(path_info);
	if (NULL == a)
		http_fatal("������ļ���");
	return (*(a->main)) ();
}
