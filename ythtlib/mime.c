#include "mime.h"
#include <string.h>
#define CHARSET "gb2312"

char *
get_mime_type(char *name)
{
	char *dot;
	dot = strrchr(name, '.');
	if (dot == (char *) NULL)
		return "text/plain; charset=" CHARSET;
	if (strcasecmp(dot, ".html") == 0 || strcasecmp(dot, ".htm") == 0)
		return "text/html; charset=" CHARSET;
	if (strcasecmp(dot, ".jpg") == 0 || strcasecmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcasecmp(dot, ".bmp") == 0)
		return "image/bmp";
	if (strcasecmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcasecmp(dot, ".png") == 0)
		return "image/png";
	if (strcasecmp(dot, ".tif") == 0 || strcasecmp(dot, ".tiff") == 0)
		return "image/tiff";
	if (strcasecmp(dot, ".pcx") == 0)
		return "image/pcx";
	if (strcasecmp(dot, ".css") == 0)
		return "text/css";
	if (strcasecmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcasecmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcasecmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcasecmp(dot, ".mov") == 0 || strcasecmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcasecmp(dot, ".wmv") == 0)
		return "video/x-ms-wmv";
	if (strcasecmp(dot, ".mpeg") == 0 || strcasecmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcasecmp(dot, ".vrml") == 0 || strcasecmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcasecmp(dot, ".midi") == 0 || strcasecmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcasecmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcasecmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";
	if (strcasecmp(dot, ".txt") == 0)
		return "text/plain; charset=" CHARSET;
	if (strcasecmp(dot, ".xht") == 0 || strcasecmp(dot, ".xhtml") == 0)
		return "application/xhtml+xml";
	if (strcasecmp(dot, ".xml") == 0)
		return "text/xml";
	if (strcasecmp(dot, ".swf") == 0)
		return "application/x-shockwave-flash";
	return "application/octet-stream";
}
