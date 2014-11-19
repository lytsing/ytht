#include "bbslib.h"

extern void
printComments(struct Blog *blog, int fileTime);

int
blogcomment_main()
{
	char userid[IDLEN + 1], tmpfile[80], who[32], by[120], *comment;
	char *ptr, *buffer;
	struct Blog blog;
	int fileTime;
	FILE *fp;

	// can't comment the bellow function, or will 500 error!
	printXBlogHeader();
	check_msg();
	strsncpy(userid, getparm2("U", "userid"), sizeof (userid));
	fileTime = atoi(getparm("T"));
	comment = getparm("comment");

	// add who by deli
	strsncpy(who, getparm("who"), sizeof (who));
	if ((ptr = strchr(who, '\n')))
		*ptr = 0;
	if ((ptr = strchr(who, '\r')))
		*ptr = 0;
	strcpy(who, strtrim(who));
	utf8cut(who, sizeof (who));
	// add end

	strsncpy(by, getparm("by"), sizeof (by));
	if ((ptr = strchr(by, '\n')))
		*ptr = 0;
	if ((ptr = strchr(by, '\r')))
		*ptr = 0;
	strcpy(by, strtrim(by));
	utf8cut(by, sizeof (by));
	if ((!loginok || isguest) && !strchr(by, '@')) {
		printf("填写的 Email 不正确。");
		printXBlogEnd();
		return 0;
	}
	if (openBlogW(&blog, userid) < 0) {
		printf("该用户尚未建立 blog。");
		printXBlogEnd();
		return 0;
	}

	sprintf(tmpfile, "bbstmpfs/tmp/%d.tmp", thispid);

	saveTidyHtml(tmpfile, comment, 1);
	buffer = readFile(tmpfile);
	unlink(tmpfile);
	if (!buffer) {
		closeBlog(&blog);
		printf("内存不足！");
		printXBlogEnd();
		return 0;
	}

	fp = fopen(tmpfile, "w");
	fprintf(fp, "F:%s\n", realfromhost);
	if (loginok && !isguest) {
		char idbuf[40];
		gb2utf8(idbuf, sizeof (idbuf), currentuser->userid);
		fprintf(fp, "B:%s %s\n", idbuf,
			urlencode(currentuser->userid));
	} else {
		fprintf(fp, "W:%s\n", who); // add by deli
		fprintf(fp, "B:%s\n", by);
	}
	if (*buffer != '\n')
		fprintf(fp, "\n");
	fputs(buffer, fp);
	fclose(fp);

	free(buffer);

	blogComment(&blog, fileTime, tmpfile);
	printComments(&blog, fileTime);

	closeBlog(&blog);
	printXBlogEnd();

	return 0;
}
