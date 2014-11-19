#include "client.h"
#include "config.h"
#include "http.h"
#include "quota.h"

extern client_t *clients;

int
may_post (cl)
	client_t *cl;
{
#if MAX_POST_REQUESTS_PER_HOST
	client_t *c = clients;
	int cnt = 0;

	while (c) {
		if (c->rt == POST &&
			c->saddr.sin_addr.s_addr == cl->saddr.sin_addr.s_addr)
			  if (cnt++ >= MAX_POST_REQUESTS_PER_HOST)
				return (0);
		c = c->next;
	}
#endif
	return (1);
}
