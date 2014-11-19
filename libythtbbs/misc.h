/* misc.c */
#ifndef __MISC_H
#define __MISC_H
void getrandomint(unsigned int *s);
void getrandomstr(unsigned char *s);
struct mymsgbuf {
	long int mtype;
	char mtext[1];
};
void newtrace(char *s);
int init_newtracelogmsq();
void tracelog(char *fmt, ...) __attribute__((format(printf, 1, 2)));
int deltree(const char *dst);
#endif
