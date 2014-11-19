#include "bbs.h"

int
main()
{
	int lockfd;
	chdir(MY_BBS_HOME);
	lockfd = openlockfile(".lock_new_register", O_RDONLY, LOCK_EX);
	if (lockfd < 0)
		return 2;
	if (!file_isfile("new_register.tmp"))
		return 1;
	return
	    system
	    ("cat new_register.tmp >> new_register;rm -f new_register.tmp");
}
