
/***************************************************************************
*                            COPYRIGHT NOTICE                              *
****************************************************************************
*                ncurses is copyright (C) 1992-1995                        *
*                          Zeyd M. Ben-Halim                               *
*                          zmbenhal@netcom.com                             *
*                          Eric S. Raymond                                 *
*                          esr@snark.thyrsus.com                           *
*                                                                          *
*        Permission is hereby granted to reproduce and distribute ncurses  *
*        by any means and for any fee, whether alone or as part of a       *
*        larger distribution, in source or in binary form, PROVIDED        *
*        this notice is included with any such distribution, and is not    *
*        removed from any of its header files. Mention of ncurses in any   *
*        applications linked with it is highly appreciated.                *
*                                                                          *
*        ncurses comes AS IS with no warranty, implied or expressed.       *
*                                                                          *
***************************************************************************/

#ifndef _TERMCAP_H
#define _TERMCAP_H	1
#define NCURSES_VERSION "1.9.9e"

#ifdef __cplusplus
extern "C" 
{
#endif /* __cplusplus */

#include <sys/types.h>

extern char PC;
extern char *UP;
extern char *BC;
extern short ospeed;

extern int tgetent(char *, const char *);
extern int tgetflag(const char *);
extern int tgetnum(const char *);
extern char *tgetstr(const char *, char **);

extern int tputs(const char *, int, int (*)(int));

extern char *tgoto(const char *, int, int);

#ifdef __cplusplus
}
#endif

#endif /* _TERMCAP_H */
