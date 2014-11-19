/*
    Pirate Bulletin Board System
    Copyright (C) 1999, KCN,Zhou Lin, kcn@cic.tsinghua.edu.cn
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "bbs.h"

#define	QLEN		5
#define	PID_FILE	"reclog/bbs.pid"
#define	LOG_FILE	"reclog/bbs.log"

#define LOAD_LIMIT
static int myports[] = { BBS_PORT, 3456, 8001 /*, 3001, 3002, 3003 */  };
static int big5ports[] = { BBS_BIG5_PORT /* , 3456, 3001, 3002, 3003 */  };
int big5 = 0;
int runtest = 0;

static int mport;

int max_load = 20;
int csock;			/* socket for Master and Child */

int checkaddr(struct in_addr addr, int csock);

void
cat(filename, msg)
char *filename, *msg;
{
	FILE *fp;

	if ((fp = fopen(filename, "a")) != NULL) {
		fputs(msg, fp);
		fclose(fp);
	}
}

void
prints(char *format, ...)
{
	va_list args;
	char buf[512];

	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	write(0, buf, strlen(buf));
}

//#ifndef CAN_EXEC
static void
telnet_init()
{
	static char svr[] = {
//              IAC, DO, TELOPT_TTYPE,
//              IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE,
		IAC, WILL, TELOPT_ECHO,
		IAC, WILL, TELOPT_SGA,
		IAC, WILL, TELOPT_BINARY,
		IAC, DO, TELOPT_NAWS,
		IAC, DO, TELOPT_BINARY
	};
	write(0, svr, sizeof (svr));
}

//#endif

static void
start_daemon(inetd, port)
int inetd;
int port;
{
	int n;
	struct rlimit rl;
	char buf[80], data[80];
	int portcount, big5portcount;
	time_t now;

	portcount = sizeof (myports) / sizeof (int);
	big5portcount = sizeof (big5ports) / sizeof (int);

	umask(007);

	rl.rlim_cur = 100 * 1024 * 1024;
	rl.rlim_max = 100 * 1024 * 1024;
	setrlimit(RLIMIT_CORE, &rl);
	rl.rlim_cur = 10 * 1024 * 1024;
	rl.rlim_max = 10 * 1024 * 1024;
	setrlimit(RLIMIT_DATA, &rl);

	close(1);
	close(2);

	now = time(0);

	if (inetd) {
		setgid(BBSGID);
		setuid(BBSUID);
		mport = port;

		big5 = port;
		sprintf(data, "%d\tinetd -i\n", getpid());
		cat(PID_FILE, data);

		return;
	}

	sprintf(buf, "bbsd start at %s", ctime(&now));
	cat(PID_FILE, buf);

	close(0);

	if (fork())
		exit(0);

	setsid();

	if (fork())
		exit(0);

	if (fork()) {
		setgid(BBSGID);
		setuid(BBSUID);
		exit(0);
	}

	if (port <= 0) {
		n = portcount + big5portcount - 1;
		while (n) {
			if (fork() == 0)
				break;
			sleep(1);
			n--;
		}
		if (n < portcount)
			port = myports[n];
		else {
			big5 = 1;
			port = big5ports[n - portcount];
		}
	}

	if (port == 3456)
		runtest = 1;
	else
		runtest = 0;

	if ((n = bindSocket(MY_BBS_IP, port, QLEN)) < 0) {
		errlog("Faild to bind & listen on port %d.", port);
		exit(1);
	}

	setgid(BBSGID);
	setuid(BBSUID);

	sprintf(data, "%d\t\t%d\n", getpid(), port);
	cat(PID_FILE, data);
}

static void
reaper()
{
	while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0) ;
}

static void
main_term()
{
	exit(0);
}

static void
main_signals()
{
	struct sigaction act;

	/* act.sa_mask = 0; *//* Thor.981105: ��׼�÷� */
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

#ifdef LINUX
	signal(SIGCHLD, SIG_IGN);
#else
	act.sa_handler = reaper;
	sigaction(SIGCHLD, &act, NULL);
#endif

	act.sa_handler = reaper;
	sigaction(SIGUSR1, &act, NULL);

	act.sa_handler = main_term;
	sigaction(SIGTERM, &act, NULL);

	act.sa_handler = SIG_IGN;
	sigaction(SIGUSR2, &act, NULL);
	sigaction(SIGPIPE, &act, NULL);
	/*sigblock(sigmask(SIGPIPE)); */
}

