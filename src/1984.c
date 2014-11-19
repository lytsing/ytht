//copy by lepton from backnumber.c writen by ecnegrevid, 2002.9.30
#include "bbs.h"
#include "bbstelnet.h"

char boarddir1984[STRLEN * 2];
static int do1984title(void);
static char *do1984doent(int num, struct fileheader *ent, char buf[512]);
static int do1984_read(int ent, struct fileheader *fileinfo, char *direct);
static int do1984_done(int ent, struct fileheader *fileinfo, char *direct);
static int gettarget_board_title(char *board, char *title, char *filename);
static int do1984(time_t dtime, int mode);

void
set1984file(char *path, char *filename)
{
	strcpy(path, boarddir1984);
	*(strrchr(path, '/') + 1) = 0;
	strcat(path, filename);
}

static int
do1984title()
{

	showtitle("�������", MY_BBS_NAME);
	prints
	    ("�뿪[\033[1;32m��\033[m,\033[1;32me\033[m]  ѡ��[\033[1;32m��\033[m,\033[1;32m��\033[m]  �Ķ�"
	     "[\033[1;32m��\033[m,\033[1;32mRtn\033[m] ����[\033[1;32mh\033[m]\033[m\n");
	prints("\033[1;44m���   %-12s %6s  %-50s\033[m\n", "������", "����",
	       "����");
	clrtobot();
	return 0;
}

static char *
do1984doent(num, ent, buf)
int num;
struct fileheader *ent;
char buf[512];
{
	char b2[512];
	time_t filetime;
	char *date;
	char *t;
	char attached;
	extern char ReadPost[];
	extern char ReplyPost[];
	char c1[8];
	char c2[8];
	int same = NA;

	filetime = ent->filetime;
	if (filetime > 740000000) {
		date = ctime(&filetime) + 4;
	} else {
		date = "";
	}

	attached = (ent->accessed & FH_ATTACHED) ? '@' : ' ';
	strcpy(c1, "\033[1;36m");
	strcpy(c2, "\033[1;33m");
	if (!strcmp(ReadPost, ent->title) || !strcmp(ReplyPost, ent->title))
		same = YEA;
	strncpy(b2, ent->owner, STRLEN);
	if ((t = strchr(b2, ' ')) != NULL)
		*t = '\0';

	if (ent->accessed & FH_1984) {
		sprintf(buf,
			" %s%3d\033[m %c %-12.12s %6.6s %c%s%.36s%.14s\033[m",
			same ? c1 : "", num, ' ', "", "", ' ', same ? c1 : "",
			"-�Ѿ�ͨ����� by ", ent->title + 35);
	} else if (!strncmp("Re:", ent->title, 3)) {
		sprintf(buf, " %s%3d\033[m %c %-12.12s %6.6s %c%s%.50s\033[m",
			same ? c1 : "", num, ' ', b2, date, attached,
			same ? c1 : "", ent->title);
	} else {
		sprintf(buf,
			" %s%3d\033[m %c %-12.12s %6.6s %c%s�� %.47s\033[m",
			same ? c2 : "", num, ' ', b2, date, attached,
			same ? c2 : "", ent->title);
	}
	return buf;
}

static int
do1984_read(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char notgenbuf[128];
	int ch;

	clear();
	setqtitle(fileinfo->title);
	directfile(notgenbuf, direct, fh2fname(fileinfo));
/*	if (fileinfo->accessed[1] & FILE1_1984) {
		move(10, 30);
		prints("�����Ѿ�ͨ�����!");
		pressanykey();
		return FULLUPDATE;
	} else*/
	ch = ansimore(notgenbuf, NA);
	move(t_lines - 1, 0);
	prints("\033[1;44;31m[�Ķ�����] \033[33m���� Q,������һ�� ��,l��"
	       "��һ�� n, <Space>,<Enter>,���������Ķ� x p \033[m");
	//usleep(300000l);
	if (!
	    (ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_PGUP
	     || ch == KEY_DOWN) && (ch <= 0 || strchr("RrEexp", ch) == NULL))
		ch = egetch();
	switch (ch) {
	case 'Q':
	case 'q':
	case KEY_LEFT:
		break;
	case 'j':
	case KEY_RIGHT:
		if (USERDEFINE(currentuser, DEF_THESIS)) {
			sread(0, 0, ent, 0, fileinfo);
			break;
		} else {
			return READ_NEXT;
		}
	case KEY_DOWN:
	case KEY_PGDN:
	case 'n':
	case ' ':
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
	case 'l':
		return READ_PREV;
	case 'x':
		sread(0, 0, ent, 0, fileinfo);
		break;
	case 'p':		/*Add by SmallPig */
		sread(4, 0, ent, 0, fileinfo);
		break;
	}
	return FULLUPDATE;
}

