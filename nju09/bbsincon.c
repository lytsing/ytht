#include "bbslib.h"
int quote_quote = 0;

int
bbsincon_main()
{
	char *path_info;
	char *name, filename[128], dir[128], board[40], *ptr;
	extern char *cginame;
	int old_quote_quote;
	path_info = getsenv("SCRIPT_URL");
	path_info = strchr(path_info + 1, '/');
	if (NULL == path_info)
		http_fatal("错误的文件名");
	if (!strncmp(path_info, "/boards/", 8))
		path_info += 8;
	else
		http_fatal("错误的文件名1 %s", path_info);
	name = strchr(path_info, '/');
	if (NULL == name)
		http_fatal("错误的文件名2");
	*(name++) = 0;
	strtok(name, "+");
	ptr = strtok(NULL, ""); //Take the rest
	if(ptr) {
		if(strchr(ptr, 'm')) {
			usingMath = 1;
			withinMath = 0;
		}
		if(strchr(ptr, 's')) {
			stripHeadTail = 1;
		}
	} else {
		usingMath = 0;
		stripHeadTail = 0;
	}
	strsncpy(board, path_info, sizeof (board));
	//translate board num to actual board name
	if (!getboard(board))
		http_fatal("错误的文件名3");
	if (strncmp(name, "M.", 2) && strncmp(name, "G.", 2))
		http_fatal("错误的参数1");
	if (strstr(name, "..") || strstr(name, "/"))
		http_fatal("错误的参数2");
	snprintf(filename, sizeof (filename), "boards/%s/%s", board, name);
	sprintf(dir, "boards/%s/.DIR", board);
	if (cache_header(file_time(filename), 86400))
		return 0;
//      dirty code for set a bbscon run environ for fshowcon
	parm_add("F", name);
	parm_add("B", board);
//      dirty code end
	printf("Content-type: application/x-javascript; charset=%s\r\n\r\n",
	       CHARSET);
	printf("<!--\n");
	fputs("docStr=\"", stdout);
	old_quote_quote = quote_quote;
	quote_quote = 1;
	cginame = "bbscon";
	binarylinkfile(getparm2("F", "file"));
	if(*getparm("strip"))
		stripHeadTail=1;
	fshowcon(stdout, filename, 2);
	stripHeadTail=0;
	cginame = "boards";
	quote_quote = old_quote_quote;
	puts("\";");
	puts("//-->");
	return 0;
}
