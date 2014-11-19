#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <errno.h>
#include <fcntl.h>
#include <epoll.h>

#include "config.h"
#include "client.h"
#include "xalloc.h"
#include "log.h"
#include "http.h"
#include "misc.h"
#include "socket.h"


#define MAX_EPOLL_CONNECTION 65536

#define err_msg(x) write (2, x, sizeof (x) - 1)

static int ep_fd=-1;
static struct epoll_event *ep_events;
static int ep_readyfds;
static client_t * fd2cl[MAX_EPOLL_CONNECTION*2+100];

static void
epoll_add_fd( int fd, int rw )
{
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = rw;
	if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, fd, &ev) < 0)
	{	
		err_msg ("epoll_add_fd: can't add epoll fd. Exiting...\n");
		exit (25);
	}
}

static void
epoll_del_fd( int fd )
{
    struct epoll_event ev;
    if (epoll_ctl(ep_fd, EPOLL_CTL_DEL, fd, &ev) < 0 && EBADF!= errno)
    {
	err_msg ("epoll_del_fd: can't del epoll fd. Exiting...\n");
	exit (25);
    }
}

static void
epoll_init( int sockfd)
{
	ep_readyfds = 0;
	ep_fd = epoll_create(MAX_EPOLL_CONNECTION + 1);
	ep_events = (struct epoll_event*) xalloc( sizeof(struct epoll_event) * MAX_EPOLL_CONNECTION );
	if ( ep_fd < 0 ){
		err_msg ("epoll_init: can't create epoll fd. Exiting...\n");
		exit (24);
	}
	epoll_add_fd(sockfd,EPOLLIN);
}

int
select_loop (int sockfd)
{
	int old_writing,newfd,io,changeio;
	client_t *cl;
	int i ;

	if(ep_fd<0)
		epoll_init(sockfd);
	if (killconns) { do_kill_conns (); return (0); }
	ep_readyfds = epoll_wait (ep_fd, ep_events, MAX_EPOLL_CONNECTION, -1);
	curtime = time (NULL);
	for (i=0; i< ep_readyfds; i++) {
		if( ep_events[i].data.fd == sockfd){
			if ( ep_events[i].events & EPOLLIN){
				if((cl=add_client (sockfd))){
					if(filenum>MAX_EPOLL_CONNECTION)
						remove_client(cl);
					else{
						epoll_add_fd(cl->sockfd, EPOLLIN);
						fd2cl[cl->sockfd]=cl;
					}
				}
			}
			else if (ep_events[i].events & EPOLLERR) {
				log_intern_err ("socket descriptor is invalid");
				exit (23);
			}
			continue;
		}
		cl=fd2cl[ep_events[i].data.fd];
		cl->lastop = curtime;
		old_writing=cl->writing;
		changeio=0;
		if ((!cl->hook && !handle_socket (cl)) ||
			(cl->hook && !cl->hook (cl))){
			remove_client (cl);
			continue;
		}
		if(cl->writing!=old_writing){
			io=cl->writing?EPOLLOUT:EPOLLIN;
			changeio=1;
		}
		newfd= (cl->xop&2)?cl->xfd:cl->sockfd;
		if(newfd!=ep_events[i].data.fd||changeio){
			epoll_del_fd(ep_events[i].data.fd);
			epoll_add_fd(newfd,io);
			fd2cl[newfd]=cl;
		}
	}
	return 0;
}
