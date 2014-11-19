/* convcode.c */
#ifndef __CONVCODE_H
#define __CONVCODE_H
int sgb2big(char *str0, int len);
int sbig2gb(char *str0, int len);
void conv_init(void);
char *gb2big(char *s, int *plen, int inst);
char *big2gb(char *s, int *plen, int inst);
#endif
