#include "an.h"
#include <unistd.h>
#include <sys/mman.h>

char board[256];
char *c_time();
FILE *do_index(char *, char *, char *, int, int);
int lastno;
int lastisdir;
FILE *lastfile;

void
htmlline(FILE * fp, char *buf)
{
	int i = 0;
	int inansi=0;
	int lsp=-2;
	while (buf[i]) {
		if (buf[i] == 27) {
			inansi = 1;
			i++;
			continue;
		}
		if (!inansi) {
			if (buf[i] == '\r') {
				i++;
				continue;
			}
			if (buf[i] == '\n')
				fprintf(fp, "<br>\r\n");
			else if (buf[i] == '<')
				fprintf(fp, "&lt;");
			else if (buf[i] == '>')
				fprintf(fp, "&gt;");
			else if (buf[i] == '&')
				fprintf(fp, "&amp;");
			else if (buf[i] == ' ') {
				if (lsp == i-1)
					fprintf(fp, "&nbsp;");
				else {
					lsp = i;
					fprintf(fp, " ");
				}
			}
			else  
				fprintf(fp, "%c", buf[i]);
		}
		if (inansi && ((buf[i] > 'a' && buf[i] < 'z')
			       || (buf[i] > 'A' && buf[i] < 'Z')
			       || buf[i] == '@'))
			inansi = 0;
		i++;
	}

}

FILE *
filetohtml(char *from, char *to, int no, int isdir, int depth)
{
	FILE *fr;
	FILE *fw;
	char buf[512], *ext, *name=NULL;
	int pic, i, len, isa, base64;
	char dir[512];
	fr = fopen(from, "r");
	if (fr == NULL)
		return NULL;
	fw = fopen(to, "w");
	if (fw == NULL) {
		fclose(fr);
		return NULL;
	}
	fprintf(fw,
		"<html>\r\n<head>\r\n<meta http-equiv='Content-Type' content='text/html; charset=gb2312'>\r\n</head>\r\n<body>\r\n");
	while (fgets(buf, sizeof (buf), fr) != NULL) {
		base64 = isa = 0;
		if (!strncmp(buf, "begin 644 ", 10)) {
			isa = 1;
			base64 = 1;
			name = buf + 10;
		} else if (checkbinaryattach(buf, fr, &len)) {
			isa = 1;
			base64 = 0;
			name = buf + 18;
		}
		if (isa) {
			strcpy(dir, to);
			ext = strrchr(dir, '.');
			if (ext != NULL)
				*ext = 0;
			switch (getattach(fr, buf, name, dir, base64, len, 0)) {
			case 0:
				if ((ext = strrchr(name, '.')) != NULL) {
					if (strcasecmp(ext, ".bmp")
					    && strcasecmp(ext, ".jpg")
					    && strcasecmp(ext, ".gif")
					    && strcasecmp(ext, ".jpeg"))
						pic = 0;
					else
						pic = 1;
				} else
					pic = 0;
				if (pic)
					fprintf
					    (fw,
					     "附图:\r\n<img src=\"%d/%s\"></img>\r\n",
					     no, name);
				else
					fprintf(fw,
						"附件:\r\n<a href=\"%d/%s\">%s</a>\r\n",
						no, name, name);

				break;
			case -1:
			case -11:
				fprintf(fw,
					"这是一个附件,但是无法建立目录!\r\n");
				break;
			case -2:
			case -12:
				fprintf(fw, "这是一个附件,但是文件名过长!\r\n");
				break;
			case -3:
				fprintf(fw, "这是一个附件,但是解码失败!\r\n");
				break;
			}
		} else
			htmlline(fw, buf);
	}
	fprintf(fw, "<center><a href=javascript:history.go(-1)>返回</a>");
	if (no > 1)
		fprintf(fw, "&nbsp;<a href=./%d%s>上一项</a>", no - 1,
			isdir ? "/index.html" : ".html");
	fprintf(fw, "&nbsp;<a href=./index.html>回到目录</a>&nbsp;<a href=");
	for (i = 1; i < depth; i++)
		fprintf(fw, "../");
	fprintf(fw, "./index.html>首页</a>");
	fclose(fr);
	return fw;
}

int
main(int n, char *arg[])
{
	char buf[256], dir[256], tmp[256],tmp1[256];
	if (n < 2)
		exit(0);
	strcpy(board, arg[1]);
	sprintf(buf, dl_path "/%s.tgz", board);
	if (do_testtime(file_time(buf), ".", 0, 2*3600) == 0) {
		printf("#no need to update %s\n", buf);
		return 0;
	}
	printf("process %s\n",board);
	sprintf(dir, dl_path "/tmp/an.tmp/%s", board);
	sprintf(buf, "mkdir -p %s", dir);
	system(buf);
	system("pwd");
	sprintf(tmp, "一塌糊涂BBS %s版精华区(%s)", board, c_time(time(0)));
	do_index(".", dir, tmp, 1, 0);
	sprintf(tmp1, "%s/tmp/an.tmp/%s", dl_path, board);
	chdir(tmp1);
	sprintf(tmp1, "%s/bin/makechm -b %s", MY_BBS_HOME, board);
	system(tmp1);
	chdir(dl_path "/tmp/an.tmp");
	printf("board: %s\n", board);
	sprintf(buf, "tar -zcf %s.tgz %s", board, board);
	system(buf);
	sprintf(buf, "rm -rf %s", board);
	printf("%s\n", buf);
	system(buf);
	snprintf(buf,sizeof(buf),"%s/%s.tgz",dl_path,board);
	if(file_size(buf)>100*1024*1024)
		truncate(buf,0);
	sprintf(buf, "mv -f %s.tgz %s", board, dl_path);
	printf("%s\n", buf);
	system(buf);
	sync();
	return 0;
}

