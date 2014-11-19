#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include "szm.h"

struct szm_ctx *
szm_init(char *host)
{
	int ret = 0;
	struct szm_ctx *ctx;

	ctx = malloc(sizeof (struct szm_ctx));
	if (ctx == NULL)
		return NULL;
	memset(ctx, 0, sizeof (struct szm_ctx));
	ctx->host.sin_port = htons(SZM_PORT);
	ctx->host.sin_family = AF_INET;
	ret = inet_aton(host, &(ctx->host.sin_addr));
	if (ret == 0) {
		free(ctx);
		return NULL;
	}
	return ctx;
}

void
szm_fini(struct szm_ctx *ctx)
{
	if (ctx != NULL)
		free(ctx);
	return;
}

static int
szm_proto(struct szm_ctx *ctx, void *in, int in_len,
	  struct szm_proto_req *szm_req, void **out, int *out_len)
{
	struct timeval tval;
	int fd, len, dlen, ret;
	fd_set rset, wset;
	struct szm_proto_ret szm_ret;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(fd, (struct sockaddr *) &(ctx->host),
		    sizeof (ctx->host)) < 0) {
		close(fd);
		return -SZM_ERR_CONNECT;
	}
	ret = send(fd, szm_req, sizeof (struct szm_proto_req), 0);
	if (ret < sizeof (*szm_req)) {
		close(fd);
		return -SZM_ERR_BROKEN;
	}
	len = in_len;
	dlen = 0;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(fd, &wset);
		tval.tv_sec = 3;
		tval.tv_usec = 0;
		if (select(fd + 1, NULL, &wset, NULL, &tval) <= 0) {
			close(fd);
			return -SZM_ERR_BROKEN;	//wait for read timeout or connection broken 
		}
		if (FD_ISSET(fd, &wset)) {
			ret = send(fd, (in + dlen), len, 0);
			if (ret < 0) {
				close(fd);
				return -SZM_ERR_BROKEN;
			}
			dlen += ret;
			len -= ret;
			if (len <= 0)
				break;
		}
	}
	ret = recv(fd, &szm_ret, sizeof (szm_ret), 0);
	if (ret != sizeof (szm_ret)) {
		close(fd);
		return -SZM_ERR_BROKEN;
	}
	if (ntohl(szm_ret.key) != SZM_KEY) {
		close(fd);
		return -SZM_ERR_BROKEN;
	}
	if (szm_ret.status != 0) {
		close(fd);
		return szm_ret.status;
	}
	dlen = 0;
	len = ntohl(szm_ret.len);
	*out_len = len;
	*out = malloc(len);

	while (1) {
		FD_ZERO(&rset);
		FD_SET(fd, &rset);
		tval.tv_sec = 3;
		tval.tv_usec = 0;
		if (select(fd + 1, &rset, NULL, NULL, &tval) <= 0) {
			close(fd);
			return -SZM_ERR_BROKEN;
		}
		if (FD_ISSET(fd, &rset)) {
			ret = recv(fd, (*out + dlen), len, 0);
			if (ret < 0) {
				close(fd);
				return -SZM_ERR_BROKEN;
			}
			dlen += ret;
			len -= ret;
			if (len <= 0)
				break;
		}
	}

	close(fd);
	return 0;
}

int
szm_scale_jpg(struct szm_ctx *ctx, void *in, int in_len,
	      uint8_t scale, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_JPG;
	szm_req.opt = SZM_OPT_SCALE;
	szm_req.param.scale.scale = scale;

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;
	return 0;
}

int
szm_scale_png(struct szm_ctx *ctx, void *in, int in_len,
	      uint8_t scale, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_PNG;
	szm_req.opt = SZM_OPT_SCALE;
	szm_req.param.scale.scale = scale;

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;

	return 0;
}

int
szm_scale_any(struct szm_ctx *ctx, void *in, int in_len,
	      uint8_t scale, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_ANY;
	szm_req.opt = SZM_OPT_SCALE;
	szm_req.param.scale.scale = scale;

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;

	return 0;
}

int
szm_resize_jpg(struct szm_ctx *ctx, void *in, int in_len, uint16_t new_x,
	       uint16_t new_y, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_JPG;
	szm_req.opt = SZM_OPT_RESIZE;
	szm_req.param.resize.new_x = htons(new_x);
	szm_req.param.resize.new_y = htons(new_y);

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;

	return 0;
}

int
szm_resize_png(struct szm_ctx *ctx, void *in, int in_len, uint16_t new_x,
	       uint16_t new_y, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_PNG;
	szm_req.opt = SZM_OPT_RESIZE;
	szm_req.param.resize.new_x = htons(new_x);
	szm_req.param.resize.new_y = htons(new_y);

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;

	return 0;
}

int
szm_resize_any(struct szm_ctx *ctx, void *in, int in_len, uint16_t new_x,
	       uint16_t new_y, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_ANY;
	szm_req.opt = SZM_OPT_RESIZE;
	szm_req.param.resize.new_x = htons(new_x);
	szm_req.param.resize.new_y = htons(new_y);

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;

	return 0;
}

int
szm_box_jpg(struct szm_ctx *ctx, void *in, int in_len,
	    uint16_t boxsize, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_JPG;
	szm_req.opt = SZM_OPT_BOX;
	szm_req.param.box.boxsize = htons(boxsize);

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;
	return 0;
}

int
szm_box_png(struct szm_ctx *ctx, void *in, int in_len,
	    uint16_t boxsize, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_PNG;
	szm_req.opt = SZM_OPT_BOX;
	szm_req.param.box.boxsize = htons(boxsize);

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;

	return 0;
}

int
szm_box_bmp(struct szm_ctx *ctx, void *in, int in_len,
	    uint16_t boxsize, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_BMP;
	szm_req.opt = SZM_OPT_BOX;
	szm_req.param.box.boxsize = htons(boxsize);

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;
	return 0;
}

int
szm_box_any(struct szm_ctx *ctx, void *in, int in_len,
	    uint16_t boxsize, void **out, int *out_len)
{
	int ret;
	struct szm_proto_req szm_req;

	szm_req.key = htonl(SZM_KEY);
	szm_req.len = htonl(in_len);
	szm_req.type = SZM_IMGTYPE_ANY;
	szm_req.opt = SZM_OPT_BOX;
	szm_req.param.box.boxsize = htons(boxsize);

	ret = szm_proto(ctx, in, in_len, &szm_req, out, out_len);
	if (ret < 0)
		return ret;

	return 0;
}
