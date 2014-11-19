#ifndef HTTP_H
#define HTTP_H

#include "misc.h"

extern time_t curtime;

enum { GET = 1, POST, HEAD };

extern int handle_post_request __P ((client_t *));
extern int parse_http_request __P ((client_t *));
/*extern int parse_http_request __P ((client_t *, char *, size_t));*/
extern int send_answer __P ((client_t *, entry_t *));
extern int send_err __P ((client_t *, entry_t *));
extern int no_auth __P ((client_t *, char *));
extern int schedule_pathconfs __P ((client_t *));

#define HTTP_VER "HTTP/1.0"
#define http_close_file(file) if (!xclose_file (file)) xfree (file, sizeof (file_t));

extern entry_t err_entries[];

#define OK200 &err_entries[0]
#define OK204 &err_entries[1]
#define OK206 &err_entries[2]
#define ERR301 &err_entries[3]
#define ERR304 &err_entries[4]
#define ERR400 &err_entries[5]
#define ERR401 &err_entries[6]
#define ERR403 &err_entries[7]
#define ERR404 &err_entries[8]
#define ERR405 &err_entries[9]
#define ERR406 &err_entries[10]
#define ERR500 &err_entries[11]
#define ERR503 &err_entries[12]

#endif
