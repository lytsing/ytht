#include "bbslib.h"

int
bbsdelmail_main()
{
	int fd;
	struct fileheader f;
	struct fileheader *fhmw, *fhw;
	char path[80], file[80], *ptr, list[40][20], fullpath[256];
	int num, ndelfile = 0;
	struct stat st;
	int count;
	int wcount, wstart;
	int spam;
	struct yspam_ctx *yctx;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("您尚未登录");
	changemode(RMAIL);

	spam=atoi(getparm("spam"));
	strsncpy(list[ndelfile], getparm("file"), 20);
	if (list[ndelfile][0] == 'M')
		ndelfile++;
	for (num = 0; num < 40 && ndelfile < 40; num++) {
		sprintf(file, "F%d", num);
		strsncpy(list[ndelfile], getparm(file), 20);
		if (list[ndelfile][0] == 'M' || list[ndelfile][0] == 'G')
			ndelfile++;
	}
	
	if(-2==get_mailsize(currentuser)){
		errlog("strange user %s",currentuser->userid);
		http_fatal("邮箱内部错误! 请联系系统维护!");
	}

	setmailfile(path, currentuser->userid, ".DIR");
	fd = open(path, O_RDWR);
	if (fd < 0)
		http_fatal("错误的参数2");
	flock(fd, LOCK_EX);
	fstat(fd, &st);
	if (st.st_size % sizeof (struct fileheader) != 0) {
		flock(fd, LOCK_UN);
		close(fd);
		return -1;
	}
	fhmw = malloc(st.st_size);
	if (fhmw == NULL) {
		flock(fd, LOCK_UN);
		close(fd);
		return -1;
	}
	fhw = fhmw;
	count = 0;
	wstart = -1;
	wcount = 0;
	yctx = yspam_init("127.0.0.1");
	while (read(fd, &f, sizeof (struct fileheader)) == sizeof (struct fileheader)) {
		count++;
		ptr = fh2fname(&f);
		for (num = 0; num < ndelfile; num++) {
			if (!strcmp(ptr, list[num]))
				break;
		}
		if (num == ndelfile) {
			if (wstart == -1)
				continue;
			memcpy(fhw, &f, sizeof (struct fileheader));
			wcount++;
			fhw++;
		} else {
			if (wstart == -1)
				wstart = count - 1;
			if (!f.filetime)
				continue;
			setmailfile(fullpath, currentuser->userid, fh2fname(&f));
			update_mailsize_down(&f, currentuser->userid);
			if (spam && yspam_feed_spam(yctx, currentuser->userid, fh2fname(&f)) == 0) {
				unlink(fullpath);
				continue;
			}
			deltree(fullpath);
		}
	}
	yspam_fini(yctx);
	if (wstart != -1) {
		ftruncate(fd, (wstart + wcount) * sizeof (struct fileheader));
		if (wcount > 0) {
			lseek(fd, wstart * sizeof (struct fileheader), SEEK_SET);
			write(fd, fhmw, wcount * sizeof (struct fileheader));
		}
	}
	free(fhmw);
	flock(fd, LOCK_UN);
	close(fd);
	printf("信件已删除.<br><a href=bbsmail>返回所有信件列表</a>\n");
	printf("<script>top.f4.location.reload();</script>");
	http_quit();
	return 0;
}
