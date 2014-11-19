#include "bbslib.h"

int
bbstop10_main()
{
	struct mmapfile mf = { ptr:NULL };
	html_header(1);
	check_msg();
	MMAP_TRY {
		if (mmapfile("wwwtmp/ctopten", &mf) < 0) {
			MMAP_UNTRY;
			http_fatal("ÎÄ¼þ´íÎó");
		}
		fwrite(mf.ptr, mf.size, 1, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);

	http_quit();
	return 0;
}
