#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>

#include "ythtbbs.h"

static int isoverride(struct override *o, char *id);

char *
sethomepath(char *buf, const char *userid)
{
	sprintf(buf, MY_BBS_HOME "/home/%c/%s", mytoupper(userid[0]), userid);
	return buf;
}

char *
sethomefile(char *buf, const char *userid, const char *filename)
{
	sprintf(buf, MY_BBS_HOME "/home/%c/%s/%s", mytoupper(userid[0]), userid,
		filename);
	return buf;
}

char *
setmailfile(char *buf, const char *userid, const char *filename)
{
	sprintf(buf, MY_BBS_HOME "/mail/%c/%s/%s", mytoupper(userid[0]), userid,
		filename);
	return buf;
}

int
saveuservalue(char *userid, char *key, char *value)
{
	char path[256];
	sethomefile(path, userid, "values");
	return savestrvalue(path, key, value);
}

int
readuservalue(char *userid, char *key, char *value, int size)
{
	char path[256];
	sethomefile(path, userid, "values");
	return readstrvalue(path, key, value, size);
}

char *
cuserexp(int exp)
{
	int expbase = 0;

	if (exp == -9999)
		return "没等级";
	if (exp <= 100 + expbase)
		return "新手上路";
	if (exp <= 450 + expbase)
		return "一般站友";
	if (exp <= 850 + expbase)
		return "中级站友";
	if (exp <= 1500 + expbase)
		return "高级站友";
	if (exp <= 2500 + expbase)
		return "老站友";
	if (exp <= 3000 + expbase)
		return "长老级";
	if (exp <= 5000 + expbase)
		return "本站元老";
	return "开国大老";
}

char *
cperf(int perf)
{
	if (perf == -9999)
		return "没等级";
	if (perf <= 5)
		return "赶快加油";
	if (perf <= 12)
		return "努力中";
	if (perf <= 35)
		return "还不错";
	if (perf <= 50)
		return "很好";
	if (perf <= 90)
		return "优等生";
	if (perf <= 140)
		return "太优秀了";
	if (perf <= 200)
		return "本站支柱";
	if (perf <= 500)
		return "神～～";
	return "机器人！";
}

int
countexp(struct userec *urec)
{
	int exp;

	if (!strcmp(urec->userid, "guest"))
		return -9999;
	exp =
	    urec->numposts + urec->numlogins / 5 + (time(0) -
						    urec->firstlogin) / 86400 +
	    urec->stay / 3600;
	return exp > 0 ? exp : 0;
}

int
countperf(struct userec *urec)
{
	int perf;
	int reg_days;

	if (!strcmp(urec->userid, "guest"))
		return -9999;
	reg_days = (time(0) - urec->firstlogin) / 86400 + 1;
	perf =
	    ((float) (urec->numposts) / (float) urec->numlogins +
	     (float) urec->numlogins / (float) reg_days) * 10;
	return perf > 0 ? perf : 0;
}

int
countlife(struct userec *urec)
{
	int value;

	/* if (urec) has XEMPT permission, don't kick it */
	if ((urec->userlevel & PERM_XEMPT)
	    || strcmp(urec->userid, "guest") == 0)
		return 999;
	value = (time(0) - urec->lastlogin) / 60;	/* min */
	if (urec->numlogins <= 3) {
		if ((urec->userlevel & PERM_LOGINOK))
			return (30 * 1440 - value) / 1440;
		return (15 * 1440 - value) / 1440;
	}
	if (!(urec->userlevel & PERM_LOGINOK))
		return (30 * 1440 - value) / 1440;
	value = (120 * 1440 - value) / 1440 +
	    min(urec->numdays, (unsigned short) 800) + urec->extralife5 * 5;
	return min(value, 998);
}

int
userlock(char *userid, int locktype)
{
	char path[256];
	int fd;
	sethomefile(path, userid, ".lock");
	fd = open(path, O_RDONLY | O_CREAT, 0660);
	if (fd == -1)
		return -1;
	flock(fd, locktype);
	return fd;
}

int
userunlock(char *userid, int fd)
{
	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}