static int
do1984_done(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	if (fileinfo->accessed & FH_1984)
		return (PARTUPDATE);
	post_1984_to_board(direct, fileinfo);
	fileinfo->accessed |= FH_1984;
	sprintf(fileinfo->title, "%-32.32s - %s", fileinfo->title,
		currentuser->userid);
	substitute_record(direct, fileinfo, sizeof (struct fileheader), ent);
	return (PARTUPDATE);
}

struct one_key do1984_comms[] = {
	{'r', do1984_read, "�Ķ��ż�"},
	{'L', show_allmsgs, "�鿴��Ϣ"},
//      {'h', do1984help, "�鿴����"},
	{'c', do1984_done, "ͨ������"},
	{Ctrl('Y'), zmodem_sendfile, "ZMODEM ����"},
	{'\0', NULL, ""}
};

static int
do1984(time_t dtime, int mode)
{
	struct tm *n;

	if (0 == mode) {
		n = localtime(&dtime);
		sprintf(boarddir1984, "boards/.1984/%04d%02d%02d/",
			n->tm_year + 1900, n->tm_mon + 1, n->tm_mday);
	} else if (1 == mode) {
		sprintf(boarddir1984, "boards/.1985/");
	} else
		return -1;
	if (!file_isdir(boarddir1984))
		return -1;
	strcat(boarddir1984, DOT_DIR);
	i_read(DO1984, boarddir1984, do1984title,
	       (void *) do1984doent, do1984_comms, sizeof (struct fileheader));
	return 999;
}

static int
gettarget_board_title(char *board, char *title, char *filename)
{
	FILE *fp;
	char buf[256], *ptr;
	fp = fopen(filename, "r");
	if (fp == NULL)
		return -1;
	fgets(buf, sizeof (buf), fp);
	ptr = strstr(buf, "����: ");
	if (ptr == NULL) {
		fclose(fp);
		return -1;
	}
	snprintf(board, 20, "%s", ptr + sizeof ("����: ") - 1);
	ptr = strrchr(board, '\n');
	if (ptr != NULL)
		*ptr = 0;
	fgets(buf, sizeof (buf), fp);
	fclose(fp);
	ptr = strstr(buf, "��  ��:");
	if (ptr == NULL) {
		return -3;
	}
	snprintf(title, 60, "%s", ptr + sizeof ("��  ��: ") - 1);
	ptr = strrchr(title, '\n');
	if (ptr != NULL)
		*ptr = 0;
	return 0;

}

void
post_1984_to_board(char *dir, struct fileheader *fileinfo)
{
	char *ptr;
	char buf[STRLEN * 2];
	char newfilepath[STRLEN], newfname[STRLEN], targetboard[STRLEN],
	    title[STRLEN];
	struct fileheader postfile;
	time_t now;
	int count;
	snprintf(buf, STRLEN * 2, "%s", dir);
	ptr = strrchr(buf, '/');
	if (NULL == ptr)
		return;
	*(ptr + 1) = 0;
	strcat(ptr, fh2fname(fileinfo));

	memcpy(&postfile, fileinfo, sizeof (struct fileheader));

	now = time(NULL);
	count = 0;
	if (gettarget_board_title(targetboard, title, buf))
		return;
	strcpy(postfile.title, title);
	while (1) {
		sprintf(newfname, "M.%d.A", (int) now);
		setbfile(newfilepath, targetboard, newfname);
		if (link(buf, newfilepath) == 0) {
			//              unlink(buf);
			postfile.filetime = now;
			break;
		}
		now++;
		if (count++ > MAX_POSTRETRY)
			return;
	}
	if (postfile.thread == 0)
		postfile.thread = postfile.filetime;
	setbdir(buf, targetboard, NA);
	if (append_record(buf, &postfile, sizeof (struct fileheader)) == -1) {
		errlog
		    ("checking '%s' on '%s': append_record failed!",
		     postfile.title, targetboard);
		pressreturn();
		return;
	}

	if (innd_board(targetboard))
		outgo_post(&postfile, targetboard, currentuser->userid,
			   currentuser->username);
	updatelastpost(targetboard);
	tracelog("%s check1984 %s %s", currentuser->userid, currboard,
		 postfile.title);
}

