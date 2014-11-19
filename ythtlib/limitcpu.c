#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include "limitcpu.h"
static int load_limit = 200;	// it means the cpu limit is 1/200=0.5%
void
set_cpu_limit(int limit)
{
	if (limit > 5000)
		limit = 5000;
	else if (limit < 2)
		limit = 2;
	load_limit = limit;
}

int
limit_cpu(void)
{
	static time_t start = 0;
	time_t now;
	struct rusage u;
	static int sc, sc_u, t = 0;
	int cost, cost_u, d, ret = 0;
	if (!start) {
		start = time(0);
		getrusage(RUSAGE_SELF, &u);
		sc = u.ru_utime.tv_sec + u.ru_stime.tv_sec;
		sc_u = u.ru_utime.tv_usec + u.ru_stime.tv_usec;
		return ret;
	}
	if ((t++) % 20)
		return ret;
	now = time(0);
	getrusage(RUSAGE_SELF, &u);
	cost = u.ru_utime.tv_sec + u.ru_stime.tv_sec;
	cost_u = u.ru_utime.tv_usec + u.ru_stime.tv_usec;
	d =
	    load_limit * (cost - sc) + (cost_u -
					sc_u) / (1000000 / load_limit) - (now -
									  start);
	if (d > 10)
		d = 10;
	if (d > 0) {
		sleep(d);
		ret = 1;
	}
	if (now - start > 300) {
		start = now;
		sc = cost;
		sc_u = cost_u;
	}
	return ret;
}

void
get_load(load)
double load[];
{
#if defined(LINUX) || defined(CYGWIN)
	FILE *fp;
	fp = fopen("/proc/loadavg", "r");
	if (!fp)
		load[0] = load[1] = load[2] = 0;
	else {
		float av[3];
		fscanf(fp, "%g %g %g", av, av + 1, av + 2);
		fclose(fp);
		load[0] = av[0];
		load[1] = av[1];
		load[2] = av[2];
	}
#elif defined(BSD44)
	getloadavg(load, 3);
#else
	struct statstime rs;
	rstat("localhost", &rs);
	load[0] = rs.avenrun[0] / (double) (1 << 8);
	load[1] = rs.avenrun[1] / (double) (1 << 8);
	load[2] = rs.avenrun[2] / (double) (1 << 8);
#endif
}
