#ifndef __BLOG_H
#define __BLOG_H

#define MAXTAG 5

struct BlogSubject {
	int count;
	int nouse0;
	unsigned char hide:1, nouse1:7;
	char title[31];
	char nouse2[24];
};

struct BlogTag {
	int count;
	int nouse0;
	unsigned char hide:1, nouse1:7;
	char title[31];
	char nouse2[24];
};

struct BlogHeader {
	time_t fileTime;
	time_t modifyTime;
	short nComment;
	short subject;
	short tag[MAXTAG];
	unsigned char hasAbstract:1, nouse1:7;
	char nouse2;
	char title[91];
	char nouse[13];
};

struct BlogCommentHeader {
	time_t commentTime;
	char nouse[12];
};

struct BlogConfig {
	time_t createTime;
	int nouse1[5];
	char useridUTF8[31];
	char useridEN[37];
	char title[61];
	char nouse2[872];
};

struct Blog {
	char userid[IDLEN+1];
	struct mmapfile configFile;
	struct mmapfile indexFile;
	struct mmapfile draftFile;
	struct mmapfile subjectFile;
	struct mmapfile tagFile;
	struct mmapfile bitFile;
	struct BlogConfig *config;
	struct BlogSubject *subject;
	struct BlogHeader *index;
	struct BlogHeader *draft;
	struct BlogTag *tag;
	int nSubject;
	int nIndex;
	int nDraft;
	int nTag;
	int wantWrite;
	int wantDraft;
	int lockfd;
};

void setBlogFile(char *buf, char *userid, char *file);
void setBlogPost(char *buf, char *userid, time_t fileTime);
void setBlogAbstract(char *buf, char *userid, time_t fileTime);
void setBlogCommentIndex(char *buf, char *userid, time_t fileTime);
void setBlogComment(char *buf, char *userid, time_t fileTime, time_t commentTime);
int createBlog(char *userid);
int openBlog(struct Blog *blog, char *userid);
int openBlogW(struct Blog *blog, char *userid);
int openBlogD(struct Blog *blog, char *userid);
void closeBlog(struct Blog *blog);
void reopenBlog(struct Blog *blog);
int blogPost(struct Blog *blog, char *title, char *tmpfile, int subject, int tags[], int nTag);
int blogSaveAbstract(struct Blog *blog, time_t fileTime, char *abstract);
int blogUpdatePost(struct Blog *blog, time_t fileTime, char *title, char *tmpfile, int subject, int tags[], int nTag);
int blogPostDraft(struct Blog *blog, char *title, char *tmpfile, int draftID);
int deleteDraft(struct Blog *blog, int draftID);
int blogComment(struct Blog *blog, time_t fileTime, char *tmpfile);
int blogCheckMonth(struct Blog *blog, char buf[32], int year, int month);
int blogModifySubject(struct Blog *blog, int subjectID, char *title, int hide);
int blogModifyTag(struct Blog *blog, int tagID, char *title, int hide);
int findBlogArticle(struct Blog *blog, time_t fileTime);
void blogLog(char *fmt, ...);

#endif //__BLOG_H

