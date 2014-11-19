/* platform.h

  (c) 1998 (W3C) MIT, INRIA, Keio University
  See tidy.c for the copyright notice.
*/

/*
  Uncomment and edit this #define if you want
  to specify the config file at compile-time

#define CONFIG_FILE "/etc/tidy_config.txt"
*/

/*
  Uncomment this if you are on a Unix system supporting
  the call getpwnam() and the HOME environment variable.
  It enables tidy to find config files named ~/.tidyrc
  and ~your/.tidyrc etc if the HTML_TIDY environment
  variable is not set. Contributed by Todd Lewis.

#define SUPPORT_GETPWNAM
*/

#include <ctype.h>
#include <stdio.h>
#include <setjmp.h>  /* for longjmp on error exit */
#include <stdlib.h>
#include <stdarg.h>  /* may need <varargs.h> for Unix V */
#include <string.h>
#include <assert.h>

#ifdef SUPPORT_GETPWNAM
#include <pwd.h>
#endif

/* used to point to Web Accessibility Guidelines */
#define ACCESS_URL  "http://www.w3.org/WAI/GL"


#ifdef NEEDS_UNISTD_H
#include <unistd.h>  /* needed for unlink on some Unix systems */
#endif

/*
 Tidy preserves the last modified time for the files it
 cleans up. If your platform doesn't support <sys/utime.h>
 and the futime function, then set PRESERVEFILETIMES to 0
*/
#define PRESERVEFILETIMES 0 

#if PRESERVEFILETIMES
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/utime.h>

/*
   MS Windows needs _ prefix for Unix file functions
   Tidy uses for preserving the lasted modified time
*/
#ifdef _WIN32
#define futime _futime
#define fstat _fstat
#define utimbuf _utimbuf
#define stat _stat
#endif /* _WIN32 */
#endif /* PRESERVEFILETIMES */

/* hack for gnu sys/types.h file  which defines uint and ulong */
/* you may need to delete the #ifndef and #endif on your system */

#ifndef __USE_MISC
typedef unsigned int uint;
typedef unsigned long ulong;
#endif /* __USE_MISC */

typedef unsigned char byte;

typedef char *UTF8;

/*
  bool is a reserved word in some but
  not all C++ compilers depending on age
  work around is to avoid bool altogether
  by introducing a new enum called Bool
*/
typedef enum
{
   no,
   yes
} Bool;

/* for null pointers */
#define null 0

/*
  portability hack for deleting files - this is used
  in pprint.c for deleting superfluous slides.

  Win32 defines _unlink as per Unix unlink function.
*/

#ifdef WINDOWS
#define unlink _unlink
#endif

typedef struct
{
    int encoding;
    int state;     /* for ISO 2022 */
    FILE *fp;
} Out;

void outc(uint c, Out *out);

void *MemAlloc(uint size);
void *MemRealloc(void *mem, uint newsize);
void MemFree(void *mem);

/* string functions */
uint ToLower(uint c);
uint ToUpper(uint c);
char *wstrdup(char *str);
char *wstrndup(char *str, int len);
void wstrncpy(char *s1, char *s2, int size);
void wstrcpy(char *s1, char *s2);
int wstrcmp(char *s1, char *s2);
int wstrcasecmp(char *s1, char *s2);
int wstrncmp(char *s1, char *s2, int n);
int wstrlen(char *str);
void ClearMemory(void *, uint size);

void tidy_out(FILE *fp, const char* msg, ...);



