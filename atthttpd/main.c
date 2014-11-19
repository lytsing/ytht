#include <stdio.h>

#include <unistd.h>

#include <sys/time.h>
#include <sys/resource.h>
#include "config.h"
#include "conn.h"
#include "sv_core.h"

int
main(void)
{
	struct rlimit rl;	
	rl.rlim_cur=40*1024*1024; 
	rl.rlim_max=80*1024*1024; 
	setrlimit(RLIMIT_CORE,&rl); 
	nice(1);
	if(initbbsinfo(&bbsinfo)<0)
		exit(1);
	shm_bcache = bbsinfo.bcacheshm;
#if 0
	sv_core_httpd();
#endif
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
	if (fork() == 0) {
		setsid();
		sv_core_httpd();
	}
	return 0;
}
