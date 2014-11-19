/* base64.h */
#ifndef __BASE64_H
#define __BASE64_H
#include <stdio.h>
int f_b64_ntop_init(unsigned char *, int *, int *);
int f_b64_ntop(FILE *out, unsigned char *data, int len, unsigned char *tail_buf, int *tail_len, int *outlen);
int f_b64_ntop_fini(FILE *out, unsigned char *, int *);
#endif
