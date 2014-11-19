#include "bbslib.h"

int
bbsicon_main()
{
	char board[40], buf[STRLEN * 2];
	struct boardmem *x1;
	struct mmapfile mf = { ptr:NULL };
	getparmboard(board, sizeof (board));
	x1 = getboard2(board);
	if (!x1 || !x1->wwwicon) {
		html_header(1);
		http_fatal("无法打开文件");
		return 0;
	}
	sprintf(buf, "ftphome/root/boards/%s/html/icon.gif", board);

	if (cache_header(file_time(buf), 10000)) {
		return 0;
	}
	if (file_size(buf) > 1024 * 20) {
		html_header(1);
		http_fatal("文件过大，请用 ftp 下载");
	}
	MMAP_TRY {
		if (mmapfile(buf, &mf)) {
			MMAP_UNTRY;
			http_fatal("错误的文件名");
		}
		printf("Content-type: %s\r\n", get_mime_type("icon.gif"));
		printf("Content-Length: %d\r\n\r\n", mf.size);
		fwrite(mf.ptr, 1, mf.size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END {
		mmapfile(NULL, &mf);
	}
	return 0;
}