#ifdef LINUX
struct proc_entry {
	const char *path;
	const char *name;
	const char *fmt;
};

#define SENSOR_P "/proc/sys/dev/sensors/lm87-i2c-0-2e/"

struct proc_entry my_proc[5] = {
	{SENSOR_P "temp1", "CPU1�¶�", "%*s %*s %20s"},
	{SENSOR_P "temp2", "CPU2�¶�", "%*s %*s %20s"},
	{SENSOR_P "fan1", "FAN1ת��", "%*s %20s"},
	{SENSOR_P "fan2", "FAN2ת��", "%*s %20s"},
	{0, 0, 0}
};

void
show_proc_info(void)
{
	int i;
	FILE *fp;
	char buf[256];
	prints("\033[1;32m����״̬\033[36m");
	for (i = 0; my_proc[i].path; i++) {
		fp = fopen(my_proc[i].path, "r");
		if (NULL == fp)
			continue;
		if (1 == fscanf(fp, my_proc[i].fmt, buf))
			prints(" %s [\033[33m%s\033[36m]", my_proc[i].name,
			       buf);
		fclose(fp);
	}
	prints("\033[m\n\r");
}

void
show_bandwidth_info(void)
{
	FILE *fp;
	char buf[256];
	fp = fopen("bbstmpfs/dynamic/ubar.txt", "r");
	if (NULL == fp)
		return;
	prints("\033[1;32m�����������");
	if (fgets(buf, 256, fp)
	    && strlen(buf) > sizeof ("Current bandwidth utilization"))
		prints("%s", buf + sizeof ("Current bandwidth utilization"));
	fclose(fp);
	prints("\033[m\n\r");
}
#endif

int
bbs_main(hid)
char *hid;
{
	char buf[256];
	char bbs_prog_path[256];
	char bbstest_prog_path[256];
	FILE *fp;

/* load control for BBS */
#ifdef LOAD_LIMIT
	double cpu_load[3];
	int load;

	get_load(cpu_load);
	load = cpu_load[0];
	if (big5)
		prints
		    ("\033[1;36mBBS �̪� \033[33m(1,5,15)\033[36m �����������t�����O��\033[33m %.2f, %.2f, %.2f \033[36m(�ثe�W�� = %d).\033[0m\n\r\n\r",
		     cpu_load[0], cpu_load[1], cpu_load[2], max_load);
	else
		prints
		    ("\033[1;36mBBS ��� \033[33m(1,5,15)\033[36m ���ӵ�ƽ�����ɷֱ�Ϊ\033[33m %.2f, %.2f, %.2f \033[36m(Ŀǰ���� = %d).\033[0m\n\r",
		     cpu_load[0], cpu_load[1], cpu_load[2], max_load);

	if (load < 0 || load > max_load) {
		if (big5)
			prints("�ܩ�p,�ثe�t�έt���L��, �еy��A��\n\r");
		else
			prints("�ܱ�Ǹ,Ŀǰϵͳ���ɹ���, ���Ժ�����\n\r");
		close(csock);
		exit(-1);
	}
#endif				/* LOAD_LIMIT */

	if ((fp = fopen("NOLOGIN", "r")) != NULL
	    && (!runtest || access("CANTEST", F_OK))) {
		while (fgets(buf, 256, fp) != NULL)
			prints(buf);
		fclose(fp);
		close(csock);
		exit(-1);
	}

	sprintf(bbs_prog_path, "%s/bin/bbs", MY_BBS_HOME);
	sprintf(bbstest_prog_path, "%s/bin/bbstest", MY_BBS_HOME);

	if (checkbansite(hid)) {
		if (big5)
			prints("�����ثe���w��Ӧ� %s �X��!\r\n", hid);
		else
			prints("��վĿǰ����ӭ���� %s ����!\r\n", hid);
		shutdown(csock, 2);
		close(csock);
		exit(-1);
	}

	hid[16] = '\0';

	if (big5) {
		if (!runtest)
			execl(bbs_prog_path, "bbs", "e", hid, NULL);	/*����BBS */
		else
			execl(bbstest_prog_path, "bbstest", "e", hid, NULL);	/*����BBS */
	} else {
#ifdef LINUX
		show_proc_info();
		show_bandwidth_info();
		prints("\033[0m\n\r");
		//sleep(1);
#endif
		if (!runtest)
			execl(bbs_prog_path, "bbs", "d", hid, NULL);	/*����BBS */
		else {
#if 1
			execl(bbstest_prog_path, "bbstest", "d", hid, NULL);	/*����BBS */
#else
			execl("/usr/bin/valgrind", "valgrind",
			      "--log-file=" MY_BBS_HOME "/memlog.txt",
			      MY_BBS_HOME "/bin/bbstest", "d", hid, NULL);
			//execl("/usr/bin/memusage", "memusage", "-u", "-d",
			//      "bbstestmemusage.dat", MY_BBS_HOME "/bin/bbstest",
			//      "d", hid, NULL);
#endif
		}
	}
	write(0, "execl failed\r\n", 12);
	exit(-1);
}

