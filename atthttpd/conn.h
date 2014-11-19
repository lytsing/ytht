#ifndef __CONN_H
#define __CONN_H

#include <stdio.h>

#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>

#include "bbs.h"
extern struct bbsinfo bbsinfo;
extern struct BCACHE *shm_bcache;
int	conn_insert (int);
void	conn_upkeep (fd_set *, fd_set *);
int	conn_cullselect (fd_set *, fd_set *);

#endif

