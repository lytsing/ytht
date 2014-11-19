#ifndef CGI_H
#define CGI_H

#include "domain.h"
#include "client.h"
#include "pathconf.h"

extern int connect_to __P ((client_t *, pathconf_t *));
extern int exec_cgi __P ((client_t *, size_t));

#endif /* CGI_H */
