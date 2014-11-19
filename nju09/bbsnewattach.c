#include "bbslib.h"

int
bbsnewattach_main()
{
	char *ptr, *path_info;
	int pos;
	char *article;
	char *attachname;
	char filename[1024];
	struct boardmem *bx;
	unsigned int size;
	struct mmapfile mf = { ptr:NULL };

	path_info = getsenv("SCRIPT_URL");
/*
 * /b/999/1232131233/21312323/asfsdfdsaf.jpg
 */
	path_info = strchr(path_info + 1, '/');
	if (NULL == path_info)
		http_fatal("错误的文件名");
	if (!strncmp(path_info, "/b/", 3))
		path_info += 3;
	else
		http_fatal("错误的文件名");
	ptr = strchr(path_info, '/');
	if (NULL == ptr)
		http_fatal("错误的文件名");
	*ptr = 0;
	bx = numboard(path_info);
	if (NULL == bx)
		http_fatal("错误的文件名");
	if (hideboard_x(bx)) {
		if (!has_read_perm_x(currentuser, bx)) {
			http_fatal("错误的文件名");
		}
	}
	path_info = ptr + 1;
	ptr = strchr(path_info, '/');
	if (NULL == ptr)
		http_fatal("错误的文件名");
	*ptr = 0;
	article = path_info;
	path_info = ptr + 1;
	ptr = strchr(path_info, '/');
	if (NULL == ptr)
		http_fatal("错误的文件名");
	*ptr = 0;
	if (strncmp(article, "M.", 2) && strncmp(article, "G.", 2))
		http_fatal("错误的参数1");
	if (strstr(article, "..") || strstr(article, "/"))
		http_fatal("错误的参数2");
	pos = atoi(path_info);
	attachname = ptr + 1;


	sprintf(filename, MY_BBS_HOME"/boards/%s/%s", bx->header.filename, article);
	
	if (cache_header(file_time(filename), 86400))
		return 0;
	MMAP_TRY {
		if (mmapfile(filename, &mf) < 0) {
			MMAP_UNTRY;
			http_fatal("无法打开附件 1");
			MMAP_RETURN(-1);
		}
		if (pos >= mf.size - 4 || pos < 1) {
			mmapfile(NULL, &mf);
			MMAP_UNTRY;
			http_fatal("无法打开附件 2");
			MMAP_RETURN(-1);
		}
		if (mf.ptr[pos - 1] != 0) {
			mmapfile(NULL, &mf);
			MMAP_UNTRY;
			http_fatal("无法打开附件 3");
			MMAP_RETURN(-1);
		}
		size = ntohl(*(unsigned int *) (mf.ptr + pos));
		if (pos + 4 + size > mf.size) {
			mmapfile(NULL, &mf);
			MMAP_UNTRY;
			http_fatal("无法打开附件 4");
			MMAP_RETURN(-1);
		}
		printf("Content-type: %s\r\n", get_mime_type(attachname));
		printf("Content-Length: %d\r\n\r\n", size);
		fwrite(mf.ptr + pos + 4, 1, size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	return 0;
}
