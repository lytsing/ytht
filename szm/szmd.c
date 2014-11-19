#include <stdio.h>
#include "szmd.h"
#include "szm.h"

volatile static int proc_cnt = 0;

#ifndef gdImageCreateFromJpegPtr
//for libgd2 < 2.0.32
gdImagePtr
gdImageCreateFromJpegPtr(int size, void *data)
{
	FILE *fp;
	gdImagePtr ret;
	fp = fmemopen(data, size, "r");
	ret = gdImageCreateFromJpeg(fp);
	fclose(fp);
	return ret;
}
#endif

#ifndef gdImageCreateFromPngPtr
gdImagePtr
gdImageCreateFromPngPtr(int size, void *data)
{
	FILE *fp;
	gdImagePtr ret;
	fp = fmemopen(data, size, "r");
	ret = gdImageCreateFromPng(fp);
	fclose(fp);
	return ret;
}
#endif

#ifndef gdImageCreateFromBmpPtr
//for libgd2 < 2.99.999
gdImagePtr
gdImageCreateFromBmpPtr(int size, void *data)
{
	gdImagePtr ret;
	bmphandle_t bh;
	int h, w, i, j;
	struct bgrpixel r;
	int color;
	bh = bmp_open(data, size);
	if (bh == NULL)
		return NULL;
	h = bmp_height(bh);
	w = bmp_width(bh);
	ret = gdImageCreateTrueColor(w, h);
	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			r = bmp_getpixel(bh, i, j);
			color = gdImageColorResolve(ret, r.r, r.g, r.b);
			gdImageSetPixel(ret, i, j, color);
		}
	}
	bmp_close(bh);
	return ret;
}
#endif

static int
szm_do_resize_jpg(char *in, int in_len, uint16_t new_x,
		  uint16_t new_y, char **out, int *out_len)
{
	int x, y;
	gdImagePtr im_in, im_out;

	im_in = gdImageCreateFromJpegPtr(in_len, in);
	if (!im_in)
		return -SZM_ERR_IMGFMT;
	x = gdImageSX(im_in);
	y = gdImageSY(im_in);
	if (new_x == 0)
		new_x = 1;
	if (new_y == 0)
		new_y = 1;
	im_out = gdImageCreateTrueColor(new_x, new_y);
	if (!im_out) {
		gdImageDestroy(im_in);
		return -SZM_ERR_IMGPROC;
	}
	gdImageCopyResized(im_out, im_in, 0, 0, 0, 0,
			   im_out->sx, im_out->sy, im_in->sx, im_in->sy);
	*out = gdImageJpegPtr(im_out, out_len, -1);

	gdImageDestroy(im_in);
	gdImageDestroy(im_out);
	return 0;
}

static int
szm_do_scale_jpg(char *in, int in_len, uint8_t scale, char **out, int *out_len)
{
	gdImagePtr im_in, im_out;
	int x, y, new_x, new_y;

	im_in = gdImageCreateFromJpegPtr(in_len, in);
	if (!im_in)
		return -SZM_ERR_IMGFMT;
	x = gdImageSX(im_in);
	y = gdImageSY(im_in);
	new_x = x / scale;
	new_y = y / scale;
	if (new_x == 0)
		new_x = 1;
	if (new_y == 0)
		new_y = 1;
	im_out = gdImageCreateTrueColor(new_x, new_y);
	if (!im_out) {
		gdImageDestroy(im_in);
		return -SZM_ERR_IMGPROC;
	}
	gdImageCopyResized(im_out, im_in, 0, 0, 0, 0,
			   im_out->sx, im_out->sy, im_in->sx, im_in->sy);
	*out = gdImageJpegPtr(im_out, out_len, -1);

	gdImageDestroy(im_in);
	gdImageDestroy(im_out);
	return 0;
}

static int
szm_do_box_jpg(char *in, int in_len, uint16_t boxsize, char **out, int *out_len)
{
	int x, y;
	int new_x, new_y;
	gdImagePtr im_in, im_out;

	im_in = gdImageCreateFromJpegPtr(in_len, in);
	if (!im_in)
		return -SZM_ERR_IMGFMT;
	x = gdImageSX(im_in);
	y = gdImageSY(im_in);
	if (x == 0)
		x = 1;
	if (y == 0)
		y = 1;
	if (x <= boxsize && y <= boxsize) {
		*out = gdImageJpegPtr(im_in, out_len, -1);
		gdImageDestroy(im_in);
		return 0;
	}
	if (x > y) {
		new_y = boxsize * y / x;
		new_x = boxsize;
	} else {
		new_x = x * boxsize / y;
		new_y = boxsize;
	}
	if (new_x == 0)
		new_x = 1;
	if (new_y == 0)
		new_y = 1;
	im_out = gdImageCreateTrueColor(new_x, new_y);
	if (!im_out) {
		gdImageDestroy(im_in);
		return -SZM_ERR_IMGPROC;
	}
	gdImageCopyResized(im_out, im_in, 0, 0, 0, 0,
			   im_out->sx, im_out->sy, im_in->sx, im_in->sy);
	*out = gdImageJpegPtr(im_out, out_len, -1);
	gdImageDestroy(im_in);
	gdImageDestroy(im_out);
	return 0;
}

