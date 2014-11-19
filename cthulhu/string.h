#ifndef STRING_H
#define STRING_H

#include <sys/types.h>
#include <string.h>
#include "config.h"

typedef struct {
	char  *str;
	size_t off;
	size_t len;
} string_t;

#define STRSZ 0x8
/*#define as_putc(str, x) if ((str).off != (str).len) (str).str[(str).off++] = x*/
#define as_putc(st,x) { if (st.off >= st.len) expand_string (&st, STRSZ); st.str[st.off++] = x; }

#define CHECK_BUF(x,y) if ((x->off + y) > x->len) expand_string (x,y)

extern void expand_string __P ((string_t *, size_t));
extern void fix_string __P ((string_t *));
extern int create_string __P ((string_t *));
extern void delete_string __P ((string_t *));
extern void reset_string __P ((string_t *));
extern void as_puts __P ((string_t *, const char *, size_t));
extern void as_putline __P ((string_t *, char *));
extern void as_putlong __P ((string_t *, long));

extern time_t parse_time_date __P ((char *));
extern int scanstr __P ((char *));
extern unsigned int snlen __P ((char *));

#endif /* STRING_H */
