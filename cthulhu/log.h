#ifndef LOG_H
#define LOG_H

#include <netinet/in.h>
#include <stdio.h>

/*#define log_msg(x) write (1, x, sizeof (x) - 1)
#define log_err(x) write (2, x, sizeof (x) - 1)*/
#define log_err(a,b) xlog_err(a,b,sizeof(a)-1)
#define log_intern_err(x) perror (x)

extern void log_req __P ((domain_t *, char *, char *, struct sockaddr_in *, int));
/*extern void log_err __P ((char *, int));
extern void log_req __P ((char *, int));*/
extern void log_msg __P ((char *, int));
extern void xlog_err __P ((char *, char *, size_t));

enum { USER_AGENT = 1, IP_ADDRESS = 2 };

extern unsigned int loglevel;

#endif /* LOG_H */
