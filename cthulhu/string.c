#include <sys/types.h>
#include <string.h>
#include <time.h>
#include "string.h"
#include "xalloc.h"

void
expand_string (string, xlen)
	string_t *string;
	size_t xlen;
{
	size_t len = (xlen>STRSZ)?xlen:STRSZ;
	string->str = xresize (string->str, string->len += len);
}

void
fix_string (string)
	string_t *string;
{
	string->str = xresize (string->str, string->off);
}

int
create_string (string)
	string_t *string;
{
	string->str = xalloc ((string->len = STRSZ));
	string->off = 0;
	return (1);
}

void
delete_string (string)
	string_t *string;
{
	xfree (string->str, string->off);
}

void
reset_string (string)
	string_t *string;
{
	string->off = 0;
}

void
as_puts (string, str, len)
	string_t *string;
	const char *str;
	size_t len;
{
	if ((string->off + len) > string->len) expand_string (string, len);
	if (len < 4) {
		size_t l = len;
		while (l--) string->str[string->off+l] = str[l];
	} else
	  memcpy (string->str+string->off, str, len);
	string->off += len;
}

void
as_putline (string, str)
	string_t *string;
	char *str;
{
	size_t len = 0;
	while (str[len++]!='\n'); len++;
	if ((string->off + len) > string->len) expand_string (string, len);
	memcpy (string->str+string->off, str, len-2);
	string->off += len-2;
	string->str[string->off++] = '\r';
	string->str[string->off++] = '\n';
}

#define SIZEOFLONGSTR sizeof (long) * 4

void
as_putlong (string, l)
	string_t *string;
	long l;
{
	size_t off = SIZEOFLONGSTR;
	char buf[SIZEOFLONGSTR], *s = buf;
	do { s[--off] = l%10+'0'; } while (off && (l/=10));
	as_puts (string, buf+off, SIZEOFLONGSTR-off);
}

#define is_num(x) ((x)<='9'&&(x)>='0')

int
scanstr (str)
	char *str;
{
	int res = 0;
	if (!is_num (*str)) return (-1);
	while (is_num (*str))
		res = res*10+*str++-'0';
	return (res);
}

static int
pscanstr (pstr)
	char **pstr;
{
	int res = 0;
	char *str = *pstr;
	while (*str && *str <= ' ') str++;
	if (!is_num (*str)) return (-1);
	while (is_num (*str))
		res = res*10+*str++-'0';
	*pstr = str;
	return (res);
}

time_t
parse_time_date (str)
	char *str;
{
	time_t res = 0;
	int i, tmp, st = 0, year;
	extern char *smonths[];
	extern int months[];
	char *month;
	for (i=0;i<27;i++) if (str[i]=='\n') return (-1);
	if (str[3]==',') str += 5; else {
		while (*str > ' ') str++;
		if (',' != str[-1]) {
			st = 2;
			month = ++str;
			while (*str++ > ' ');
		} else st = 1;
	}
	if (-1 == (tmp = pscanstr (&str))) return (-1);
	res = tmp*86400;
	if (st < 2) {
		month = ++str;
		str += (st==1?4:4);
	}
	if (st != 2)
	 if (-1 == (year = pscanstr (&str))) return (-1);
	if (-1 == (tmp = pscanstr (&str))) return (-1);
	res += tmp*3600;
	str++;
	if (-1 == (tmp = pscanstr (&str))) return (-1);
	res += tmp*60;
	str++;
	if (-1 == (tmp = pscanstr (&str))) return (-1);
	res += tmp;
	if (st == 2)
	 if (-1 == (year = pscanstr (&str))) return (-1);
	if (year > 1970) year -= 1970; else year -= 70;
	i = year/4-1;
	res += year*86400*365+i*86400;
	for (i=0,tmp=0;i<11;i++) {
		int x;
		for (x=0;x<3;x++) if (month[x]!=smonths[i][x]) break;
		if (x == 3) break;
		tmp += months[i];
	}
	res += tmp*86400;
	return (res);
}

unsigned int
snlen (str)
	char *str;
{
	unsigned int res = 0;
	while (str[res]!='\r'&&str[res]!='\n') res++;
	res += str[res]=='\n'?1:2;
	return (res);
}