static int
szm_do_box_bmp(char *in, int in_len, uint16_t boxsize, char **out, int *out_len)
{
	int x, y;
	int new_x, new_y;
	gdImagePtr im_in, im_out;

	im_in = gdImageCreateFromBmpPtr(in_len, in);
	if (!im_in)
		return -SZM_ERR_IMGFMT;
	x = gdImageSX(im_in);
	y = gdImageSY(im_in);
	if (x == 0)
		x = 1;
	if (y == 0)
		y = 1;
	if (x <= boxsize && y <= boxsize) {
		*out = gdImageJpegPtr(im_in, out_len, -1);
		gdImageDestroy(im_in);
		return 0;
	}
	if (x > y) {
		new_y = boxsize * y / x;
		new_x = boxsize;
	} else {
		new_x = x * boxsize / y;
		new_y = boxsize;
	}
	if (new_x == 0)
		new_x = 1;
	if (new_y == 0)
		new_y = 1;
	im_out = gdImageCreateTrueColor(new_x, new_y);
	if (!im_out) {
		gdImageDestroy(im_in);
		return -SZM_ERR_IMGPROC;
	}
	gdImageCopyResized(im_out, im_in, 0, 0, 0, 0,
			   im_out->sx, im_out->sy, im_in->sx, im_in->sy);
	*out = gdImageJpegPtr(im_out, out_len, -1);
	gdImageDestroy(im_in);
	gdImageDestroy(im_out);
	return 0;

	return 0;
}

static int
szm_do_resize_png(char *in, int in_len, uint16_t new_x,
		  uint16_t new_y, char **out, int *out_len)
{
	int x, y;
	gdImagePtr im_in, im_out;

	im_in = gdImageCreateFromPngPtr(in_len, in);
	if (!im_in)
		return -SZM_ERR_IMGFMT;
	x = gdImageSX(im_in);
	y = gdImageSY(im_in);
	if (new_x == 0)
		new_x = 1;
	if (new_y == 0)
		new_y = 1;
	im_out = gdImageCreateTrueColor(new_x, new_y);
	if (!im_out) {
		gdImageDestroy(im_in);
		return -SZM_ERR_IMGPROC;
	}
	gdImageCopyResized(im_out, im_in, 0, 0, 0, 0,
			   im_out->sx, im_out->sy, im_in->sx, im_in->sy);
	*out = gdImagePngPtr(im_out, out_len);

	gdImageDestroy(im_in);
	gdImageDestroy(im_out);
	return 0;
}

static int
szm_do_scale_png(char *in, int in_len, uint8_t scale, char **out, int *out_len)
{
	gdImagePtr im_in, im_out;
	int x, y, new_x, new_y;

	im_in = gdImageCreateFromPngPtr(in_len, in);
	if (!im_in)
		return -SZM_ERR_IMGFMT;
	x = gdImageSX(im_in);
	y = gdImageSY(im_in);
	new_x = x / scale;
	new_y = y / scale;
	if (new_x == 0)
		new_x = 1;
	if (new_y == 0)
		new_y = 1;
	im_out = gdImageCreateTrueColor(new_x, new_y);
	if (!im_out) {
		gdImageDestroy(im_in);
		return -SZM_ERR_IMGPROC;
	}
	gdImageCopyResized(im_out, im_in, 0, 0, 0, 0,
			   im_out->sx, im_out->sy, im_in->sx, im_in->sy);
	*out = gdImagePngPtr(im_out, out_len);

	gdImageDestroy(im_in);
	gdImageDestroy(im_out);
	return 0;
}

