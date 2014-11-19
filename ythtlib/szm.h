#ifndef _SZM_H
#define _SZM_H
#include <arpa/inet.h>

#define SZM_PORT  1217
#define SZM_KEY   0x19781217

#define SZM_OPT_RESIZE 1
#define SZM_OPT_SCALE  2
#define SZM_OPT_BOX    3

#define SZM_IMGTYPE_ANY 0	//type auto detect
#define SZM_IMGTYPE_JPG 1
#define SZM_IMGTYPE_PNG 2
#define SZM_IMGTYPE_BMP 3

#define SZM_MAX_DATALEN  (5*1024*1024)

struct resize_param {
	uint16_t new_x;		//resize wide to new_x
	uint16_t new_y;		//resize height to new_y
} __attribute__ ((packed));

struct scale_param {
	uint8_t scale;		//scale to 1/scale
} __attribute__ ((packed));

struct box_param {
	uint16_t boxsize;	//put image into a boxsize squared box
} __attribute__ ((packed));

struct szm_proto_req {
	uint32_t key;		//magic key
	uint32_t len;		//length of data to process
	uint8_t type;		//data type, currently support jpeg and png
	uint8_t opt;		//can be resize or scale or box
	uint16_t reserved;	//unused bytes
	union szm_param {
		struct resize_param resize;	//param for resize opt
		struct scale_param scale;	//param for scale opt
		struct box_param box;	//param for box opt
	} param;
} __attribute__ ((packed));

#define SZM_ERR_BROKEN  1	//link is broken
#define SZM_ERR_DATALEN     2	//data length is too long
#define SZM_ERR_OPT     3	//no such operation
#define SZM_ERR_IMGFMT     4	//wrong format or un support format
#define SZM_ERR_IMGPROC     5	//error occor when processing image
#define SZM_ERR_CONNECT  6	//can not connect

struct szm_proto_ret {
	uint32_t key;		//magic key
	uint32_t len;		//length of data to process
	int8_t status;
} __attribute__ ((packed));

struct szm_ctx {
	struct sockaddr_in host;
} __attribute__ ((packed));

struct szm_ctx *szm_init(char *host);
void szm_fini(struct szm_ctx *ctx);
int szm_scale_jpg(struct szm_ctx *ctx, void *in, int in_len,
		  uint8_t scale, void **out, int *out_len);
int szm_scale_png(struct szm_ctx *ctx, void *in, int in_len,
		  uint8_t scale, void **out, int *out_len);
int szm_scale_any(struct szm_ctx *ctx, void *in, int in_len,
		  uint8_t scale, void **out, int *out_len);
int szm_resize_jpg(struct szm_ctx *ctx, void *in, int in_len, uint16_t new_x,
		   uint16_t new_y, void **out, int *out_len);
int szm_resize_png(struct szm_ctx *ctx, void *in, int in_len, uint16_t new_x,
		   uint16_t new_y, void **out, int *out_len);
int szm_resize_any(struct szm_ctx *ctx, void *in, int in_len, uint16_t new_x,
		   uint16_t new_y, void **out, int *out_len);
int szm_box_jpg(struct szm_ctx *ctx, void *in, int in_len,
		uint16_t boxsize, void **out, int *out_len);
int szm_box_png(struct szm_ctx *ctx, void *in, int in_len,
		uint16_t boxsize, void **out, int *out_len);
int szm_box_bmp(struct szm_ctx *ctx, void *in, int in_len,
		uint16_t boxsize, void **out, int *out_len);
int szm_box_any(struct szm_ctx *ctx, void *in, int in_len,
		uint16_t boxsize, void **out, int *out_len);
#endif
