#include "bbslib.h"
#ifdef ENABLE_MYSQL
int do_eva(char *board, char *file, int star);
int
bbseva_main()
{
	char board[80], file[80];
	int star;
	html_header(1);
	check_msg();
	getparmboard(board, sizeof(board));
	strsncpy(file, getparm("F"), 20);
	if (!file[0])
		strsncpy(file, getparm("file"), 32);
	star = atoi(getparm("star"));
	if (!loginok)
		http_fatal("匆匆过客不能进行本项操作");
	changemode(READING);
	if (!getboard(board))
		http_fatal("错误的讨论区");
	if (hideboard(board))
		http_fatal("隐藏版面就不要评价文章啦!");
	if (star < 1 || star > 5)
		http_fatal("错误的参数");
	if (star == 1)
		star++;
	printf("<center>%s -- 评价文章 [使用者: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	printf("<table><td>");
	do_eva(board, file, star);
	printf("</td></table>");
	printf("[<a href='javascript:history.go(-1)'>返回</a>]");
	http_quit();
	return 0;
}

char *des[6] = { "没评价", "不错", "不错", "很好", "很好", "我顶!" };

int
set_eva(char *board, char *file, int star, int result[2], char *buf)
{
	int count, starchanged, oldstar;
	float avg;
	starchanged = 0;
	if (now_t - atoi(file + 2) > EVADAY * 86400) {
		sprintf(buf, "这么老的文章就别评价了吧,去评点新文章吧!");
		return 0;
	}
	//oldstar = 0/1都认为是未评价状态
	if (bbseva_qset
	    (utmpent, board, file, currentuser->userid, star, &oldstar, &count,
	     &avg) < 0) {
		sprintf(buf, "对文章的评价未能改变");
		return 0;
	}
	if (star == oldstar) {
		sprintf(buf, "您没有改变您对这篇文章的评价");
	} else {
		if (oldstar != 1 && oldstar != 0)
			sprintf(buf, "您把您对这篇文章的评价从 %s 改到 %s",
				des[oldstar], des[star]);
		else
			sprintf(buf, "这篇文章被您评价为 %s", des[star]);
	}
	if (oldstar != star) {
		starchanged = 1;
		result[0] = (int) (50 * avg);
		result[1] = count > 255 ? 255 : count;
	}
	return starchanged;
}

int
do_eva(char *board, char *file, int star)
{
	char dir[256];
	struct fileheader x;
	int result[2], filetime;
	int num = -1;
	int fd, ret;
	filetime = atoi(file + 2);
	sprintf(dir, "boards/%s/.DIR", board);
	fd = open(dir, O_RDWR);
	if (fd < 0)
		http_fatal("错误的参数2");
	flock(fd, LOCK_EX);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &num, 1);
	if (ret == -1) {
		flock(fd, LOCK_UN);
		close(fd);
		http_fatal("错误的文件名");
	}
	lseek(fd, num * sizeof (struct fileheader), SEEK_SET);
	if (set_eva(board, file, star, result, dir)) {
		x.staravg50 = result[0];
		x.hasvoted = result[1];
		write(fd, &x, sizeof (struct fileheader));
	}
	flock(fd, LOCK_UN);
	close(fd);
	printf("%s", dir);
	return 0;
}
#else
int
bbseva_main()
{
	html_header(1);
	http_fatal("请安装MySQL支持环境!");
	return 0;
}
#endif