void
post_to_1984(char *file, struct fileheader *fileinfo, int mode)
{
	char buf[STRLEN * 2];
	char newfilepath[STRLEN], newfname[STRLEN];
	struct fileheader postfile;
	time_t now;
	int count;
	struct tm *n;

	now = time(NULL);
	if (0 == mode) {
		n = localtime(&now);
		sprintf(buf, "boards/.1984/%04d%02d%02d", n->tm_year + 1900,
			n->tm_mon + 1, n->tm_mday);
	} else if (1 == mode) {
		sprintf(buf, "boards/.1985");
	} else
		return;
	if (!file_isdir(buf))
		if (mkdir(buf, 0770) < 0)
			return;

	memcpy(&postfile, fileinfo, sizeof (struct fileheader));

	count = 0;
	while (1) {
		sprintf(newfname, "M.%d.A", (int) now);
		sprintf(newfilepath, "%s/%s", buf, newfname);
		if (link(file, newfilepath) == 0) {
			postfile.filetime = now;
			break;
		}
		now++;
		if (count++ > MAX_POSTRETRY)
			break;
	}
	strcat(buf, "/" DOT_DIR);
	if (append_record(buf, &postfile, sizeof (struct fileheader)) == -1) {
		errlog
		    ("post1984 '%s' on '%s': append_record failed!",
		     postfile.title, currboard);
		pressreturn();
		return;
	}
	switch (mode) {
	case 0:
		updatelastpost("tochecktoday");
	case 1:
		updatelastpost("delete4request");
	default:
		break;
	}
	tracelog("%s post %s %s", currentuser->userid, currboard,
		 postfile.title);
	return;
}

void
do1984menu()
{
	time_t now;
	char buf[PASSLEN];
	char tmpid[STRLEN];
	int day;
	now = time(NULL);
	modify_user_mode(DO1984);
	clear();
	move(5, 0);
	buf[0] = 0;
	getdata(6, 0,
		"��ѡ��:0)�鿴��������� 1)�鿴��ɾ�������� 2)�������� 3)��ס���� [0]",
		buf, 2, DOECHO, NA);
	switch (atoi(buf)) {
	case 0:
		getdata(7, 0, "Ҫ�����������£�(Ĭ��Ϊ��������) [0]: ",
			buf, 3, DOECHO, YEA);
		day = atoi(buf);
		do1984(now - day * 86400, 0);
		return;
	case 1:
		do1984(now, 1);
		return;
	case 2:
		if (!bbsinfo.utmpshm->watchman) {
			prints("Ŀǰ���沢û����ס!");
			pressreturn();
			return;
		}
		getdata(7, 0, "����������û���¼����: ", buf, PASSLEN, NOECHO,
			YEA);
		if (*buf == '\0'
		    || !checkpasswd(currentuser->passwd, currentuser->salt,
				    buf)) {
			prints("\n\n�ܱ�Ǹ, ����������벻��ȷ��\n");
			pressreturn();
			return;
		}
		sprintf(tmpid, "%u", bbsinfo.utmpshm->unlock % 10000);
		getdata(8, 0, "�����������:", buf, 5, DOECHO, YEA);

		if (strcmp(tmpid, buf)) {
			prints("�����벻��!\n");
			pressreturn();
			return;
		}
		if (!bbsinfo.utmpshm->watchman) {
			prints("Ŀǰ���沢û����ס,�������˱����Ƚ�����!");
			pressreturn();
			return;
		}
		if (time(NULL) < bbsinfo.utmpshm->watchman)
			strcpy(buf, "��ʱ");
		else
			buf[0] = 0;
		bbsinfo.utmpshm->watchman = 0;
		sprintf(tmpid, "�û� %s %s����������������������",
			currentuser->userid, buf);
		postfile("help/watchmanhelp", "deleterequest", tmpid, 1);
		prints("�ɹ�����!\n");
		pressreturn();
		return;
	case 3:
		if (bbsinfo.utmpshm->watchman) {
			prints("Ŀǰ�����Ѿ�����ס��!");
			pressreturn();
			return;
		}
		getdata(7, 0, "����������û���¼����: ", buf, PASSLEN, NOECHO,
			YEA);
		if (*buf == '\0'
		    || !checkpasswd(currentuser->passwd, currentuser->salt,
				    buf)) {
			prints("\n\n�ܱ�Ǹ, ����������벻��ȷ��\n");
			pressreturn();
			return;
		}
		if (bbsinfo.utmpshm->watchman) {
			prints
			    ("Ŀǰ�����Ѿ�����ס��,�������˱�������ס������!");
			pressreturn();
			return;
		}
		bbsinfo.utmpshm->watchman = time(NULL) + 600;
		sprintf(tmpid, "�û� %s ����!������: %u",
			currentuser->userid, bbsinfo.utmpshm->unlock % 10000);
		postfile("help/watchmanhelp", "deleterequest", tmpid, 1);
		prints("�ɹ���ס!");
		pressreturn();
		return;
	default:
		return;
	}
}
