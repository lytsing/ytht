#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct rule {
	char *name;
	char *value;
	struct rule *next;
} head = {
NULL, NULL, NULL};

void
appendRule(struct rule *r)
{
	struct rule *p;
	r->next = NULL;
	p = &head;
	while (p->next)
		p = p->next;
	p->next = r;
}

void
allocRule(char *name, char *value)
{
	struct rule *r = calloc(1, sizeof (struct rule));
	r->name = strdup(name);
	r->value = strdup(value);
	appendRule(r);
}

char *
strtrim(char *s)
{
	char *ptr = s + strlen(s) - 1;
	while (ptr >= s && strchr("\r\n \t", *ptr)) {
		*ptr = 0;
		ptr--;
	}
	ptr = s;
	while (*ptr && strchr("\r\n \t", *ptr)) {
		ptr++;
	}
	return ptr;
}

void
parseRule(char *line)
{
	char *ptr;
	if (line[0] == '#')
		return;
	ptr = strchr(line, '=');
	if (!ptr)
		return;
	*ptr = 0;
	allocRule(strtrim(line), strtrim(ptr + 1));
}

void
parseRules(char *filename)
{
	char buf[1024];
	FILE *fp = fopen(filename, "r");
	if (!fp)
		return;
	while (fgets(buf, sizeof (buf), fp)) {
		parseRule(buf);
	}
	fclose(fp);
}

void
applyRule(char *buf, struct rule *r)
{
	char buf1[10240] = "";
	char *p0 = buf, *ptr;
	while (*p0) {
		ptr = p0;
	      AGAIN:
		ptr = strstr(ptr, r->name);
		if (!ptr) {
			strcat(buf1, p0);
			break;
		}
		if (isalnum(ptr[strlen(r->name)])
		    || ptr[strlen(r->name)] == '_') {
			ptr++;
			goto AGAIN;
		}
		*ptr = 0;
		strcat(buf1, p0);
		strcat(buf1, r->value);
		p0 = ptr + strlen(r->name);
	}
	strcpy(buf, buf1);
}

void
applyRules(char *buf)
{
	struct rule *r = head.next;
	while (r) {
		applyRule(buf, r);
		//printf("=%s", buf);
		r = r->next;
	}
}

int
main(int argn, char **argv)
{
	char buf[102400];
	parseRules(argv[1]);
	while (fgets(buf, 1024, stdin)) {
		applyRules(buf);
		printf("%s", buf);
	}
	return 0;
}
