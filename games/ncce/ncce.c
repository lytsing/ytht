//ecnegrevid 2001.7.20
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <newt.h>
#include <stdlib.h>
#include "ncce.h"
#define Ctrl(c)         ( c & 037 )

int
do_dict()
{
	newtComponent form, label, entry, text;
	struct newtExitStruct es;
	char *entryValue, *wordlist;
	newtCenteredWindow(70, 20, NULL);

	newtPushHelpLine
	    ("����Ҫ���ҵĴʣ��س�ȷ�������¼�������^C�˳���^X���");
	label = newtLabel(2, 1, "������Ҫ���ҵĴ�");
	entry = newtEntry(2, 2, "", 40, (void *) &entryValue,
			  NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
	text = newtTextbox(2, 3, 66, 17, NEWT_FLAG_WRAP | NEWT_FLAG_SCROLL);
	newtTextboxSetText(text, "");
	form = newtForm(NULL, NULL, 0);
	newtFormAddHotKey(form, Ctrl('c'));
	newtFormAddHotKey(form, Ctrl('x'));
	newtFormAddHotKey(form, Ctrl('\r'));
	newtFormAddHotKey(form, Ctrl('\n'));
	newtFormAddComponents(form, label, entry, text, NULL);
	while (1) {
		newtFormRun(form, &es);
		if ((es.u.key == Ctrl('c')
		     || es.u.key == NEWT_KEY_F12)
		    && es.reason == NEWT_EXIT_HOTKEY)
			break;
		if (es.u.key == Ctrl('x') && es.reason == NEWT_EXIT_HOTKEY) {
			newtEntrySet(entry, "", 0);
			continue;
		}
		wordlist = search_dict(entryValue);
		newtTextboxSetText(text, wordlist);
	}
	newtFormDestroy(form);
	newtPopHelpLine();
	newtPopWindow();
	return 0;
}

int
main(int argn, char **argv)
{
	printf("\033[H\033[2J\033[m");
	printf("ʹ��cterm�������'c', �����ն˻س�����");
	fflush(stdout);
	if ('c' == getchar())
		setenv("TERM", "ansi", 1);
	else
		setenv("TERM", "vt100", 1);
	newtInit();
	newtCls();
	newtDrawRootText(0, 0, "�Ƽ��ǵ� 1.0");
	do_dict();
	newtFinished();
	return 0;
}
