#include <time.h>
#include <sys/msg.h>
#include "ythtbbs.h"
#include "blog.h"
#include "strop.h"

void
setBlogFile(char *buf, char *userid, char *file)
{
	sprintf(buf, MY_BBS_HOME "/home/%c/%s/B/%s", mytoupper(userid[0]),
		userid, file);
}

void
setBlogPost(char *buf, char *userid, time_t fileTime)
{
	sprintf(buf, MY_BBS_HOME "/home/%c/%s/B/%d", mytoupper(userid[0]),
		userid, (int) fileTime);
}

void
setBlogAbstract(char *buf, char *userid, time_t fileTime)
{
	sprintf(buf, MY_BBS_HOME "/home/%c/%s/B/%d.A", mytoupper(userid[0]),
		userid, (int) fileTime);
}

void
setBlogCommentIndex(char *buf, char *userid, time_t fileTime)
{
	sprintf(buf, MY_BBS_HOME "/home/%c/%s/B/%d.C", mytoupper(userid[0]),
		userid, (int) fileTime);
}

void
setBlogComment(char *buf, char *userid, time_t fileTime, time_t commentTime)
{
	sprintf(buf, MY_BBS_HOME "/home/%c/%s/B/%d.%d", mytoupper(userid[0]),
		userid, (int) fileTime, (int) commentTime);
}

int
createBlog(char *userid)
{
	int fd;
	char buf[256];
	struct BlogConfig blogConfig;
	struct BlogSubject blogSubject = { count: 0, hide: 0, title:"默认栏目"
	};
	setBlogFile(buf, userid, "");
	mkdir(buf, 0770);

	bzero(&blogConfig, sizeof (blogConfig));
	blogConfig.createTime = time(NULL);
	strsncpy(blogConfig.title, "尚未确定 blog 标题",
		 sizeof (blogConfig.title));
	gb2utf8(blogConfig.useridUTF8, sizeof (blogConfig.useridUTF8), userid);
	strsncpy(blogConfig.useridEN, urlencode(userid),
		 sizeof (blogConfig.useridEN));

	setBlogFile(buf, userid, "config");
	fd = open(buf, O_CREAT | O_WRONLY | O_EXCL, 0660);
	if (fd >= 0) {
		write(fd, &blogConfig, sizeof (blogConfig));
		close(fd);
	}

	setBlogFile(buf, userid, "index");
	close(open(buf, O_CREAT | O_WRONLY | O_EXCL, 0660));
	setBlogFile(buf, userid, "draft");
	close(open(buf, O_CREAT | O_WRONLY | O_EXCL, 0660));
	setBlogFile(buf, userid, "tag");
	close(open(buf, O_CREAT | O_WRONLY | O_EXCL, 0660));
	setBlogFile(buf, userid, "subject");
	fd = open(buf, O_CREAT | O_WRONLY | O_EXCL, 0660);
	if (fd >= 0) {
		write(fd, &blogSubject, sizeof (blogSubject));
		close(fd);
	}
	setBlogFile(buf, userid, "bit");
	close(open(buf, O_CREAT | O_WRONLY | O_EXCL, 0660));
	return 0;
}

static int
_openBlog(struct Blog *blog, char *userid, int wantWrite, int wantDraft)
{
	char buf[256];
	int (*mmapfunc) (char *, struct mmapfile *);
	int lockop;
	if (wantWrite) {
		mmapfunc = mmapfilew;
		lockop = LOCK_EX;
	} else {
		mmapfunc = mmapfile;
		lockop = LOCK_SH;
	}
	bzero(blog, sizeof (*blog));
	strsncpy(blog->userid, userid, sizeof (blog->userid));

	setBlogFile(buf, userid, "config");
	blog->lockfd = open(buf, O_RDONLY, 0660);
	if (blog->lockfd < 0 || flock(blog->lockfd, lockop) < 0) {
		close(blog->lockfd);
		return -1;
	}

	mmapfunc(buf, &blog->configFile);
	if (blog->configFile.size < sizeof (struct BlogConfig)) {
		mmapfile(NULL, &blog->configFile);
		flock(blog->lockfd, LOCK_UN);
		close(blog->lockfd);
		return -1;
	}
	blog->config = (struct BlogConfig *) blog->configFile.ptr;

	setBlogFile(buf, userid, "index");
	mmapfunc(buf, &blog->indexFile);
	blog->index = (struct BlogHeader *) blog->indexFile.ptr;
	blog->nIndex = blog->indexFile.size / sizeof (struct BlogHeader);

	if (wantDraft) {
		setBlogFile(buf, userid, "draft");
		mmapfile(buf, &blog->draftFile);
		blog->draft = (struct BlogHeader *) blog->draftFile.ptr;
		blog->nDraft =
		    blog->draftFile.size / sizeof (struct BlogHeader);
	}

	setBlogFile(buf, userid, "subject");
	mmapfunc(buf, &blog->subjectFile);
	blog->subject = (struct BlogSubject *) blog->subjectFile.ptr;
	blog->nSubject = blog->subjectFile.size / sizeof (struct BlogSubject);

	setBlogFile(buf, userid, "tag");
	mmapfunc(buf, &blog->tagFile);
	blog->tag = (struct BlogTag *) blog->tagFile.ptr;
	blog->nTag = blog->tagFile.size / sizeof (struct BlogTag);

	setBlogFile(buf, userid, "bit");
	mmapfunc(buf, &blog->bitFile);
	blog->wantWrite = wantWrite;
	return 0;
}

