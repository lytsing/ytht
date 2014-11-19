/*    自动解封系统    KCN 1999.7.26 		  */

#include "bbs.h"
#include "sysrecord.h"

struct bbsinfo bbsinfo;

int
report(str)
char *str;
{
	return 0;
	//      printf("%s\n",str);
}

unsigned long
atoul(p)
char *p;
{
	unsigned long s;
	char *t;
	t = p;
	s = 0;
	while ((*t >= '0') && (*t <= '9')) {
		s = s * 10 + *t - '0';
		t++;
	}
	return s;
}

void
showunprisonmessage(linebuf)
char *linebuf;
{
	char msgbuf[256];
	char repbuf[256];
	char uident[14];
	int i;
	strncpy(uident, linebuf, 12);
	uident[12] = 0;
	for (i = 0; i < 12; i++)
		if (uident[i] == ' ') {
			uident[i] = 0;
			break;
		}
	sprintf(repbuf, "%s 出狱", uident);
	sprintf(msgbuf, "出狱原因：刑满释放!");
	securityreport("deliver", msgbuf, repbuf);
	system_mail_buf(msgbuf, strlen(msgbuf), uident, repbuf, "deliver");
}

int
canunprison(linebuf, nowtime)
char *linebuf;
unsigned long nowtime;
{
	char *p;
	unsigned long time2;

	p = linebuf;
	while ((*p != 0) && (*p != 0x1b))
		p++;
	if (*p == 0)
		return 0;
	p++;
	if (*p == 0)
		return 0;
	p++;
	if (*p == 0)
		return 0;
	time2 = atoul(p);
	return nowtime > time2;
}

int
sgetline(buf, linebuf, idx, maxlen)
char *buf, *linebuf;
int *idx;
int maxlen;
{
	int len = 0;
	while (len < maxlen) {
		char ch;
		linebuf[len] = buf[*idx];
		ch = buf[*idx];
		(*idx)++;
		if (ch == 0x0d) {
			linebuf[len] = 0;
			if (buf[*idx] == 0x0a)
				(*idx)++;
			break;
		}
		if (ch == 0x0a) {
			linebuf[len] = 0;
			break;
		}
		if (ch == 0)
			break;
		len++;
	}
	return len;
}

int
main()
{
	int d_fd;
	struct boardheader bh;
	char denyfile[256];
	char *buf = NULL;
	int bufsize = 0;
	int size;
	struct stat st;
	time_t nowtime;
	int idx1, idx2;
	char linebuf[256];

	size = sizeof (bh);
	nowtime = time(NULL);

	if (initbbsinfo(&bbsinfo) < 0)
		return -1;
	sprintf(denyfile, "etc/prisonor");
	if (stat(denyfile, &st) == 0 && st.st_size != 0) {
		if (bufsize < st.st_size + 1) {
			if (buf)
				free(buf);
			buf = malloc(st.st_size + 1);
			buf[st.st_size] = 0;
			bufsize = st.st_size + 1;
		}
		if ((d_fd = open(denyfile, O_RDWR)) != -1) {
			flock(d_fd, LOCK_EX);
			if (read(d_fd, buf, st.st_size) == st.st_size) {
				idx1 = 0;
				idx2 = 0;
				while (idx2 < st.st_size) {
					int len =
					    sgetline(buf, linebuf, &idx2, 255);
					puts(linebuf);
					if (!canunprison(linebuf, nowtime)) {
						if (idx1 != 0) {
							buf[idx1] = 0x0a;
							idx1++;
						}
						memcpy(buf + idx1, linebuf,
						       len);
						idx1 += len;
					} else {
						showunprisonmessage(linebuf,
								  NULL, 0);
					}
				}
				buf[idx1] = 0x0a;
				idx1++;
				lseek(d_fd, 0, SEEK_SET);
				write(d_fd, buf, idx1);
				ftruncate(d_fd, idx1);
			}
			flock(d_fd, LOCK_UN);
			close(d_fd);
		}
	}

	if (buf)
		free(buf);
	return 0;
}