FILE *
do_index(char *path0, char *path2, char *title0, int thisno, int isdir)
{
	FILE *fp, *fp2;
	unsigned char names[512], index[512], title[256];
	unsigned char genbuf[512], path00[512], path22[512];
	int no = 0, m, lastisdir = 0;
	FILE *lastfp = NULL;
	static int depth = 0;
	depth++;
	mkdir(path2, 0777);
	if (!lfile_isdir(path0))
		return NULL;
	sprintf(names, "%s/.Names", path0);
	fp = fopen(names, "r");

	sprintf(index, "%s/index.html", path2);
	fp2 = fopen(index, "w");
	fprintf(fp2, "<html><head><TITLE>");
	htmlline(fp2, title0);
	fprintf(fp2, "</TITLE> <style> body {font - size:14 px}"
		"A {text - decoration:none; color: #0000FF}"
		" A:hover {color: #FF0000}"
		"</style></head><body><pre><center>");
	htmlline(fp2, title0);
	fprintf(fp2, "<hr width=560>\r\n");
	if (fp != NULL) {
		while (fgets(genbuf, 80, fp) > 0) {
			if (!strncmp(genbuf, "Name=", 5)) {
				sprintf(title, "%s", genbuf + 5);
				title[38] = 0;
				for (m = 0; m < strlen(title); m++)
					if (title[m] <= 27)
						title[m] = 0;
				fgets(genbuf, 256, fp);
				if (!strncmp("Path=~/", genbuf, 6)) {
					for (m = 0; m < strlen(genbuf); m++)
						if (genbuf[m] <= 27)
							genbuf[m] = 0;
					if (!strcmp("Path=~/", genbuf))
						continue;
					sprintf(path00, "%s/%s", path0,
						genbuf + 7);
					for (m = 0; m < strlen(path00); m++)
						if (path00[m] <= 27)
							path00[m] = 0;
					if (!file_exist(path00))
						continue;
					no++;
					if (lfile_isdir(path00)) {
						sprintf(path22, "%s/%d", path2,
							no);
						if (strstr(title, "BMS")
						    || strstr(title, "SYSOPS")
						    || strstr(title, "<HIDE>"))
							continue;
						fprintf(fp2,
							"%4d  [目录] <a href='%d/index.html'>",
							no, no);
						sprintf(genbuf, "%-40.40s",
							title);
						htmlline(fp2, genbuf);
						fprintf(fp2, "</a> [%s]\r\n",
							c_time(file_time
							       (path00)));
						if (lastfp != NULL) {
							fprintf(lastfp,
								"&nbsp;<a href=%s/%d/index.html>下一项</a></center></body></html>\r\n",
								lastisdir ? ".."
								: ".", no);
							fclose(lastfp);
						}
						lastfp =
						    do_index(path00, path22,
							     title, no,
							     lastisdir);
						lastisdir = 1;
						continue;
					}
					sprintf(path22, "%s/%d.html", path2,
						no);
					if (lastfp != NULL) {
						fprintf(lastfp,
							"&nbsp;<a href=%s/%d.html>下一项</a></center></body></html>\r\n",
							lastisdir ? ".." : ".",
							no);
						fclose(lastfp);
					}
					lastfp =
					    filetohtml(path00, path22, no,
						       lastisdir, depth);
					lastisdir = 0;
					fprintf(fp2,
						"%4d  [文件] <a href='%d.html'>",
						no, no);
					sprintf(genbuf, "%-40.40s", title);
					htmlline(fp2, genbuf);
					fprintf(fp2, "</a> [%s]\r\n",
						c_time(file_time(path00)));
				}
			}
		}
		if (lastfp != NULL) {
			fprintf(lastfp, "</body></html>\r\n");
			fclose(lastfp);
		}
		fclose(fp);
	}
	fprintf(fp2,
		"<hr width=560><a href=http://ytht.net><font color=green>一塌糊涂 BBS</font></a>&nbsp;&nbsp;1999-2002&nbsp;&nbsp;</center></pre>\r\n");
	fprintf(fp2, "<center>");
	fprintf(fp2, "<a href=javascript:history.go(-1)>返回</a>");
	if (depth > 1) {
		if (thisno > 1)
			fprintf(fp2, "&nbsp;<a href=../%d%s>上一项</a>",
				thisno - 1, isdir ? "/index.html" : ".html");
		fprintf(fp2,
			"&nbsp;<a href=../index.html>回到目录</a>&nbsp;<a href=");
		for (m = 1; m < depth; m++)
			fprintf(fp2, "../");
		fprintf(fp2, "index.html>首页</a>");
	}
	depth--;
	if (depth)
		return fp2;
	else {
		fprintf(fp2, "</center></body></html>");
		fclose(fp2);
		return NULL;
	}
}

char *
c_time(time_t t)
{
	static char mybuf[256];
	struct tm *lt;
	lt = localtime(&t);
	sprintf(mybuf, "%4d.%02d.%02d", lt->tm_year + 1900,
		lt->tm_mon + 1, lt->tm_mday);
	return mybuf;
}