static int
szm_do_box_png(char *in, int in_len, uint16_t boxsize, char **out, int *out_len)
{
	int x, y;
	int new_x, new_y;
	gdImagePtr im_in, im_out;

	im_in = gdImageCreateFromPngPtr(in_len, in);
	if (!im_in)
		return -SZM_ERR_IMGFMT;
	x = gdImageSX(im_in);
	y = gdImageSY(im_in);
	if (x == 0)
		x = 1;
	if (y == 0)
		y = 1;
	if (x <= boxsize && y <= boxsize) {
		*out = gdImagePngPtr(im_in, out_len);
		gdImageDestroy(im_in);
		return 0;
	}
	if (x > y) {
		new_y = boxsize * y / x;
		new_x = boxsize;
	} else {
		new_x = x * boxsize / y;
		new_y = boxsize;
	}
	if (new_x == 0)
		new_x = 1;
	if (new_y == 0)
		new_y = 1;
	im_out = gdImageCreateTrueColor(new_x, new_y);
	if (!im_out) {
		gdImageDestroy(im_in);
		return -SZM_ERR_IMGPROC;
	}
	gdImageCopyResized(im_out, im_in, 0, 0, 0, 0,
			   im_out->sx, im_out->sy, im_in->sx, im_in->sy);
	*out = gdImagePngPtr(im_out, out_len);

	gdImageDestroy(im_in);
	gdImageDestroy(im_out);
	return 0;
}

static void
szm_err_quit(int connfd, int err)
{
	struct szm_proto_ret head_ret;

	head_ret.key = htonl(SZM_KEY);
	head_ret.len = 0;
	head_ret.status = err;
	send(connfd, &head_ret, sizeof (head_ret), 0);
	exit(-88);
	return;
}

