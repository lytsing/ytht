#include "misc.h"

#define DO_CONT if ('*' == *pattern || '?' == *pattern || '[' == *pattern) continue

/*XXX: ** -> a* */
/* text may not contain *, ? or other evil characters */
int
wildmat (text, pattern)
	char *text;
	char *pattern;
{
	while (*text && *pattern) {
	 if ('*' == *pattern) {
		char *op;
		/* user might be stupid. */
		while ('*' == *pattern || '?' == *pattern) pattern++;
		while (*text && *text != *pattern) text++;
		if (!*text && !*pattern) return (1);
		op = pattern-1;
		while (*text && *pattern == *text) text++, pattern++;
		DO_CONT;
		pattern = op;
	 } else if ('?' == *pattern) {
		if (!*text++) return (!*++pattern);
		pattern++;
	 } else if ('[' == *pattern) {
		int not;
		pattern++;
		if ((not = '^' == *pattern)) pattern++;
		while (*pattern && *pattern != ']') { /*XXX*/
		  if ('-' == *pattern)
			if (*text >= pattern[-1] && *text <= pattern[1]) break;
		  if (*pattern == *text) break;
		  pattern++;
		}
		if ((*pattern == ']') != not) return (0);
		while (*pattern && *pattern++ != ']');
		text++;
	 }
	 while (*text && *text == *pattern) text++, pattern++;
	 DO_CONT;
	 if (*text) return (0);
	}
	if (!*text && *pattern) {
		if (!pattern[1] && (pattern[0] == '*' || pattern[0] == '?'))
			return (1);
		return (0);
	}
	return (1);
}
