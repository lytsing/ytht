#include "config.h"
#include "xalloc.h"
#include "client.h"
#include "domain.h"
#include "pathconf.h"
#include "http.h"
#include "log.h"
#include "fastcgi.h"
#include "misc.h"
#include "auth.h"
#include "cgi.h"

#define extract_header(a,b) _extract_header (a,b,sizeof (b) - 1)

char *
_extract_header (cl, name, len)
	client_t *cl;
	char *name;
	size_t len;
{
	char *str = cl->buf;
	size_t i = 0;
	while (i<cl->ilen) {
		if (str[i]==*name && !memcmp (str+i,name,len))
			return (str+i+len);
		while (i<cl->len&&str[i++]!='\n');
	}
	return (NULL);
}

static char
c2e(x)
	char x;
{
	if ('A' <= x && x <= 'Z')
		return (x) - 'A';
	if ('a' <= x && x <= 'z')
		return (x) - 'a' + 26;
	if ('0' <= x && x <= '9')
		return (x) - '0' + 52;
	if (x == '+')
		return 62;
	if (x == '/')
		return 63;
	return -1;
}

char *
rdec (src, dst)
	unsigned char *src;
	char *dst;
{
	unsigned char *rdst = dst;
	if (*src <= ' ') return (NULL);
	while (*src > ' ') {
		int i = 0;
		for (i=0;src[i]>' '&&i<4;i++)
			dst[i] = c2e (src[i]);
		if (-1 == (dst[0] = ((dst[0] << 2) | (dst[1] >> 4)))) {
			dst[0] = 0;
			break;
		}
		if (-1 == (dst[1] = ((dst[1] << 4) | (dst[2] >> 2)))) {
			dst[1] = 0;
			break;
		}
		if (-1 == (dst[2] = ((dst[2] << 6) | (dst[3])))) {
			dst[2] = 0;
			break;
		}
		dst += i-1;
		if (i != 4) break;
		src += 4;
		*dst = 0;
	}
	return (rdst);
}

static int
read_auth_reply (cl)
	client_t *cl;
{
	char c = 1;
	ssize_t ret = read (cl->xfd, &c, sizeof (c));

	if (ret < 0) {
		if (EINTR == errno) return (1);
		return (send_err (cl, ERR500));
	}

	cl->hook = NULL;
	if (!c) return (schedule_pathconfs (cl));
	return (no_auth (cl, cl->pc->p1));
}

static int
do_auth (cl)
	client_t *cl;
{
	char *str = extract_header (cl, "Authorization: ");
	char *buffer;
	size_t len;

	cl->writing = 0;

	if (!cl->pc) return (send_err (cl, ERR500));

	if (!str) return (no_auth (cl, cl->pc->p1));

	for (len=0;str[len]>' ';len++);
	buffer = alloca (len+1 + sizeof (cl->url));
	buffer[0] = 0x10;
	memcpy (buffer+1, cl->url, cl->urlp);
	buffer[cl->urlp+1] = 0;
	if (!rdec (str+6, buffer+2+cl->urlp))
		return (send_err (cl, ERR400));
	len = strlen (buffer+2+cl->urlp);

	if (-1 == write (cl->xfd, buffer, cl->urlp+3+len)) {
		log_intern_err ("write to authentification daemon failed");
		return (send_err (cl, ERR500));
	}
	cl->hook = read_auth_reply;
	return (1);
}

extern int do_fcgi_auth ();

int
need_auth (cl, len)
	client_t *cl;
	size_t len;
{
	pathconf_t *pc = pc_generic_wildmat (PC_AUTH, cl->url+len, cl->domain);

	if ((cl->pc = pc) && !extract_header (cl, "Authorization: "))
		return (no_auth (cl, cl->pc->p1));

	if (!pc && !(cl->pc = pc_generic_wildmat (PC_FCGI_AUTH,
		cl->url+len, cl->domain))) return (0);

	if (!connect_to (cl, cl->pc)) {
		log_intern_err ("connect to authentification daemon failed");
		send_err (cl, ERR500);
		return (1);
	}
	cl->writing = 1;
	cl->xop |= 18;
	cl->hook = ((cl->pc->type == PC_AUTH) ? do_auth : do_fcgi_auth);
	return (1);
}
