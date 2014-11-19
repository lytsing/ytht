#include "bbslib.h"

//filterlevel:
//      0, for article content
//      1, for comment
int
saveTidyHtml(char *filename, char *content, int filterlevel)
{
	FILE *fp;
	char tmpfn[80], buf[256];
	sprintf(tmpfn, "bbstmpfs/tmp/%d.tidy", thispid);
	fp = fopen(tmpfn, "w");
	fprintf(fp, "<html><body>");
	fputs(content, fp);
	fprintf(fp, "</body></html>");
	fclose(fp);
	snprintf(buf, sizeof (buf), "/usr/bin/tidy -utf8 %s 2>/dev/null |"
		 "bin/htmlfilter %d >  %s", tmpfn, filterlevel, filename);
	system(buf);
	unlink(tmpfn);

	return 0;
}

char *
readFile(char *filename)
{
	char *buffer;
	struct mmapfile mf = { ptr:NULL };
	mmapfile(filename, &mf);
	buffer = malloc(mf.size + 1);
	if (!buffer) {
		mmapfile(NULL, &mf);
		return NULL;
	}
	memcpy(buffer, mf.ptr, mf.size);
	buffer[mf.size] = 0;
	mmapfile(NULL, &mf);

	return buffer;
}

static int
getparmtag(int tags[], int max)
{
	int i = 0;
	char buf[80], *ptr;
	if (!max)
		return 0;
	strsncpy(buf, getparm("tag"), sizeof (buf));
	ptr = buf;
	while (i < max && ptr && isdigit(*ptr)) {
		tags[i] = atoi(ptr);
		i++;
		ptr = strchr(ptr, ';');
		if (ptr)
			ptr++;
	}

	return i;
}

static void
logPost(struct Blog *blog, int fileTime)
{
	int n;
	char buf[512];
	n = findBlogArticle(blog, fileTime);
	if (n >= 0) {
		//snprintf(buf, sizeof (buf), "%s\n", blog->userid);
		//f_append("blog/blog.post", buf);
		// TODO: make the second userid to utf8
		snprintf(buf, sizeof (buf), "%d\t%s\t%s\t%s\n",
				fileTime,
				blog->userid,
				blog->userid,
				(blog->index + n)->title);
		f_append("blog/blog.post", buf);
	}
}

int
blogsend_main()
{
	struct Blog blog;
	char buf[512], filename[80];
	char *abstract, *content, title[512], *ptr;
	int subject;
	int tags[MAXTAG], ntag;
	int draftID = 0;
	int fileTime = 0;
	int size;

	printXBlogHeader();

	strsncpy(title, strtrim(getparm("title")), sizeof (title));
	if (!*title) {
		printf("请设定标题。");
		printXBlogEnd();
		return 0;
	}
	ptr = title;
	while (*ptr) {
		if (strchr("\t\r\n", *ptr))
			*ptr = ' ';
		ptr++;
	}

	subject = atoi(getparm("subject"));
	ntag = getparmtag(tags, MAXTAG);
	abstract = getparm("abstract");
	content = getparm("content");

	draftID = atoi(getparm("draftID"));
	fileTime = atoi(getparm("T"));

	if (openBlogW(&blog, currentuser->userid) < 0) {
		printf("您的 blog 尚未建立。");
		printXBlogEnd();
		return 0;
	}

	printBlogHeader(&blog);

	sprintf(filename, "bbstmpfs/tmp/%d.tmp", thispid);

	saveTidyHtml(filename, content, 0);

	//if (insertattachments_byfile(filename, filename2, currentuser->userid))
	//      mark = mark | FH_ATTACHED;
	//unlink(filename2);

	if (subject == -1) {
		blogPostDraft(&blog, title, filename, draftID);
		goto END;
	}

	if (fileTime > 0) {
		blogUpdatePost(&blog, fileTime, title, filename, subject, tags,
			       ntag);
		goto END;
	}
	//post filename
	fileTime = blogPost(&blog, title, filename, subject, tags, ntag);
	if (fileTime >= 0 && draftID > 0) {
		deleteDraft(&blog, draftID);
	}
	logPost(&blog, fileTime);

      END:
	size = file_size(filename);
	unlink(filename);
	if (subject == -1) {
		sprintf(buf, "blogdraft");
	} else if (size > 2500) {
		int n = findBlogArticle(&blog, fileTime);
		if (n > 0 && !blog.index[n].hasAbstract) {
			sprintf(buf, "blogeditabstract?T=%d&s=%d",
				(int) fileTime, size);
		} else {
			sprintf(buf, "blog?U=%s", blog.config->useridEN);
		}
	} else {
		sprintf(buf, "blog?U=%s", blog.config->useridEN);
	}
	closeBlog(&blog);
	redirect(buf);
	printXBlogEnd();

	return 0;
}