int
mktmpdirs()
{
	char *(dirs[]) = {
	"bbstmpfs", "bbstmpfs/brc", "bbstmpfs/tmp",
		    "bbstmpfs/dynamic", "bbstmpfs/userattach",
		    "bbstmpfs/zmodem", "bbstmpfs/bbsattach",
		    "bbstmpfs/innd", NULL};
	char buf[80];
	int i;
	for (i = 0; dirs[i]; i++)
		mkdir(dirs[i], 0770);
	for (i = 'A'; i <= 'Z'; i++) {
		sprintf(buf, "bbstmpfs/brc/%c", i);
		mkdir(buf, 0770);
	}
	for (i = '0'; i <= '9'; i++) {
		sprintf(buf, "bbstmpfs/brc/%c", i);
		mkdir(buf, 0770);
	}
	return 0;
}

int
init_boardaux()
{
	struct stat st;
	if (!file_exist(BOARDAUX))
		close(open(BOARDAUX, O_CREAT | O_EXCL, 0660));
	if (stat(BOARDAUX, &st) != -1) {
		if (MAXBOARD * sizeof (struct boardaux) > st.st_size) {
			if (truncate
			    (BOARDAUX,
			     MAXBOARD * sizeof (struct boardaux)) < 0) {
				errlog("truncate " BOARDAUX " error %d", errno);
				exit(-1);
			}
		}
	}
	return 0;
}

int
main(argc, argv)
int argc;
char *argv[];
{
	int value;
	fd_set fds;
	struct sockaddr_in sin;
	char hid[17];

	chdir(MY_BBS_HOME);
	main_signals();
	start_daemon(argc > 2, atoi(argv[argc - 1]));
	mktmpdirs();

	init_boardaux();
//  main_signals();

	if (argc <= 2)
		for (;;) {
			FD_ZERO(&fds);
			FD_SET(0, &fds);
			if (select(1, &fds, NULL, NULL, NULL) < 0)
				continue;
/*
    value = 1;
    if (select(1, (fd_set *) & value, NULL, NULL, NULL) < 0)
      continue;
*/
			value = sizeof (sin);
			csock = accept(0, (struct sockaddr *) &sin, &value);
			if (csock < 0) {
				reaper();
				continue;
			}
//add by ylsdd, �Կ���վ��
			if (checkaddr(sin.sin_addr, csock) < 0) {
				close(csock);
				continue;
			}

			if (fork()) {
				close(csock);

#ifdef LOAD_LIMIT
				{
					double cpu_load[3];
					int load;
					get_load(cpu_load);
					load = cpu_load[0];
					if (load < 0 || load > max_load)
						sleep(5);	/* sleep for heavy load */
				}
#endif
				continue;
			}

			if (csock) {
				dup2(csock, 0);
				close(csock);
			}
			break;
	} else {
		int sinlen = sizeof (struct sockaddr_in);
		getpeername(0, (struct sockaddr *) &sin, (void *) &sinlen);
	}
#ifdef GETHOST
	whee =
	    gethostbyaddr((char *) &sin.sin_addr.s_addr,
			  sizeof (struct in_addr), AF_INET);
	if ((whee) && (whee->h_name[0])) {
		strncpy(hid, whee->h_name, 17);
		hid[16] = 0;
	} else
#endif
	{
		char *host = (char *) inet_ntoa(sin.sin_addr);
		strncpy(hid, host, 17);
		hid[16] = 0;
	}

//#ifndef CAN_EXEC
	telnet_init();
//#endif
	bbs_main(hid);
	return 0;
}

