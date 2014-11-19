#include "bbslib.h"

int
bbslastip_main()
{
	printf("Content-type: application/x-javascript; charset=%s\r\n\r\n", CHARSET);
	printf("document.l.lastip%s.value='%s';\n", getparm("n"), realfromhost);
	//printf("document.write('%s');", realfromhost);
	return 0;
}