int
openBlog(struct Blog *blog, char *userid)
{
	return _openBlog(blog, userid, 0, 0);
}

int
openBlogW(struct Blog *blog, char *userid)
{
	return _openBlog(blog, userid, 1, 0);
}

int
openBlogD(struct Blog *blog, char *userid)
{
	return _openBlog(blog, userid, 1, 1);
}

void
closeBlog(struct Blog *blog)
{
	flock(blog->lockfd, LOCK_UN);
	close(blog->lockfd);
	mmapfile(NULL, &blog->configFile);
	mmapfile(NULL, &blog->indexFile);
	mmapfile(NULL, &blog->draftFile);
	mmapfile(NULL, &blog->subjectFile);
	mmapfile(NULL, &blog->tagFile);
	mmapfile(NULL, &blog->bitFile);
}

void
reopenBlog(struct Blog *blog)
{
	int wantWrite, wantDraft;
	char userid[sizeof (blog->userid)];
	strcpy(userid, blog->userid);
	wantWrite = blog->wantWrite;
	wantDraft = blog->wantDraft;
	closeBlog(blog);
	_openBlog(blog, userid, wantWrite, wantDraft);
}

int
blogPost(struct Blog *blog, char *title, char *tmpfile,
	 int subject, int tags[], int nTag)
{
	int fileTime;
	int i, j;
	char filePath[256];
	struct BlogHeader blogHeader;

	setBlogFile(filePath, blog->userid, "");
	fileTime = trycreatefile(filePath, "%d", time(NULL), 100);
	if (fileTime < 0)
		return -1;

	if (copyfile(tmpfile, filePath) < 0) {
		unlink(filePath);
		return -1;
	}

	bzero(&blogHeader, sizeof (blogHeader));
	blogHeader.fileTime = fileTime;
	strsncpy(blogHeader.title, title, sizeof (blogHeader.title));
	utf8cut(blogHeader.title, sizeof (blogHeader.title));

	if (subject >= blog->nSubject || subject == 0)
		subject = 0;
	blogHeader.subject = subject;
	blog->subject[subject].count++;

	for (i = 0; i < MAXTAG; ++i) {
		blogHeader.tag[i] = -1;
	}
	if (tags && nTag) {
		for (i = 0, j = 0; i < nTag && j < MAXTAG; ++i) {
			int t = tags[i];
			if (t < 0 || t >= blog->nTag)
				break;
			blogHeader.tag[j] = t;
			blog->tag[t].count++;
			j++;
		}
	}

	setBlogFile(filePath, blog->userid, "index");
	append_record(filePath, &blogHeader, sizeof (blogHeader));

	reopenBlog(blog);
	return fileTime;
}

int
blogSaveAbstract(struct Blog *blog, time_t fileTime, char *abstract)
{
	int n;
	char filePath[256];
	struct BlogHeader *blogHeader;

	if (fileTime < 0)
		return -1;
	n = findBlogArticle(blog, fileTime);
	if (n < 0)
		return -1;
	blogHeader = &blog->index[n];
	setBlogAbstract(filePath, blog->userid, fileTime);
	if (abstract) {
		f_write(filePath, abstract);
		if (!blogHeader->hasAbstract)
			blogHeader->hasAbstract = 1;
	} else {
		unlink(filePath);
		if (blogHeader->hasAbstract)
			blogHeader->hasAbstract = 0;
	}
	return 0;
}