//checkaddr(...), add by ylsdd, �Կ���վ��
#define NADDRCHECK 500
struct {
	struct in_addr addr;
	time_t t;
	float x;
	int n;
} addrcheck[NADDRCHECK];

int
checkaddr(struct in_addr addr, int csock)
{
	int i, j;
	static int fd = -1, lll = 1;
	char str[150];
	time_t timenow, ttemp;

	if (fd < 0) {
		fd = open("/tmp/attacklog", O_CREAT | O_WRONLY | O_APPEND,
			  0664);
		if (fd >= 0)
			fcntl(fd, F_SETFD, 1);
	}
	if (lll == 1) {
		bzero(addrcheck, sizeof (addrcheck));
		lll = 0;
	}

	time(&timenow);
	for (i = 0; i < NADDRCHECK; i++) {
		if (addrcheck[i].t == 0)
			continue;
		if (timenow - addrcheck[i].t > 60 * 5
		    || timenow < addrcheck[i].t) {
			if (addrcheck[i].x > 100 && addrcheck[i].n > 7) {
				sprintf(str, "remove\t%s\t%d\t%s",
					inet_ntoa(addrcheck[i].addr),
					addrcheck[i].n, ctime(&timenow));
				write(fd, str, strlen(str));
			}
			addrcheck[i].t = 0;
			continue;
		}
		if (memcmp(&addrcheck[i].addr, &addr, sizeof (addr)) == 0) {
			if (addrcheck[i].x <= 100 || addrcheck[i].n <= 7) {
				j = 0;
				addrcheck[i].x = addrcheck[i].x / (((unsigned)

								    (timenow -
								     addrcheck
								     [i].t) +
								    15) / 20.) +
				    30;
			} else
				j = 1;
			addrcheck[i].n++;

			if (addrcheck[i].x > 100 && addrcheck[i].n > 7) {
				if (j == 0 && fd >= 0)
					if (fork() == 0) {
						sprintf(str, "add\t%s\t%d\t%s",
							inet_ntoa(addr),
							addrcheck[i].n,
							ctime(&timenow));
						write(fd, str, strlen(str));
						write(csock,
						      "�Բ���, ���ӽ����5���ӡ��벻Ҫ�������ӳ����վ\n",
						      strlen
						      ("�Բ���, ���ӽ����5���ӡ��벻Ҫ�������ӳ����վ\n"));
						sleep(5);
						exit(0);
					}
				return -1;
			}
			addrcheck[i].t = timenow;
			return 0;
		}
	}
	//�����addrcheck��û�иõ�ַ, �����.
	for (i = 0, j = -1, ttemp = timenow + 1; i < NADDRCHECK; i++) {
		if (addrcheck[i].t < ttemp
		    && (addrcheck[i].x <= 100 || addrcheck[i].n <= 7)) {
			ttemp = addrcheck[i].t;
			j = i;
		}
	}
	if (j == -1)
		for (i = 0, j = 0, ttemp = timenow + 1; i < NADDRCHECK; i++) {
			if (addrcheck[i].t < ttemp) {
				ttemp = addrcheck[i].t;
				j = i;
			}
		}
	if (addrcheck[j].x > 100 && addrcheck[j].n > 7) {
		sprintf(str, "remove\t%s\t%d\t%s", inet_ntoa(addrcheck[i].addr),
			addrcheck[j].n, ctime(&timenow));
		write(fd, str, strlen(str));
	}
	addrcheck[j].addr = addr;
	addrcheck[j].t = timenow;
	addrcheck[j].x = 0;
	addrcheck[j].n = 1;
	return 0;
}
