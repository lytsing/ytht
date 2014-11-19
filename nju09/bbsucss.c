#include "bbslib.h"


int
bbsucss_main()
{
	char  *path_info, filename[255] ;
	path_info = getsenv("SCRIPT_URL");
	path_info = strrchr(path_info , '/')+1;
	printf("Content-type: text/css\r\n\r\n");
	if (!loginok || isguest) {
		if(!strcmp(path_info,"uleft.css"))
			strcpy(filename,HTMPATH"left.css");
		else if(!strcmp(path_info,"ubbs.css"))
			strcpy(filename,HTMPATH"bbs.css");
		else if(!strcmp(path_info,"upercor.css"))
			strcpy(filename,HTMPATH"bbs.css");
		else
			return 0;
	} else {
		sethomefile(filename, currentuser->userid, path_info);
		if (!file_exist(filename)){
			if(!strcmp(path_info,"uleft.css"))
				strcpy(filename,HTMPATH"left.css");
			else if(!strcmp(path_info,"ubbs.css"))
				strcpy(filename,HTMPATH"bbs.css");
			else if(!strcmp(path_info,"upercor.css"))
				strcpy(filename,HTMPATH"bbs.css");
			else
				return 0;
		}
	}

	showfile(filename);
		return 0;
}
