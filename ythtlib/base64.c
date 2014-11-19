#include "base64.h"
#include <string.h>
#include <stdlib.h>
static const char Base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

int f_b64_ntop_init(unsigned char *tailbuf, int *taillen, int *outlen)
{
	bzero(tailbuf, 3);
	*taillen = 0;
	*outlen = 0;
	return 0;
}

int f_b64_ntop_fini(FILE *out, unsigned char *tailbuf, int *taillen)
{
	unsigned char input[3];
	unsigned char output[4];
	int i;
	unsigned char *src = tailbuf;

	if (0 != *taillen) {
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < *taillen; i++)
			input[i] = *src++;
	
		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		
		fprintf(out, "%c%c", Base64[output[0]], Base64[output[1]]);
		if (*taillen == 1)
			fprintf(out, "%c", Pad64);
		else
			fprintf(out, "%c", Base64[output[2]]);
		fprintf(out, "%c", Pad64);
	}
	bzero(tailbuf, 3);
	*taillen = 0;
	return 0;
}

int
f_b64_ntop(FILE *out, unsigned char *data, int len, unsigned char *tailbuf, int *taillen, int *outlen)
{
	unsigned char input[3];
	unsigned char output[4];
	int i;
	unsigned char *src;
	int srclength = len;

	src = data;
	if (*taillen > 0) {
		if (len + *taillen < 3) {
			for (i = 0; i < len; i++) {
				tailbuf[*taillen] = data[i];
				(*taillen)++;
			}
			return 0;
		}
		srclength -= (3 - *taillen);
		for (i = 0; i < *taillen; i++)
			input[i] = tailbuf[i];
		*taillen = 0;
		bzero(tailbuf, 3);
		for (; i < 3; i++)
			input[i] = *src++;
		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;

		fprintf(out, "%c%c%c%c", Base64[output[0]], Base64[output[1]], Base64[output[2]], Base64[output[3]]);
		(*outlen) += 4;
		if ((*outlen) >= 60) {
			fprintf(out, "\r\n");
			*outlen = 0;
		}
	}

	while (2 < srclength) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclength -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;

		fprintf(out, "%c%c%c%c", Base64[output[0]], Base64[output[1]], Base64[output[2]], Base64[output[3]]);
		(*outlen) += 4;
		if ((*outlen) >= 60) {
			fprintf(out, "\r\n");
			*outlen = 0;
		}
	}

	*taillen = srclength;
	for (i = 0; i < srclength; i++)
		tailbuf[i] = *src++;
	return 0;
}