int
blogUpdatePost(struct Blog *blog, time_t fileTime, char *title, char *tmpfile,
	       int subject, int tags[], int nTag)
{
	int i, j, n;
	char filePath[256];
	struct BlogHeader blogHeader;

	if (fileTime < 0)
		return -1;
	n = findBlogArticle(blog, fileTime);
	if (n < 0)
		return -1;
	blogHeader = blog->index[n];
	blogHeader.modifyTime = time(NULL);

	if (tmpfile) {
		setBlogPost(filePath, blog->userid, fileTime);
		if (copyfile(tmpfile, filePath) < 0) {
			return -1;
		}
	}

	if (subject < blog->nSubject && subject >= 0) {
		blog->subject[blogHeader.subject].count--;
		blogHeader.subject = subject;
		blog->subject[blogHeader.subject].count++;
	}

	if (tags && nTag) {
		for (i = 0, j = 0; i < nTag && j < MAXTAG; i++) {
			int t = tags[i];
			if (t < 0 || t >= blog->nTag)
				break;
			blog->tag[blogHeader.tag[j]].count--;
			blogHeader.tag[j] = t;
			blog->tag[t].count++;
			j++;
		}
	}
	if (title) {
		strsncpy(blogHeader.title, title, sizeof (blogHeader.title));
		utf8cut(blogHeader.title, sizeof (blogHeader.title));
	}
	blog->index[n] = blogHeader;
	reopenBlog(blog);
	return fileTime;
}

int
blogPostDraft(struct Blog *blog, char *title, char *tmpfile, int draftID)
{
	int fileTime;
	int i;
	char filePath[256];
	struct BlogHeader blogHeader;
	struct mmapfile mf = { ptr:NULL };

	if (draftID > 0) {
		fileTime = draftID;
		setBlogFile(filePath, blog->userid, "D");
		sprintf(filePath + strlen(filePath), "%d", draftID);
		if (!file_exist(filePath))
			draftID = -1;
	}
	if (draftID <= 0) {
		setBlogFile(filePath, blog->userid, "");
		fileTime = trycreatefile(filePath, "D%d", time(NULL), 100);
		if (fileTime < 0)
			return -1;
	}
	if (copyfile(tmpfile, filePath) < 0) {
		unlink(filePath);
		return -1;
	}

	bzero(&blogHeader, sizeof (blogHeader));

	blogHeader.fileTime = fileTime;
	for (i = 0; i < 6; i++)
		blogHeader.tag[i] = -1;
	blogHeader.subject = -1;
	strsncpy(blogHeader.title, title, sizeof (blogHeader.title));
	utf8cut(blogHeader.title, sizeof (blogHeader.title));

	setBlogFile(filePath, blog->userid, "draft");
	while (draftID > 0) {
		struct BlogHeader *blh;
		int i, count;
		if (mmapfilew(filePath, &mf) < 0) {
			draftID = -1;
			break;
		}
		blh = (struct BlogHeader *) mf.ptr;
		count = mf.size / sizeof (*blh);
		for (i = 0; i < count; i++) {
			if (blh[i].fileTime == draftID) {
				memcpy(&blh[i], &blogHeader, sizeof (*blh));
				break;
			}
		}
		mmapfile(NULL, &mf);
		if (i >= count)
			draftID = -1;
		break;
	}
	if (draftID <= 0) {
		append_record(filePath, &blogHeader, sizeof (blogHeader));
	}
	return fileTime;
}

int
deleteDraft(struct Blog *blog, int draftID)
{
	char filePath[80], buf[80];
	struct mmapfile mf = { ptr:NULL };
	struct BlogHeader *blh;
	int i, count;
	sprintf(buf, "D%d", draftID);
	setBlogFile(filePath, blog->userid, buf);
	unlink(filePath);
	setBlogFile(filePath, blog->userid, "draft");
	if (mmapfilew(filePath, &mf) < 0)
		return -1;
	blh = (struct BlogHeader *) mf.ptr;
	count = mf.size / sizeof (*blh);
	for (i = 0; i < count; ++i) {
		if (blh[i].fileTime == draftID) {
			break;
		}
	}
	mmapfile(NULL, &mf);
	if (i >= count)
		return -1;
	delete_record(filePath, sizeof (*blh), i + 1);
	return 0;
}

