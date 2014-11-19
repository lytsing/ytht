#include <sys/ipc.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include "ythtbbs.h"

void
getrandomint(unsigned int *s)
{
#ifdef LINUX
	int fd;
	fd = open("/dev/urandom", O_RDONLY);
	read(fd, s, 4);
	close(fd);
#else
	srandom(getpid() - 19751016);
	*s = random();
#endif
}

void
getrandomstr(unsigned char *s)
{
	int i;
#ifdef LINUX
	int fd;
	fd = open("/dev/urandom", O_RDONLY);
	read(fd, s, 30);
	close(fd);
	for (i = 0; i < 30; i++)
		s[i] = 65 + s[i] % 26;
#else
	time_t now_t;
	now_t = time(NULL);
	srandom(now_t - 19751016);
	for (i = 0; i < 30; i++)
		s[i] = 65 + random() % 26;
#endif
	s[30] = 0;
}

int
init_newtracelogmsq()
{
	int msqid;
	msqid = msgget(getBBSKey(BBSLOG_MSQ), IPC_CREAT | 0664);
	if (msqid < 0)
		return -1;
	return msqid;
}

void
newtrace(s)
char *s;
{
	static int disable = 0;
	static int msqid = -1;
	time_t dtime;
	char buf[512];
	char timestr[16];
	char *ptr;
	struct tm *n;
	struct mymsgbuf *msg = (struct mymsgbuf *) buf;
	if (disable)
		return;
	time(&dtime);
	n = localtime(&dtime);
	sprintf(timestr, "%02d:%02d:%02d", n->tm_hour, n->tm_min, n->tm_sec);
	snprintf(msg->mtext, sizeof (buf) - sizeof (msg->mtype),
		 "%s %s\n", timestr, s);
	ptr = msg->mtext;
	while ((ptr = strchr(ptr, '\n'))) {
		if (!ptr[1])
			break;
		*ptr = '*';
	}
	msg->mtype = 1;
	if (msqid < 0) {
		msqid = init_newtracelogmsq();
		if (msqid < 0) {
			disable = 1;
			return;
		}
	}
	msgsnd(msqid, msg, strlen(msg->mtext), IPC_NOWAIT | MSG_NOERROR);
	return;
}

void
tracelog(char *fmt, ...)
{
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof (buf), fmt, ap);
	va_end(ap);
	newtrace(buf);
}

int
deltree(const char *dst)
{
	char rpath[PATH_MAX + 1 + 10], buf[PATH_MAX + 1];
	char diskrpath[PATH_MAX + 1];
	int i = 0, fd;
	int dnum, len;
	static char *const (disks[]) = {
	MY_BBS_HOME "/home/",
		    MY_BBS_HOME "/0Announce/",
		    MY_BBS_HOME "/boards/",
		    MY_BBS_HOME "/mail/", MY_BBS_HOME "/", NULL};

	//Verify the real path. Is it in the disks list?
	if (realpath(dst, rpath) == NULL)
		return -1;
	len = strlen(rpath);
	if (rpath[len - 1] == '/')
		rpath[len - 1] = 0;
	for (dnum = 0; disks[dnum]; dnum++) {
		if (realpath(disks[dnum], diskrpath) == NULL)
			return -1;
		len = strlen(diskrpath);
		if (diskrpath[len - 1] != '/') {
			diskrpath[len] = '/';
			diskrpath[len + 1] = 0;
			len++;
		}
		if (!strncmp(rpath, diskrpath, len))
			break;
	}
	if (!disks[dnum])
		return -1;
	if (strncmp(rpath, diskrpath, len))
		return -1;

	//If it is just a symbol link, remove it.
	if (lfile_islnk(dst)) {
		unlink(dst);
		return 0;
	}

	strcpy(buf, rpath);
	//Make the file name in the junk directory
	memmove(rpath + len + 6, rpath + len, sizeof (rpath) - len - 6);
	memcpy(rpath + len, ".junk/", 6);
	len += 6;
	normalize(rpath + len);
	rpath[PATH_MAX - 10] = 0;

	len = strlen(rpath);
	i = 0;
	if (file_isdir(dst)) {
		while (mkdir(rpath, 0770) != 0 && i < 1000) {
			sprintf(rpath + len, "+%d", i);
			i++;
		}
		if (i == 1000)
			return -1;
	} else {
		while (((fd = open(rpath, O_CREAT | O_EXCL | O_WRONLY, 0660)) <
			0) && i < 1000) {
			sprintf(rpath + len, "+%d", i);
			i++;
		}
		if (i == 1000)
			return -1;
		close(fd);
	}
	rename(buf, rpath);
	//How should rpath be a link? Race condition? Maybe. If so, remove it
	if (lfile_islnk(rpath))
		unlink(rpath);
	return 0;
}
