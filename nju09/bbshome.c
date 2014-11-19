#include "bbslib.h"

char *
userid_str2(char *s)
{
	static char buf[512];
	char buf2[256], tmp[256], *ptr, *ptr2;
	strsncpy(tmp, s, 255);
	buf[0] = 0;
	ptr = strtok(tmp, " ,();\r\n\t");
	while (ptr && strlen(buf) < 400) {
		if ((ptr2 = strchr(ptr, '.'))) {
			ptr2[1] = 0;
			strcat(buf, ptr);
		} else {
			ptr = nohtml(ptr);
			sprintf(buf2,
				"<a href=qry?U=%s target=boardpage>%s</a>",
				ptr, ptr);
			strcat(buf, buf2);
		}
		ptr = strtok(0, " ,();\r\n\t");
		if (ptr != NULL)
			strcat(buf, " ");
	}
	return buf;
}

int
bbshome_main()
{
	char board[80], *path_info, *filename = NULL, *ptr, genbuf[STRLEN * 2];
	struct boardmem *x1;
	int i;
	char bmbuf[(IDLEN + 1) * 4];
	struct mmapfile mf = { ptr:NULL };
	changemode(READING);
	getparmboard(board, sizeof(board));
	if (strcmp(board, "")) {
		x1 = getboard2(board);
		if (x1 == NULL) {
			html_header(1);
			nosuchboard(board, "home");
		}
		if (!has_read_perm_x(currentuser, x1)) {
			html_header(1);
			noreadperm(board, "home");
		}
		sprintf(genbuf,
			MY_BBS_HOME "/ftphome/root/boards/%s/html/index.htm",
			board);
		if (*getparm("t") != 'b') {
			if (!file_exist(genbuf)) {
				return bbsdoc_main();
			}
/*			if (!strcmp(board, "Euro2004") && strcmp(currentuser->userid, "yuhuan") && strcmp(currentuser->userid, "they")) {
				return bbsdoc_main();
			} */
			html_header(1);
			check_msg();
			printf
			    ("<frameset rows=\"%d, *\" frameSpacing=0 frameborder=0 border=0 >\n"
			     "<frame scrolling=no marginwidth=0 marginheight=0 name=bar "
			     "src=\"home?B=%d&t=b\">\n"
			     "<frame src=\"home/boards/%s/html/index.htm\" name=boardpage>\n"
			     "</frameset>", 15 + (wwwstylenum % 2) * 2, getbnumx(x1),
			     board);
		} else {
			html_header(1);
			check_msg();
			printf("<body topmargin=0 MARGINHEIGHT=0>\n");
			printf("<a href=boa?secstr=%s target=f3>%s</a> --",
			       x1->header.sec1,
			       nohtml(getsectree(x1->header.sec1)->title));
			printf("<a href=home?B=%d target=f3>%s版</a> ",
			       getbnumx(x1), board);
			printf("版主[%s] ",
			       userid_str2(bm2str(bmbuf, &(x1->header))));
			printf
			    ("<a href=bbsbrdadd?B=%d target=f3>预定本版</a> \n",
			     getbnumx(x1));
			printf("<a href=doc?B=%d target=f3>讨论区</a> ",
			       getbnumx(x1));
			printf("<a href=bbsgdoc?B=%d target=f3>文摘区</a> ",
			       getbnumx(x1));
			printf("<a href=bbs0an?path=%s target=f3>精华区</a> ",
			       anno_path_of(board));
			sprintf(genbuf, "boards/.backnumbers/%s/.DIR", board);
			if (!politics(board) && file_exist(genbuf))
				printf
				    ("<a href=bbsbknsel?B=%d target=f3>过刊区</a> ",
				     getbnumx(x1));
		}
		http_quit();
	}
	path_info = getsenv("SCRIPT_URL");
	path_info = strchr(path_info + 1, '/');
	if (NULL == path_info)
		http_fatal("错误的文件名");
	path_info = utf8_decode(path_info);
	if (!strncmp(path_info, "/bbshome/", 9))
		path_info += 9;
	else if (!strncmp(path_info, "/home/", 6))
		path_info += 6;
	else
		http_fatal("错误的文件名");
	if (!strncmp(path_info, "boards/", 7)) {
		if ((ptr = strchr(path_info + 7, '/')) != NULL) {
			*ptr = 0;
			ptr++;
		} else
			ptr = "";
		strsncpy(board, path_info + 7, 32);
		if (getboard(board) == NULL)
			http_fatal("错误的文件名");
		i = strlen(ptr);
		filename = malloc(i + 100);
		if (filename == NULL)
			http_fatal("错误的文件名");
		sprintf(filename, MY_BBS_HOME "/ftphome/root/boards/%s/%s",
			board, ptr);
	} else if (!strncmp(path_info, "pub/", 4)) {
		i = strlen(path_info);
		filename = malloc(i + 100);
		if (filename == NULL)
			http_fatal("错误的文件名");
		sprintf(filename, MY_BBS_HOME "/ftphome/root/%s", path_info);
	} else
		http_fatal("错误的文件名");

	if (strstr(filename, "..")) {
		free(filename);
		http_fatal("错误的文件名");
	}

	if (file_isdir(filename)) {
		if (filename[strlen(filename) - 1] == '/')
			strcat(filename, "index.htm");
		else {
			free(filename);
			http_fatal("错误的文件名");
			//strcat(filename,"/index.htm");
		}
	}

	if (!strcmp(filename, MY_BBS_HOME "/ftphome/root/boards/Olympic/html/index.htm")) {
		free(filename);
		return bbsolympic_main();
	}
	
	if (file_size(filename) > 1024 * 1024 * 2) {
		free(filename);
		http_fatal("文件过大，请用 ftp 下载");
	}
//      no_outcache();
	if (cache_header(file_time(filename), 600)) {
		free(filename);
		return 0;
	}
	MMAP_TRY {
		if (mmapfile(filename, &mf)) {
			MMAP_UNTRY;
			free(filename);
			http_fatal("错误的文件名");
		}
		printf("Content-type: %s\r\n", get_mime_type(filename));
		printf("Content-Length: %d\r\n\r\n",mf.size);
		fwrite(mf.ptr, 1, mf.size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END {
		free(filename);
		mmapfile(NULL, &mf);
	}
	return 0;
}
