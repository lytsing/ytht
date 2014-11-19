#ifndef BBSLIB_H
#define BBSLIB_H

typedef struct nodelist_t {
	char *node;
	char *exclusion;
	char *host;
	char *protocol;
	char *comments;
	int feedtype;
	FILE *feedfp;
} nodelist_t;

#ifdef FILTER
typedef struct filter_t {
	char *group;
	char *rcmdfilter, *scmdfilter;
} filter_t;

typedef char *(*FuncPtr) ();

#endif

typedef struct newsfeeds_t {
	char *newsgroups;
	char *board;
	char *path;
#ifdef FILTER
	FuncPtr rfilter, sfilter;
	char *rcmdfilter, *scmdfilter;
#endif
} newsfeeds_t;

typedef struct overview_t {
	char *board, *filename, *group;
	time_t mtime;
	char *from, *subject;
} overview_t;

extern char MYBBSID[];
extern char *BBSHOME;
extern char ECHOMAIL[];
extern char BBSFEEDS[];
extern char LOCALDAEMON[];
extern char INNDHOME[];
extern char INNDBINHOME[];
extern char HISTORY[];
extern char LOGFILE[];
extern char INNBBSCONF[];
extern char FILTERCTL[];
extern char INNDTMPFS[];
extern nodelist_t *NODELIST;
extern nodelist_t **NODELIST_BYNODE;
extern newsfeeds_t *NEWSFEEDS, **NEWSFEEDS_BYBOARD;
extern int NFCOUNT, NLCOUNT;
extern int Expiredays, His_Maint_Min, His_Maint_Hour;
extern int LOCALNODELIST, NONENEWSFEEDS;
extern int Maxclient;

# ifndef ARG
#  ifdef __STDC__
#   define ARG(x) x
#  else
#   define ARG(x) ()
#  endif
# endif
int initial_bbs ARG((char *));
char *restrdup ARG((char *, char *));
nodelist_t *search_nodelist ARG((char *, char *));
newsfeeds_t *search_group ARG((char *));
void *mymalloc ARG((int));
void *myrealloc ARG((void *, int));

#endif
