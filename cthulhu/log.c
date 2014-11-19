#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include "domain.h"
#include "log.h"
#include "misc.h"

unsigned int loglevel = 3;
static char logbuf[0x100], *s = logbuf;
static domain_t *dom;

extern time_t curtime;
extern unsigned int months[12],lmonths[12];

static void
log_flush ()
{
	write (dom->logfd, logbuf, s-logbuf);
	s = logbuf;
}

#define CHECK_BOUNDS(x) if (!check_bounds(x)) return
#define log_putc(x) (*s++ = x)

static int
check_bounds (len)
	size_t len;
{
	if ((s-logbuf+len) > sizeof (logbuf))
		log_flush ();
	return (len <= sizeof (logbuf));
}

static inline void
xlog_putc (c)
	char c;
{
	if ((s-logbuf+1) >= sizeof (logbuf)) log_flush ();
	*s++ = c;
}

static void
log_puts (str, len)
	char *str;
	size_t len;
{
	if (len > sizeof (logbuf)) {
		log_flush ();
		write (dom->logfd, str, len);
		return;
	}
	if ((s-logbuf+len) > sizeof (logbuf))
		log_flush ();
	memcpy (s, str, len);
	s += len;
}

static void
log_putipaddr (addr)
	unsigned int addr;
{
	unsigned char *ipaddr = (unsigned char *) &addr;
	int i;
	CHECK_BOUNDS(15);

	for (i=0;i<4;i++) {
		unsigned char cad = ipaddr[i], f;
		if ((f=(cad>=100))) { log_putc (cad/100+'0'); cad %= 100; }
		if (f||cad>=10) log_putc (cad/10+'0');
		log_putc (cad%10+'0');
		if (i != 3) log_putc ('.');
	}
}

static void
log_putline (str)
	char *str;
{
	size_t s1 = 0;
	while (str[s1] != '\r' && str[s1] != '\n') s1++;
	log_puts (str, s1);
}

static void
log_putlong (l)
	unsigned long l;
{
	size_t i = sizeof (long) * 4, len = i;
	CHECK_BOUNDS(sizeof (long) * 4);
	do s[--i] = l%10+'0'; while (i && (l/=10));
	memmove (s, s+i, len-i);
	s += len-i;
}

static void
log_puthdr (str, hdr, len, slen)
	char *str, *hdr;
	size_t slen, len;
{
	size_t i;

	for (i=0;i<len;) {
	  if (str[i] == *hdr && !memcmp (str+i, hdr, slen)) {
		log_putline (str+i+slen);
		return;
	  }
	  while (i<len&&str[i++]!='\n');
	}
}

#define put2(x)  { log_putc (x/10+'0'); log_putc (x%10+'0'); }
#define put2x(x) { put2(x); log_putc ('/'); }
#define put2y(x) { put2(x); log_putc (':'); }

/* 01/13/03 15:28:08 */
static void
log_putdate (t)
	time_t t;
{
	unsigned int year = (t / ((365*24*60*60) + 21600))/* - 30*/;
	unsigned int month, day, hours, mins, secs, *xmonths = (((year+2)%4)?months:lmonths);
	int x;
	CHECK_BOUNDS (36);
	t %= (year * 365*24*60*60 + (year/4*(24*60*60)));
	year -= 30;
	x = t / (24*60*60);
	for (month=0;;month++) if ((x -= xmonths[month])<0) break;
	day = xmonths[month++] + x + 1;
	x = t % (24*60*60);
	hours = x / (60*60);
	x %= 60*60;
	mins = x / 60;
	secs = x % 60;
	put2x (month);
	put2x (day);
	put2 (year); log_putc (' ');
	put2y (hours);
	put2y (mins);
	put2 (secs); log_putc (' ');
}

/* loglevel:
  0: log nothing
  1: log only request-type and url
  2: log request type, url, user-agent
  3: log request type, url, user-agent, ip-address:port
  4: log all := whole request
  default: 3
*/
/*#define xputs(x) { memcpy (s, x, sizeof (x) - 1); s += sizeof (x) - 1; }
#define putln(x) while (*x!='\n') *s++=*x++;*/
void
log_req (domain, str, ret, inaddr, len)
	domain_t *domain;
	char *str, *ret;
	struct sockaddr_in *inaddr;
	int  len;
{
	if (!loglevel || domain->logfd == -1) return;
	dom = domain;
	log_putdate (curtime);
	log_putipaddr (inaddr->sin_addr.s_addr);
	if (loglevel >= 3) { xlog_putc (':'); log_putlong (inaddr->sin_port); }
	xlog_putc ('\t');
	log_puts (ret, 4);
	if (loglevel == 4) { log_puts (str, len); return; }
	log_putline (str);
	if (loglevel > 1) {
		xlog_putc ('\t');
		log_puthdr (str, "User-Agent: ", len, 12);
	}
	if ((domain == &default_domain) && (loglevel > 1)) {
		xlog_putc ('\t');
		log_puthdr (str, "Host: ", len, 6);
	}
	xlog_putc ('\n');
	log_flush ();
}

void
xlog_err (str1, str2, s1)
	char *str1, *str2;
	size_t s1;
{
	size_t s2 = strlen (str2);
	char *buf = alloca (s1+s2+8);
	memcpy (buf, str1, s1);
	buf[s1] = '"';
	memcpy (buf+s1+1, str2, s2);
	buf[s1+s2+1] = '"';
	buf[s1+s2+2] = '\n';
	write (2, buf, s1+s2+3);
}
