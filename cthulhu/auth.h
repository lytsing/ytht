#ifndef AUTH_H
#define AUTH_H

extern int need_auth __P ((client_t *, size_t));
extern char *rdec __P ((unsigned char *, char *));
extern char *_extract_header __P ((client_t *, char *, size_t));

#endif /* AUTH_H */
