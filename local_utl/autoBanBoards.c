//自动按照上限封禁/解封版面的程序
//版面的上限可以在创建版面和修改版面属性的地方设定
//crontab 的写法
//2 2,8,14,20 * * * /home/bbs/bin/autoBanBoards 2 &>/dev/null
//5 0,1,3,4,5,6,7,9,10,11,12,13,15,16,17,18,19,21,22,23 * * *  /home/bbs/bin/autoBanBoards 1 &>/dev/null

#include "bbs.h"
int level = 0;			//0: 仅进行解封; 1: 仅进行解封和警告; 2: 解封/警告/封禁

struct bbsinfo bbsinfo;

int
getlimit(struct boardheader *bh)
{
	if (!bh->limitchar)
		return 3000;
	return bh->limitchar * 100;
}

int
warnmsg(struct boardheader *bh, int action, int limit, int limit_new)
{
	char buf[512];
	int i, uid;
	struct msghead head;
	struct user_info *uinfo;
	int pos;
	head.time = time(0);
	head.sent = 0;
	head.mode = 0;
	strcpy(head.id, "系 统");
	head.frompid = 0;
	head.topid = 0;
	snprintf(buf, sizeof (buf),
		 "您担任版主的 %s 版版面文章数%s超限, 超限版面会被自动封闭, 请即时整理\n"
		 "  3 天前的文章数上限 %d, 12 小时前的文章数上限 %d, \n"
		 "  所有文章总数不要超过 %d.\n", bh->filename,
		 (action == 1) ? "即将" : "已经", limit, limit_new,
		 limit_new + 500);
	for (i = 0; i < BMNUM; i++) {
		if (!bh->bm[i][0])
			continue;
		if (!strcasecmp(bh->bm[i], "SYSOP"))
			continue;
		uid = getuser(bh->bm[i], NULL);
		if (uid <= 0)
			continue;
		save_msgtext(bh->bm[i], &head, buf);
		if (!(uinfo = queryUIndex(uid, NULL, 0, &pos)))
			continue;
		if (uinfo->pid > 1)
			kill(uinfo->pid, SIGUSR2);
		else
			(uinfo->unreadmsg)++;
	}
	return 0;
}

int
checklimit(int fd, int count, int limit, int flex, time_t t)
{
	int retv = 0;
	struct fileheader fh;
	if (count < limit + 2 * flex / 3)
		return 0;

	lseek(fd, sizeof (fh) * limit, SEEK_SET);
	read(fd, &fh, sizeof (fh));
	if (fh.filetime <= t + 12 * 3600)
		retv = 1;
	if (fh.filetime <= t) {
		if (count < limit + flex)
			retv = 1;
		else
			retv = 2;
	}
	return retv;
}

int
shouldBan(struct boardheader *bh, int oldvalue)
{
	int limit, limit_new, count, fd = -1, ban = 0;
	char buf[256];
	time_t t = time(NULL);
	if (!oldvalue && !level)
		return oldvalue;
	limit = getlimit(bh);
	limit_new = limit + limit / 3;
	sprintf(buf, MY_BBS_HOME "/boards/%s/.DIR", bh->filename);
	count = file_size(buf) / sizeof (struct fileheader);

	if (count <= limit)
		return 0;
	if (count > limit_new + 500)
		goto SHOULDBAN;

	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return 0;
	ban = max(ban, checklimit(fd, count, limit, 500, t - 3 * 24 * 3600));
	ban = max(ban, checklimit(fd, count, limit_new, 500, t - 12 * 3600));
	close(fd);

	if (ban == 2)
		goto SHOULDBAN;
	if (ban == 1)
		goto WOULDBAN;
	return 0;
      WOULDBAN:
	if (oldvalue != 1) {
		if (!level)
			return 0;
		warnmsg(bh, 1, limit, limit_new);
	}
	return 1;
      SHOULDBAN:
	if (oldvalue != 2) {
		if (level < 2)
			goto WOULDBAN;
		warnmsg(bh, 2, limit, limit_new);
	}
	return 2;
}

int
genfiles()
{
	FILE *fp;
	if ((fp = fopen("bbstmpfs/dynamic/banAlert.www.new", "w")) == NULL)
		return -1;
	fprintf(fp, "<b>版面文章超限报警</b><br>"
		"<font class=red>!!!</font> 版面文章将要超限, 版面将在 5 小时内被禁止发文.<br>"
		"<font class=red><b>!!!</b></font> 版面文章超限, 版面已经被禁止发文.<br>"
		"<br><br>普通版面"
		"<li><u>3 天前的文章总数</u>上限 3000, "
		"<li><u>5 小时前的文章总数</u>上限 4000, "
		"<li><u>所有文章总数</u>不得超过 4000 + 500."
		"<br>版面文章数有特殊规定者"
		"<li><u>3 天前的文章总数</u>上限若为 x,"
		"<li><u>5 小时前的文章总数</u>上限 x + x/3,"
		"<li><u>所有文章总数</u>不得超过 x + x/3 + 500.");
	fprintf(fp, "<br><br>上次检查时间：%s<br>", Ctime(time(NULL)));
	fclose(fp);
	rename("bbstmpfs/dynamic/banAlert.www.new",
	       "bbstmpfs/dynamic/banAlert.www");
	return 0;
}

//argv[1]: -1: 清理所有封禁/警告标记; 0: 只进行解封; 
//1: 解封和警告; 2: 封禁/解封和警告
int
main(int argn, char **argv)
{
	int i;
	int oldvalue;
	struct boardheader *bh;
	char *(valuestr[]) = { "解封", "警告", "封禁" };

	printf("自动封禁/解封版面\n运行时间: %s\n", Ctime(time(NULL)));

	if (argn >= 2)
		level = atoi(argv[1]);
	if (level > 2 || level < -1)
		level = 0;
	if (initbbsinfo(&bbsinfo) < 0) {
		printf("Failed to attach shm.\n");
		return -1;
	}
	if (uhash_uptime() == 0) {
		printf("Failed to access uhash.\n");
		return -1;
	}

	chdir(MY_BBS_HOME);
	for (i = 0; i < MAXBOARD; i++) {
		bh = &bbsinfo.bcache[i].header;
		if (!bh->filename[0])
			continue;
		if (!strcasecmp(bh->filename, "syssecurity")) {
			bbsinfo.bcache[i].ban = 0;
			continue;
		}
		if (level == -1) {
			bbsinfo.bcache[i].ban = 0;
			continue;
		}
		oldvalue = bbsinfo.bcache[i].ban;
		bbsinfo.bcache[i].ban = shouldBan(bh, bbsinfo.bcache[i].ban);
		if (bbsinfo.bcache[i].ban || oldvalue)
			printf("%-20.20s %s ==> %s\n", bh->filename,
			       valuestr[oldvalue],
			       valuestr[(int) bbsinfo.bcache[i].ban]);
	}
	genfiles();
	return 0;
}
