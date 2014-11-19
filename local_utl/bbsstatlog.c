#include "bbs.h"
#include "bbsstatlog.h"

struct bbsstatlogitem item;
struct bbsinfo info;

void
add_uindex(int uid, int utmpent)
{
	int i, uent;
	if (uid <= 0 || uid > MAXUSERS)
		return;
	for (i = 0; i < 6; i++)
		if (info.uindexshm->user[uid - 1][i] == utmpent)
			return;
	for (i = 0; i < 6; i++) {
		uent = info.uindexshm->user[uid - 1][i];
		if (uent <= 0 || !info.utmpshm->uinfo[uent - 1].active ||
		    info.utmpshm->uinfo[uent - 1].uid != uid) {
			info.uindexshm->user[uid - 1][i] = utmpent;
			return;
		}
	}
}

void
rmdup_uindex(int uid)
{
	int i, j, uent;
	for (i = 0; i < 5; i++) {
		uent = info.uindexshm->user[uid - 1][i];
		if (uent <= 0)
			continue;
		for (j = i + 1; j < 6; j++) {
			if (uent == info.uindexshm->user[uid - 1][j])
				info.uindexshm->user[uid - 1][j] = 0;
		}
	}
}

void
syn_uindxexshm()
{
	int i;
	for (i = 0; i < USHM_SIZE; i++) {
		if (info.utmpshm->uinfo[i].active) {
			add_uindex(info.utmpshm->uinfo[i].uid, i + 1);
		}
	}
	for (i = 0; i < MAXUSERS; i++)
		rmdup_uindex(i + 1);
}

void
bonlinesync()
{
	int i, numboards;
	struct user_info *uentp;
	numboards = info.bcacheshm->number;
	for (i = 0; i < numboards; i++)
		info.bcacheshm->bcache[i].inboard = 0;
	for (i = 0; i < USHM_SIZE; i++) {
		uentp = &(info.utmpshm->uinfo[i]);
		if (uentp->active && uentp->pid && uentp->curboard)
			info.bcacheshm->bcache[uentp->curboard - 1].inboard++;
	}
}

void
get_load_float(load)
float load[];
{
	FILE *fp;
	fp = fopen("/proc/loadavg", "r");
	if (!fp)
		load[0] = load[1] = load[2] = 0;
	else {
		float av[3];
		fscanf(fp, "%f %f %f", av, av + 1, av + 2);
		fclose(fp);
		load[0] = av[0];
		load[1] = av[1];
		load[2] = av[2];
	}
}

int
_get_netflow(char *fname)
{
	FILE *fp;
	double fMbit_s = 0;
	char *ptr, buf[256];
	fp = fopen(fname, "r");
	if (!fp)
		return 0;
	if (!fgets(buf, sizeof (buf), fp))
		goto ERR;
	ptr = strchr(buf, '.');
	if (!ptr)
		goto ERR;
	while (ptr > buf && isdigit(*(ptr - 1)))
		ptr--;
	fMbit_s = atof(ptr);
      ERR:
	fclose(fp);
	return fMbit_s * 1024;
}

int
get_netflow()
{
	int netflow;
	netflow = _get_netflow(MY_BBS_HOME "/bbstmpfs/dynamic/ubar.txt");
	netflow += _get_netflow(MY_BBS_HOME "/bbstmpfs/dynamic/ubar_in.txt");
	return netflow;
}

int
main()
{
	int i, fd;
	struct user_info *p;
	struct tm *ptm;

	if (initbbsinfo(&info) < 0)
		return 0;
	item.time = time(NULL);
	get_load_float(item.load);
	item.netflow = get_netflow();
	item.naccount = usersum();
	for (i = 0; i < USHM_SIZE; i++) {
		p = &info.utmpshm->uinfo[i];
		if (!p->active)
			continue;
		item.nonline++;
		if (p->pid == 1 && strcmp(p->userid, "guest"))
			item.nwww++;
		else if (p->pid == 1)
			item.nwwwguest++;
		else
			item.ntelnet++;
		if (!strncmp("162.105", p->from, 7)) {
			item.n162105++;
			if (p->pid != 1)
				item.n162105telnet++;
		}
	}
	ptm = localtime(&item.time);
	if (ptm->tm_hour == 0 && ptm->tm_min < 6)
		info.utmpshm->maxtoday = item.nonline;
	fd = open(BBSSTATELOGFILE, O_WRONLY | O_CREAT, 0600);
	if (fd >= 0) {
		lseek(fd,
		      ((ptm->tm_mday * 24 + ptm->tm_hour) * 10 +
		       ptm->tm_min / 6) * sizeof (item), SEEK_SET);
		write(fd, &item, sizeof (item));
		close(fd);
	}
//	syn_uindxexshm();
	bonlinesync();
	return 0;
}
