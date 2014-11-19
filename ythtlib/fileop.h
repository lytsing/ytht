/* fileop.c */
#ifndef __FILEOP_H
#define __FILEOP_H
#include <setjmp.h>
#include <signal.h>

struct mmapfile {
	char *ptr;
	time_t mtime;
	size_t size;
};

sigjmp_buf* push_sigbus();
void popup_sigbus();
void sigbus(int signo);
	
#define MMAP_TRY \
    if (!sigsetjmp(*push_sigbus(), 1)) { \
        signal(SIGBUS, sigbus);

#define MMAP_CATCH \
    } \
    else { \

#define MMAP_END } \
    popup_sigbus();

#define MMAP_UNTRY {popup_sigbus();}
#define MMAP_RETURN(x) {popup_sigbus();return (x);}
#define MMAP_RETURN_VOID {popup_sigbus();return;}

int crossfs_rename(const char *oldpath, const char *newpath);
int readstrvalue(const char *filename, const char *str, char *value, int size);
int savestrvalue(const char *filename, const char *str, const char *value);
int mmapfile(char *filename, struct mmapfile *pmf);
int mmapfilew(char *filename, struct mmapfile *pmf);
int trycreatefile(char *path, char *fnformat, int startnum, int maxtry);
int copyfile(char *source, char *destination);
int f_write(char *file, char *buf);
int f_append(char *file, char *buf);
int openlockfile(const char *filename, int flag, int op);
//int checkfilename(const char *filename);
int clearpath(const char *path);
#endif
