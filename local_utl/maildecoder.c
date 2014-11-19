//从 mail2bbs.c 里面抄了一堆；没搞清 mail2bbs.c 怎么用吧...
#include "ythtlib.h"
int str_decode(register unsigned char *dst, register unsigned char *src);
#define BUFLEN 10240

char *
nextLine(char *buf)
{
	return fgets(buf, BUFLEN, stdin);
}

void
chop(char *buf)
{
	char *ptr = strchr(buf, '\r');
	if (ptr)
		*ptr = 0;
	ptr = strchr(buf, '\n');
	if (ptr)
		*ptr = 0;
}

int
main()
{
	char buf[BUFLEN], sbuf[BUFLEN + 10], dbuf[BUFLEN];
	char boundary[1024] = "--";
	char *ptr;
	int blen = 0, len;
	int isBase64 = 0, isQuoted = 0, wc;
	int hasr = 0;
	while (nextLine(buf)) {
		chop(buf);
		if (!*buf)
			break;
		if(!strncasecmp(buf, "To: ", 3)) {
			//errlog("%s", buf);
			ptr = strchr(buf, '@');
			if(!ptr)
				continue;
			*ptr = 0;
			ptr = strchr(buf, '.');
			if(!ptr)
				continue;
			*ptr = 0;
			printf("%s\n", buf);
			continue;
		}
		//looking for boundary...
		if ((ptr = strcasestr(buf, "boundary="))) {
			ptr += strlen("boundary=");
			if(*ptr=='"')
				ptr++;
			strsncpy(boundary + 2, ptr, sizeof (boundary) - 2);
			if((ptr=strrchr(boundary, '"')))
				*ptr = 0;
			//printf("-------------%s-----------", boundary);
			break;
		}
	}
	if (*boundary) {
		blen = strlen(boundary);
		while (nextLine(buf)) {
			chop(buf);
			if (!strcmp(boundary, buf)) {
				//printf("----------");
				break;
			}
		}
		len = strlen("Content-Transfer-Encoding: ");
		while (nextLine(buf)) {
			chop(buf);
			if (!*buf)
				break;
			if (!strncasecmp
			    (buf, "Content-Transfer-Encoding: ", len)) {
				if (!strncasecmp(buf + len, "Base64", 6))
					isBase64 = 1;
				if (!strncasecmp
				    (buf + len, "quoted-printable", 16))
					isQuoted = 1;
			}
		}
		strcat(boundary, "--");
		blen += 2;
		if (isBase64 || isQuoted) {
			if (isQuoted)
				strcpy(sbuf, "=??q?");
			else
				strcpy(sbuf, "=??b?");

			while (nextLine(buf)) {
				if (!strncmp(boundary, buf, blen))
					break;
				strcpy(sbuf + 5, buf);
				chop(sbuf);
				if (isQuoted) {
					if (sbuf[strlen(sbuf) - 1] == '=')
						sbuf[strlen(sbuf) - 1] = 0;
					else
						hasr = 1;
				}
				strcat(sbuf, "?=");
				wc = str_decode(dbuf, sbuf);
				if (wc >= 0) {
					fwrite(dbuf, wc, 1, stdout);
					if (hasr) {
						fputc('\n', stdout);
						hasr = 0;
					}
				} else {
					fputs(buf, stdout);
				}
			}
		} else {
			while (nextLine(buf)) {
				if (!strncmp(boundary, buf, blen))
					break;
				fputs(buf, stdout);
			}
		}
	} else {
		while (fgets(buf, sizeof (buf), stdin)) {
			fputs(buf, stdout);
		}
	}
	return 0;
}