static int
checkbansitefile(const char *addr, const char *filename)
{
	FILE *fp;
	char temp[STRLEN];
	if ((fp = fopen(filename, "r")) == NULL)
		return 0;
	while (fgets(temp, STRLEN, fp) != NULL) {
		strtok(temp, " \n");
		if ((!strncmp(addr, temp, 16))
		    || (!strncmp(temp, addr, strlen(temp))
			&& temp[strlen(temp) - 1] == '.')
		    || (temp[0] == '.' && strstr(addr, temp) != NULL)) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

int
checkbansite(const char *addr)
{
	return checkbansitefile(addr, MY_BBS_HOME "/.bansite")
	    || checkbansitefile(addr, MY_BBS_HOME "/bbstmpfs/dynamic/bansite");
}

int
userbansite(const char *userid, const char *fromhost)
{
	char path[STRLEN];
	FILE *fp;
	char buf[STRLEN];
	int i, deny;
	char addr[STRLEN], mask[STRLEN], allow[STRLEN];
	char *tmp[3] = { addr, mask, allow };
	unsigned int banaddr, banmask;
	unsigned int from;
	from = inet_addr(fromhost);
	sethomefile(path, userid, "bansite");
	if ((fp = fopen(path, "r")) == NULL)
		return 0;
	while (fgets(buf, STRLEN, fp) != NULL) {
		i = mystrtok(buf, ' ', tmp, 3);
		if (i == 1) {	//单独 ip
			banaddr = inet_addr(addr);
			banmask = inet_addr("255.255.255.255");
			deny = 1;
		} else if (i == 2) {
			banaddr = inet_addr(addr);
			banmask = inet_addr(mask);
			deny = 1;
		} else if (i == 3) {	//带 allow 项
			banaddr = inet_addr(addr);
			banmask = inet_addr(mask);
			deny = !strcmp(allow, "allow");
		} else		//空行？
			continue;
		if ((from & banmask) == (banaddr & banmask)) {
			fclose(fp);
			return deny;
		}
	}
	fclose(fp);
	return 0;
}

void
logattempt(const char *user, char *from, char *zone, time_t time)
{
	char buf[256], filename[80];
	int fd, len;

	sprintf(buf, "system passerr %s", from);
	newtrace(buf);
	snprintf(buf, 256, "%-12.12s  %-30s %-16s %-6s\n",
		 user, Ctime(time), from, zone);
	len = strlen(buf);
	if ((fd =
	     open(MY_BBS_HOME "/" BADLOGINFILE, O_WRONLY | O_CREAT | O_APPEND,
		  0644)) >= 0) {
		write(fd, buf, len);
		close(fd);
	}
	sethomefile(filename, user, BADLOGINFILE);
	if ((fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644)) >= 0) {
		write(fd, buf, len);
		close(fd);
	}
}

static int
isoverride(struct override *o, char *id)
{
	if (strcasecmp(o->id, id) == 0)
		return 1;
	return 0;
}

int
inoverride(char *who, char *owner, char *file)
{
	char buf[80];
	struct override o;
	sethomefile(buf, owner, file);
	if (search_record(buf, &o, sizeof (o), (void *) isoverride, who) != 0)
		return 1;
	return 0;
}

int
saveuserdata(char *uid, struct userdata *udata)
{
	char newfname[80];
	char fname[80];
	int fd;
	sethomefile(newfname, uid, "newuserdata");
	sethomefile(fname, uid, "userdata");
	fd = open(newfname, O_WRONLY | O_CREAT, 0660);
	if (fd == -1) {
		errlog("open userdata error %s", uid);
		return -1;
	}
	write(fd, udata, sizeof (struct userdata));
	close(fd);
	rename(newfname, fname);
	return 0;
}

int
loaduserdata(char *uid, struct userdata *udata)
{
	char buf[80];
	int fd;
	sethomefile(buf, uid, "userdata");
	bzero(udata, sizeof (struct userdata));
	fd = open(buf, O_RDONLY);
	if (fd == -1) {
		return -1;
	}
	read(fd, udata, sizeof (struct userdata));
	close(fd);
	return 0;
}

int
lock_passwd()
{
	int lockfd;

	lockfd = open(MY_BBS_HOME "/.PASSWDS.lock", O_RDWR | O_CREAT, 0600);
	if (lockfd < 0) {
		errlog("uhash lock err: %s", strerror(errno));
		exit(1);
	}
	flock(lockfd, LOCK_EX);
	return lockfd;
}

int
unlock_passwd(int fd)
{
	flock(fd, LOCK_UN);
	close(fd);
	return 0;
}

/*返回插入的passwd位置，第一个编号为 1
  id已经被注册，返回 0
  插入失败，返回 -1
*/
int
insertuserec(const struct userec *x)
{
	int lockfd, fd;
	int i;
	lockfd = lock_passwd();
	if (finduseridhash(x->userid) > 0) {
		unlock_passwd(lockfd);
		return 0;
	}
	i = deluseridhash("");
	if (i <= 0) {
		unlock_passwd(lockfd);
		return -1;
	}
	fd = open(".PASSWDS", O_WRONLY);
	if (fd == -1) {
		insertuseridhash("", i, 1);
		unlock_passwd(lockfd);
		return -1;
	}
	if (lseek(fd, (i - 1) * sizeof (struct userec), SEEK_SET) == -1) {
		insertuseridhash("", i, 1);
		close(fd);
		unlock_passwd(lockfd);
		return -1;
	}
	if (write(fd, x, sizeof (struct userec)) != sizeof (struct userec)) {
		close(fd);
		unlock_passwd(lockfd);
		errlog("can't write!");
		return -1;
	};
	close(fd);
	insertuseridhash(x->userid, i, 1);
	unlock_passwd(lockfd);
	return i;
}

//根据userid，填充urec指针到，指向passwdptr中相应条目，该结构只读
//返回对应偏移量，第一个编号为 1
//如果用户已经kickout，那也找不到，找不到返回 0
int
getuser(const char *userid, struct userec **urec)
{
	int i;
	if (userid[0] == 0 || strchr(userid, '.')) {
		if (urec != NULL)
			*urec = NULL;
		return 0;
	}
	i = finduseridhash(userid);
	if (i > 0 && i <= MAXUSERS
	    && !strcasecmp(passwdptr[i - 1].userid, userid)
	    && !(passwdptr[i - 1].kickout)) {
		if (urec != NULL) {
			*urec = (struct userec *) (passwdptr + (i - 1));
		}
		return i;
	}
	return 0;
}

//根据uid，填充urec指针到，指向passwdptr中相应条目，该结构只读
//返回对应偏移量，第一个编号为 1
//如果用户已经kickout，那也找不到，找不到返回 0
int
getuserbynum(const int uid, struct userec **urec)
{
	if (uid > 0 && uid <= MAXUSERS && !(passwdptr[uid - 1].kickout)) {
		if (urec != NULL) {
			*urec = (struct userec *) (passwdptr + (uid - 1));
		}
		return uid;
	}
	return 0;
}

//找到userid按照passwd的位置, 第一个位置编号为 1
//若kickout则返回负值
//用于新用户注册时预检是否可以注册
//找不到返回 0 
int
user_registered(const char *userid)
{
	int i;
	if (userid[0] == 0 || strchr(userid, '.')) {
		return 0;
	}
	i = finduseridhash(userid);
	if (i > 0 && i <= MAXUSERS
	    && !strcasecmp(passwdptr[i - 1].userid, userid)) {
		if (passwdptr[i - 1].kickout)
			return -i;
		else
			return i;
	}
	return 0;
}

//kickout不从hash中删除用户，用户flags[0] KICKOUT_FLAG位 置1
//dietime置为当前时间
//不修改hash
//24小时后，调用deluserec从passwd中删除
int
kickoutuserec(const char *userid)
{
	int i;
	struct userec tmpuser;
	time_t now_t;
	int fd;
	char buf[1024];
	i = finduseridhash(userid);
	if (i <= 0 || i > MAXUSERS) {	//查找hash失败
		return -1;
	}
	if (passwdptr[i - 1].kickout)	//已经被kick了
		return 0;
	now_t = time(NULL);
	memcpy(&tmpuser, &(passwdptr[i - 1]), sizeof (struct userec));
	tmpuser.kickout = now_t;
	fd = open(".PASSWDS", O_WRONLY);
	if (fd == -1) {
		return -1;
	}
	if (lseek(fd, (i - 1) * sizeof (struct userec), SEEK_SET) == -1) {
		close(fd);
		return -1;
	}
	sprintf(buf, "mail/%c/%s", mytoupper(userid[0]), userid);
	deltree(buf);
	sprintf(buf, "home/%c/%s", mytoupper(userid[0]), userid);
	deltree(buf);
	write(fd, &tmpuser, sizeof (struct userec));
	close(fd);
	return 0;
}

//从passwd和hash中删除条目
//成功返回 0，失败返回 -1
int
deluserec(const char *userid)
{
	int i;
	time_t now_t;
	struct userec tmpuser;
	int fd, lockfd;
	lockfd = lock_passwd();
	i = finduseridhash(userid);
	if (i <= 0 || i > MAXUSERS) {
		errlog("del user error %d", i);
		unlock_passwd(lockfd);
		return 0;
	}
	now_t = time(NULL);
	if (!(passwdptr[i - 1].kickout)
	    || (now_t - passwdptr[i - 1].kickout <= 86400 - 3600)) {
		errlog("del user error 2 %d", (int) passwdptr[i - 1].kickout);
		unlock_passwd(lockfd);
		return 0;
	}
	if (deluseridhash(userid) <= 0) {
		errlog("del user error 3 %s", userid);
		unlock_passwd(lockfd);
		return -1;
	}
	fd = open(".PASSWDS", O_WRONLY);
	if (fd == -1) {
		insertuseridhash(userid, i, 1);
		errlog("del user error open");
		unlock_passwd(lockfd);
		return -1;
	}
	if (lseek(fd, (i - 1) * sizeof (struct userec), SEEK_SET) == -1) {
		insertuseridhash(userid, i, 1);
		errlog("del user error seek");
		close(fd);
		unlock_passwd(lockfd);
		return -1;
	}
	bzero(&tmpuser, sizeof (struct userec));
	write(fd, &tmpuser, sizeof (struct userec));
	close(fd);
	insertuseridhash("", i, 1);
	unlock_passwd(lockfd);
	return 0;
}

/* 修改 x->userid 对应条目为 x
 * 即修改不修改 id
 * 也不修改 hash
 * 如果 usernum 为 0，从hash中查找，否则直接检查对应位置是否是该用户
 * usernum从 1 开始计数
 */
int
updateuserec(const struct userec *x, const int usernum)
{
	int i = usernum, fd;
	if (i == 0)
		i = finduseridhash(x->userid);
	if (i <= 0 || i > MAXUSERS
	    || strcasecmp(passwdptr[i - 1].userid, x->userid)) {
		errlog("update user error. %s %s", passwdptr[i - 1].userid,
		       x->userid);
		return -1;
	}
	if (passwdptr[i - 1].kickout) {
		return -1;
	}
	fd = open(".PASSWDS", O_WRONLY);
	if (fd == -1) {
		return -1;
	}
	if (lseek(fd, (i - 1) * sizeof (struct userec), SEEK_SET) == -1) {
		close(fd);
		return -1;
	}
	write(fd, x, sizeof (struct userec) - sizeof (time_t));
	//FIX ME, maybe it's a bug for 64-bit mode
	close(fd);
	return 0;
}

int
apply_passwd(int (*fptr) (const struct userec *, char *), char *arg)
{
	int i;
	int ret;
	int count;
	count = 0;
	for (i = 1; i <= MAXUSERS; i++) {
		ret = (*fptr) (&(passwdptr[i - 1]), arg);
		if (ret == -1)
			break;
		if (ret == 0)
			count++;
	}
	return count;
}

int
has_fill_form(char *userid)
{
	DIR *dirp;
	struct dirent *direntp;
	FILE *fp;
	int i = 1;
	char tmp[256], buf[256], *ptr;
	dirp = opendir(SCANREGDIR);
	if (dirp == NULL)
		return 0;
	while (i) {
		if ((direntp = readdir(dirp)) != NULL)
			snprintf(tmp, 256, "%s%s", SCANREGDIR, direntp->d_name);
		else {
			snprintf(tmp, 256, "%s/new_register", MY_BBS_HOME);
			i = 0;
		}
		if ((fp = fopen(tmp, "r")) != NULL) {
			while (1) {
				if (fgets(buf, 256, fp) == 0)
					break;
				if ((ptr = strchr(buf, '\n')) != NULL)
					*ptr = '\0';
				if (strncmp(buf, "userid: ", 8) == 0 &&
				    strcmp(buf + 8, userid) == 0) {
					fclose(fp);
					closedir(dirp);
					return 1;
				}
			}
			fclose(fp);
		}
	}
	closedir(dirp);
	return 0;
}