static int
szm_do_resize(char *in, int in_len, uint8_t type, uint16_t new_x,
	      uint16_t new_y, char **out, int *out_len)
{
	int res;

	switch (type) {
	case SZM_IMGTYPE_JPG:
		res = szm_do_resize_jpg(in, in_len, new_x, new_y, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	case SZM_IMGTYPE_PNG:
		res = szm_do_resize_png(in, in_len, new_x, new_y, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	default:
		res = szm_do_resize_jpg(in, in_len, new_x, new_y, out, out_len);
		if (res == -SZM_ERR_IMGFMT)
			res =
			    szm_do_resize_png(in, in_len, new_x, new_y, out,
					      out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	}
	return 0;
}

static int
szm_do_scale(char *in, int in_len, uint8_t type, uint8_t scale,
	     char **out, int *out_len)
{
	int res;

	if (scale == 0)
		return -SZM_ERR_IMGPROC;
	switch (type) {
	case SZM_IMGTYPE_JPG:
		res = szm_do_scale_jpg(in, in_len, scale, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	case SZM_IMGTYPE_PNG:
		res = szm_do_scale_png(in, in_len, scale, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	default:
		res = szm_do_scale_jpg(in, in_len, scale, out, out_len);
		if (res == -SZM_ERR_IMGFMT)
			res = szm_do_scale_png(in, in_len, scale, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	}
	return 0;
}

static int
szm_do_box(char *in, int in_len, uint8_t type, uint16_t boxsize,
	   char **out, int *out_len)
{
	int res;

	switch (type) {
	case SZM_IMGTYPE_JPG:
		res = szm_do_box_jpg(in, in_len, boxsize, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	case SZM_IMGTYPE_PNG:
		res = szm_do_box_png(in, in_len, boxsize, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	case SZM_IMGTYPE_BMP:
		//      sleep(20);
		res = szm_do_box_bmp(in, in_len, boxsize, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	default:
		res = szm_do_box_jpg(in, in_len, boxsize, out, out_len);
		if (res == -SZM_ERR_IMGFMT)
			res = szm_do_box_png(in, in_len, boxsize, out, out_len);
		if (res < 0) {
			*out = NULL;
			*out_len = 0;
			return res;
		}
		break;
	}
	return 0;
}

static int
szm_do_child(int connfd)
{
	char *buf, *rbuf;
	fd_set rset, wset;
	int res, len, dlen, rlen;
	struct timeval tval;
	struct szm_proto_req head_req;
	struct szm_proto_ret head_ret;

	res = recv(connfd, &head_req, sizeof (head_req), 0);
	if (res != sizeof (head_req)) {
		exit(-1);
	}
	len = ntohl(head_req.len);
	if (len > SZM_MAX_DATALEN) {
		head_ret.key = htonl(SZM_KEY);
		head_ret.len = 0;
		head_ret.status = -SZM_ERR_DATALEN;
		send(connfd, &head_ret, sizeof (head_ret), 0);
		exit(-2);
	}
	buf = (char *) malloc(len);
	if (NULL == buf)
		exit(-7);
	dlen = 0;
	while (1) {
		FD_ZERO(&rset);
		FD_SET(connfd, &rset);
		tval.tv_sec = 1;
		tval.tv_usec = 0;
		if (select(connfd + 1, &rset, NULL, NULL, &tval) <= 0) {
			exit(-3);	//wait for read timeout or connection broken 
		}
		if (FD_ISSET(connfd, &rset)) {
			res = recv(connfd, (buf + dlen), len, 0);
			if (res < 0)
				exit(-4);
			dlen += res;
			len -= res;
			if (len == 0)
				break;
		}
	}
	rbuf = NULL;
	switch (head_req.opt) {
	case SZM_OPT_RESIZE:
		res = szm_do_resize(buf, dlen, head_req.type,
				    ntohs(head_req.param.resize.new_x),
				    ntohs(head_req.param.resize.new_y), &rbuf,
				    &rlen);
		break;
	case SZM_OPT_SCALE:
		res = szm_do_scale(buf, dlen, head_req.type,
				   head_req.param.scale.scale, &rbuf, &rlen);
		break;
	case SZM_OPT_BOX:
		res =
		    szm_do_box(buf, dlen, head_req.type,
			       ntohs(head_req.param.box.boxsize), &rbuf, &rlen);
		break;
	default:
		res = -SZM_ERR_OPT;
		break;
	}
	if (res < 0)
		szm_err_quit(connfd, res);
	head_ret.key = htonl(SZM_KEY);
	head_ret.len = htonl(rlen);
	head_ret.status = 0;
	send(connfd, &head_ret, sizeof (head_ret), 0);
	len = rlen;
	dlen = 0;
	while (1) {
		FD_ZERO(&wset);
		FD_SET(connfd, &wset);
		tval.tv_sec = 1;
		tval.tv_usec = 0;
		if (select(connfd + 1, NULL, &wset, NULL, &tval) <= 0) {
			exit(-5);	//wait for read timeout or connection broken 
		}
		if (FD_ISSET(connfd, &wset)) {
			res = send(connfd, (rbuf + dlen), len, 0);
			if (res < 0)
				exit(-6);
			dlen += res;
			len -= res;
			if (len == 0)
				break;
		}
	}
	if (buf)
		free(buf);
	if (rbuf)
		gdFree(rbuf);
	exit(0);
	return 0;
}

static void
szm_sig_child(int signo)
{
	while (waitpid(-1, NULL, WNOHANG) > 0)
		proc_cnt--;
	return;
}

static int
szm_service(char *listenIP)
{
	int listenfd, connfd;
	int val;
	struct linger ld;
	struct sigaction act;
	//pid_t child_pid;
	struct sockaddr_in cliaddr, servaddr;
	int caddr_len;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if (listenfd < 0) {
		syslog(1, "szmd: socket error");
		exit(-1);
	}

	val = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *) &val,
		   sizeof (val));
	ld.l_onoff = ld.l_linger = 0;
	setsockopt(listenfd, SOL_SOCKET, SO_LINGER, (char *) &ld, sizeof (ld));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SZM_PORT);
	if (!listenIP)
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		inet_aton(listenIP, &servaddr.sin_addr);

	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) <
	    0) {
		syslog(1, "szmd: bind error");
		exit(-1);
	}

	listen(listenfd, 32);

	sigemptyset(&act.sa_mask);
	act.sa_handler = szm_sig_child;
	sigaction(SIGCHLD, &act, NULL);
	while (1) {
		caddr_len = sizeof (cliaddr);
		connfd =
		    accept(listenfd, (struct sockaddr *) &cliaddr, &caddr_len);
		if (connfd < 0)
			continue;
		while (proc_cnt >= SZMD_MAX_PROC) {
			if (waitpid(-1, NULL, 0) > 0)
				proc_cnt--;
		}
		proc_cnt++;
		if (fork() == 0) {
			szm_do_child(connfd);
		}
		close(connfd);
	}
	return 0;
}

int
main(int argc, char **argv)
{
	struct stat st;
#ifdef DEBUG
	int fd;
	void *in;
	char *out;
	int out_len, res;
	fd = open("in.bmp", O_RDONLY);
	stat("in.bmp", &st);
	in = malloc(st.st_size);
	read(fd, in, st.st_size);
	close(fd);
	res = szm_do_box_bmp(in, st.st_size, 400, &out, &out_len);
	printf("%d %p %d\n", res, out, out_len);
	fd = open("out.jpg", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	write(fd, out, out_len);
	close(fd);
	return 0;
#else
	if (getuid() == 0) {
		if (chdir("/tmp/szm")) {
			printf("can't chdir!\n");
			exit(-1);
		}
		if (stat("/tmp/szm/dev/null", &st) || !S_ISCHR(st.st_mode)) {
			printf("no null device!\n");
			exit(-2);
		}
		if (chroot("/tmp/szm")) {
			printf("Can't chroot!\n");
			exit(-3);
		}
		if (setregid(99, 99)) {
			printf("Can't setgid!\n");
			exit(-4);
		}
		if (setreuid(9999, 9999)) {
			printf("Can't setuid!\n");
			exit(-5);
		}
	} else
		printf("Warning! no chroot!\n");

	if (daemon(1, 0))
		return -1;
	if (argc >= 2)
		szm_service(argv[1]);
	else
		szm_service(NULL);
	return 0;
#endif
}
