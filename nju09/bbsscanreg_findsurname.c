/* used for surname checkup.
 * input parameter: surname string
 * result: print checkup result to browser
 */

#include "bbslib.h"

#define SURNAME_FILEPATH MY_BBS_HOME "/etc/surname"

int
bbsscanreg_findsurname_main()
{
	char *surname;
	FILE *fp;
	char buf[64];
	int len, find = 0;

	if (!loginok || isguest || !USERPERM(currentuser, PERM_ACCOUNTS))
		http_fatal("unknown request");
	surname = getparm("surname");
	len = strlen(surname);
	html_header(1);
	printf("<body>");
	if (surname[0] == 0) {
		/* no input, ask for it */
		printf("<form name=form1 method=post action=bbsscanreg_findsurname>\n");
		printf("请输入要查询的姓氏: <input type=text name=surname size=10> <input type=submit value=查询></form></body></html>\n");
		printf("<script>document.form1.surname.focus();</script>\n");
		return 0;
	}
	fp = fopen(SURNAME_FILEPATH, "r");
	if (!fp)
		http_fatal("数据文件错误！无法查询！");
	printf("查询结果<hr>\n");
	while (fgets(buf, 256, fp)) {
		if (!strncmp(surname, buf, len)) {
			printf("<b>%s</b><br>\n", buf);
			find = 1;
			break;
		}
	}
	fclose(fp);
	if (!find)
		printf("<i>&lt;none&gt;</i><br>\n");
	printf("<a href=bbsscanreg_findsurname>继续查询</a></body></html>\n");
	return 0;
}

