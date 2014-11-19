#include "bbs.h"
#include "bbstelnet.h"

#define NADDRESSITEM 6
static const char addressitems[NADDRESSITEM][16] = {
	"oicq����",
	"����Email",
	"�̶��绰",
	"����/�ֻ�",
	"ͨѶ��ַ",
	"��λ/ѧУ"
};
static const char openlevel[4][16] = {
	"������",
	"����Ϊ����",
	"�ҵĺ���",
	"������"
};
static int addressbookmode(char *me, char *him);
static void showaddressitem(char *item, int i, int mode);
static void showaddressbook(char items[][STRLEN], int mode);
static void readaddressbook(char *filename, char items[][STRLEN]);
static void saveaddressbook(char *filename, char items[][STRLEN]);

int
addressbook()
{
	char str[STRLEN], buf[STRLEN], filename[STRLEN],
	    items[NADDRESSITEM][STRLEN];
	int i;
	modify_user_mode(ADDRESSBOOK);
	while (1) {
		move(0, 0);
		clrtobot();
		getdata(0, 0, "(A)��ѯͨѶ¼ (B)����ͨѶ¼ (Q)�뿪 [Q]: ",
			buf, 3, DOECHO, YEA);
		switch (buf[0]) {
		case 'A':
		case 'a':
			usercomplete("������ʹ���ߴ���: ", buf);
			if (!buf[0])
				continue;
			prints("%s��ͨѶ¼��������\n", buf);
			prints("=================================\n");
			sethomefile(filename, buf, "addressbook");
			if (!file_isfile(filename)) {
				prints("���û���δ����ͨѶ¼");
				pressanykey();
				continue;
			}
			prints("��Ŀ       ����\n");
			readaddressbook(filename, items);
			move(5, 0);
			clrtobot();
			showaddressbook(items,
					addressbookmode(currentuser->userid,
							buf));
			pressanykey();
			break;
		case 'B':
		case 'b':
			sethomefile(filename, currentuser->userid,
				    "addressbook");
			readaddressbook(filename, items);
			prints("�༭ͨѶ¼(\033[1;31mС�ı�����\033[m)\n");
			prints("=================================\n");
			prints("��Ŀ       ��������   ����\n");
			for (i = 0; i < NADDRESSITEM; i++) {
				move(4, 0);
				clrtobot();
				showaddressbook(items, -1);
				move(NADDRESSITEM + 4, 0);
				prints("=================================\n");
				showaddressitem(items[i], i, -1);
				sprintf(str, "%s: ", addressitems[i]);
				getdata(NADDRESSITEM + 5, 0, str, items[i] + 1,
					60, DOECHO, items[i][0] ? NA : YEA);
				if (items[i][0] < '0' || items[i][0] > '3')
					items[i][0] = '3';
				sprintf(buf,
					"�������� (0)������ (1)����Ϊ���� (2)�ҵĺ��� (3)������ [%c]",
					items[i][0]);
				if (items[i][1])
					getdata(NADDRESSITEM + 6, 0, buf, str,
						3, DOECHO, YEA);
				if (str[0] < '0' || str[0] > '3')
					str[0] = items[i][0];
				items[i][0] = str[0];
			}
			move(3, 0);
			clrtobot();
			showaddressbook(items, 1);
			prints("=================================\n");
			getdata(NADDRESSITEM + 5, 0,
				"���������Ƿ���ȷ, �� Q ���� (Y/Quit)? [Y]:",
				str, 3, DOECHO, YEA);
			if (str[0] != 'Q' || str[0] == 'q')
				saveaddressbook(filename, items);
			break;
		default:
			return FULLUPDATE;
		}
	}
}

static int
addressbookmode(char *me, char *him)
{
	if (!strcasecmp(me, him))
		return 3;
	if (inoverride(him, me, "friends"))
		return 2;
	if (inoverride(me, him, "friends"))
		return 1;
	return 0;
}

static void
showaddressitem(char *item, int i, int mode)
{
	if (item[0] == 0) {
		prints("%-10s ��δ�趨\n", addressitems[i]);
	} else if (mode == -1) {
		prints("%-10s %-10s %s\n", addressitems[i],
		       openlevel[item[0] - '0'], item + 1);
	} else {
		prints("%-10s %s\n", addressitems[i],
		       (mode >= item[0] - '0') ? (item + 1) : "����");
	}
}

static void
showaddressbook(char items[][STRLEN], int mode)
{
	int i;
	for (i = 0; i < NADDRESSITEM; i++)
		showaddressitem(items[i], i, mode);
}

static void
readaddressbook(char *filename, char items[][STRLEN])
{
	int i = 0;
	FILE *fp;
	char *ptr;
	fp = fopen(filename, "r");
	if (fp != NULL)
		for (i = 0; i < NADDRESSITEM; i++) {
			if (fgets(items[i], STRLEN, fp) == NULL)
				break;
			if ((ptr = strrchr(items[i], '\n')) != NULL)
				*ptr = 0;
		}
	for (; i < NADDRESSITEM; i++)
		items[i][0] = 0;
	if (fp != NULL)
		fclose(fp);
}

static void
saveaddressbook(char *filename, char items[][STRLEN])
{
	int i;
	FILE *fp;
	fp = fopen(filename, "w");
	for (i = 0; i < NADDRESSITEM; i++) {
		fprintf(fp, "%s\n", items[i]);
	}
	fclose(fp);
}