int
blogComment(struct Blog *blog, time_t fileTime, char *tmpfile)
{
	struct BlogCommentHeader comment;
	char filePath[80], filePath2[80], buf[20];
	time_t commentTime;
	int fd;
	int n;

	setBlogFile(filePath, blog->userid, "");
	snprintf(buf, sizeof (buf), "%d.%%d", (int) fileTime);
	commentTime = trycreatefile(filePath, buf, time(NULL), 100);
	if (commentTime < 0)
		return -1;

	if (copyfile(tmpfile, filePath) < 0) {
		unlink(filePath);
		return -1;
	}
	//setBlogComment(filePath, blog->userid, fileTime, now);
	setBlogCommentIndex(filePath2, blog->userid, fileTime);
	fd = open(filePath2, O_WRONLY | O_APPEND | O_CREAT, 0660);
	if (fd < 0) {
		unlink(filePath);
		return -1;
	}
	comment.commentTime = commentTime;
	if (safewrite(fd, &comment, sizeof (comment)) < 0) {
		unlink(filePath);
		return -1;
	}
	close(fd);

	n = findBlogArticle(blog, fileTime);
	if (n < 0) {
		unlink(filePath);
		//unlink(filePath2);
		return -1;
	}
	blog->index[n].nComment++;
	return 0;
}

int
blogCheckMonth(struct Blog *blog, char buf[32], int year, int month)
{

	return 0;
}

int
blogModifySubject(struct Blog *blog, int subjectID, char *title, int hide)
{
	int fd, isAdd = 0;
	struct BlogSubject *blogSubject, aBlogSubject;
	char buf[256];
	if (subjectID >= 64)
		return -1;
	if (subjectID > blog->nSubject || subjectID < 0)
		return -1;
	if (subjectID == blog->nSubject) {
		bzero(&aBlogSubject, sizeof (aBlogSubject));
		blogSubject = &aBlogSubject;
		isAdd = 1;
	} else {
		blogSubject = &blog->subject[subjectID];
	}

	strsncpy(blogSubject->title, title, sizeof (blogSubject->title));
	utf8cut(blogSubject->title, sizeof (blogSubject->title));
	if (hide)
		blogSubject->hide = 1;
	else
		blogSubject->hide = 0;
	if (!isAdd)
		return 0;
	setBlogFile(buf, blog->userid, "subject");
	fd = open(buf, O_WRONLY | O_CREAT, 0660);
	if (fd >= 0) {
		lseek(fd, subjectID * sizeof (struct BlogSubject), SEEK_SET);
		write(fd, blogSubject, sizeof (struct BlogSubject));
		close(fd);
		reopenBlog(blog);
		return 0;
	}
	return -1;
}

int
blogModifyTag(struct Blog *blog, int tagID, char *title, int hide)
{
	int fd, isAdd = 0;
	struct BlogTag *blogTag, aBlogTag;
	char buf[256];
	if (tagID >= 128)
		return -1;
	if (tagID > blog->nTag || tagID < 0)
		return -1;
	if (tagID == blog->nTag) {
		bzero(&aBlogTag, sizeof (aBlogTag));
		blogTag = &aBlogTag;
		isAdd = 1;
	} else {
		blogTag = &blog->tag[tagID];
	}

	strsncpy(blogTag->title, title, sizeof (blogTag->title));
	utf8cut(blogTag->title, sizeof (blogTag->title));
	if (hide)
		blogTag->hide = 1;
	else
		blogTag->hide = 0;
	if (!isAdd)
		return 0;
	setBlogFile(buf, blog->userid, "tag");
	fd = open(buf, O_WRONLY | O_CREAT, 0660);
	if (fd >= 0) {
		lseek(fd, tagID * sizeof (struct BlogTag), SEEK_SET);
		write(fd, blogTag, sizeof (struct BlogTag));
		close(fd);
		reopenBlog(blog);
		return 0;
	}
	return -1;
}

int
findBlogArticle(struct Blog *blog, time_t fileTime)
{
	int n;
	for (n = 0; n < blog->nIndex; ++n) {
		if (blog->index[n].fileTime == fileTime)
			return n;
	}
	return -1;
}

static int
initBlogTraceMSQ()
{
	int msqid;
	msqid = msgget(getBBSKey(BLOGLOG_MSQ), IPC_CREAT | 0664);
	if (msqid < 0)
		return -1;
	return msqid;
}

static void
blogTrace(char* s)
{
	static int disable = 0;
	static int msqid = -1;
	char buf[512];
	struct mymsgbuf *msg = (struct mymsgbuf *) buf;
	if (disable)
		return;
	snprintf(msg->mtext, sizeof (buf) - sizeof (msg->mtype),
		 "%d %s", (int) time(NULL), s);
	msg->mtype = 1;
	if (msqid < 0) {
		msqid = initBlogTraceMSQ();
		if (msqid < 0) {
			disable = 1;
			return;
		}
	}
	msgsnd(msqid, msg, strlen(msg->mtext), IPC_NOWAIT | MSG_NOERROR);
	return;
}

void
blogLog(char *fmt, ...)
{
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof (buf), fmt, ap);
	va_end(ap);
	blogTrace(buf);
}

