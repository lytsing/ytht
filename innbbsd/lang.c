#include <string.h>
#include <stdio.h>
#include "lang.h"

void lang_init(int type);

char Big5FromTxt[] = "�o�H�H: ";
char Big5BoardTxt[] = "�H��: ";
char Big5SubjectTxt[] = "��  �D: ";
char Big5OrganizationTxt[] = "�o�H��: ";
char Big5PathTxt[] = "��H��: ";
char Big5NCTUCSIETxt[] = "��j��u News Server";
char Big5ModerationTxt[] = "�z���峹 \"%s\" �w�g�e���f�֤�. �е��ݦ^��.\n";
char Big5bbslinkUsage1[] = "���{���n���`���楲���N�H�U�ɮ׸m�� %s/innd �U:\n";
char Big5bbslinkUsage2[] = "bbsname.bbs   �]�w�Q���� BBS ID (�о��q²�u)\n";
char Big5bbslinkUsage3[] =
    "nodelist.bbs  �]�w�����U BBS ���� ID, Address �M fullname\n";
char Big5bbslinkUsage4[] =
    "newsfeeds.bbs �]�w�����H�� newsgroup board nodelist ...\n";

char GBFromTxt[] = "������: ";
char GBBoardTxt[] = "����: ";
char GBSubjectTxt[] = "��  ��: ";
char GBOrganizationTxt[] = "����վ: ";
char GBPathTxt[] = "ת��վ: ";
char GBNCTUCSIETxt[] = "�����ʹ� News Server";
char GBModerationTxt[] = "�������� \"%s\" �Ѿ����������. ��ȴ��ظ�.\n";
char GBbbslinkUsage1[] = "����ʽҪ����ִ�б��뽫���µ������ %s/innd ��:\n";
char GBbbslinkUsage2[] = "bbsname.bbs   �趨��վ�� BBS ID (�뾡�����)\n";
char GBbbslinkUsage3[] =
    "nodelist.bbs  �趨��·�� BBS վ�� ID, Address �� fullname\n";
char GBbbslinkUsage4[] =
    "newsfeeds.bbs �趨��·�ż��� newsgroup board nodelist ...\n";

char EFromTxt[] = "From: ";
char EBoardTxt[] = "Board: ";
char ESubjectTxt[] = "Subject: ";
char EOrganizationTxt[] = "Site: ";
char EPathTxt[] = "Path: ";
char ENCTUCSIETxt[] = "NCTU CSIE News Server";
char EModerationTxt[] =
    "Your article \"%s\" has been sent to the moderator. Please wait for reply.\n";
char EbbslinkUsage1[] =
    "To work normally, the following files should be put in %s/innd:\n";
char EbbslinkUsage2[] = "bbsname.bbs   Your BBS ID (in short)\n";
char EbbslinkUsage3[] = "nodelist.bbs  BBS ID, Address and fullname\n";
char EbbslinkUsage4[] = "newsfeeds.bbs newsgroup board nodelist ...\n";

char *FromTxt = Big5FromTxt;
char *BoardTxt = Big5BoardTxt;
char *SubjectTxt = Big5SubjectTxt;
char *OrganizationTxt = Big5OrganizationTxt;
char *PathTxt = Big5PathTxt;
char *NCTUCSIETxt = Big5NCTUCSIETxt;
char *ModerationTxt = Big5ModerationTxt;
char *bbslinkUsage1 = Big5bbslinkUsage1;
char *bbslinkUsage2 = Big5bbslinkUsage2;
char *bbslinkUsage3 = Big5bbslinkUsage3;
char *bbslinkUsage4 = Big5bbslinkUsage4;

/*
typedef struct TxtClass {
  char *msgtxt;
  int size;
} TxtClass;
*/

TxtClass OrganizationTxtClass[] = {
	{Big5OrganizationTxt, sizeof Big5OrganizationTxt}
	,
	{GBOrganizationTxt, sizeof GBOrganizationTxt}
	,
	{EOrganizationTxt, sizeof EOrganizationTxt}
	,
	{NULL, 0}
	,
};

TxtClass FromTxtClass[] = {
	{Big5FromTxt, sizeof Big5FromTxt}
	,
	{GBFromTxt, sizeof GBFromTxt}
	,
	{EFromTxt, sizeof EFromTxt}
	,
	{NULL, 0}
	,
};

TxtClass SubjectTxtClass[] = {
	{Big5SubjectTxt, sizeof Big5SubjectTxt}
	,
	{GBSubjectTxt, sizeof GBSubjectTxt}
	,
	{ESubjectTxt, sizeof ESubjectTxt}
	,
	{NULL, 0}
	,
};

TxtClass PathTxtClass[] = {
	{Big5PathTxt, sizeof Big5PathTxt}
	,
	{GBPathTxt, sizeof GBPathTxt}
	,
	{EPathTxt, sizeof EPathTxt}
	,
	{NULL, 0}
	,
};

TxtClass BoardTxtClass[] = {
	{Big5BoardTxt, sizeof Big5BoardTxt}
	,
	{GBBoardTxt, sizeof GBBoardTxt}
	,
	{EBoardTxt, sizeof EBoardTxt}
	,
	{NULL, 0}
	,
};

int
isMsgTxt(txtclass, orgtxt)
TxtClass *txtclass;
char *orgtxt;
{
	TxtClass *ptr = txtclass;
	while (ptr && ptr->msgtxt && ptr->size > 0) {
		if (strncmp(ptr->msgtxt, orgtxt, ptr->size - 1) == 0)
			return 1;
		ptr++;
	}
	return 0;
}

void
initial_lang()
{
	if (strcasecmp(LANG, "BIG5") == 0) {
		lang_init(Big5Locale);
	} else if (strcasecmp(LANG, "GB") == 0) {
		lang_init(GBLocale);
	} else if (strcasecmp(LANG, "ENGLISH") == 0) {
		lang_init(EnglishLocale);
	}
}

void
lang_init(type)
int type;
{
	switch (type) {
	case Big5Locale:
		FromTxt = Big5FromTxt;
		BoardTxt = Big5BoardTxt;
		SubjectTxt = Big5SubjectTxt;
		OrganizationTxt = Big5OrganizationTxt;
		PathTxt = Big5PathTxt;
		NCTUCSIETxt = Big5NCTUCSIETxt;
		ModerationTxt = Big5ModerationTxt;
		bbslinkUsage1 = Big5bbslinkUsage1;
		bbslinkUsage2 = Big5bbslinkUsage2;
		bbslinkUsage3 = Big5bbslinkUsage3;
		bbslinkUsage4 = Big5bbslinkUsage4;
		break;
	case GBLocale:
		FromTxt = GBFromTxt;
		BoardTxt = GBBoardTxt;
		SubjectTxt = GBSubjectTxt;
		OrganizationTxt = GBOrganizationTxt;
		PathTxt = GBPathTxt;
		NCTUCSIETxt = GBNCTUCSIETxt;
		ModerationTxt = GBModerationTxt;
		bbslinkUsage1 = GBbbslinkUsage1;
		bbslinkUsage2 = GBbbslinkUsage2;
		bbslinkUsage3 = GBbbslinkUsage3;
		bbslinkUsage4 = GBbbslinkUsage4;
		break;
	case EnglishLocale:
		FromTxt = EFromTxt;
		BoardTxt = EBoardTxt;
		SubjectTxt = ESubjectTxt;
		OrganizationTxt = EOrganizationTxt;
		PathTxt = EPathTxt;
		NCTUCSIETxt = ENCTUCSIETxt;
		ModerationTxt = EModerationTxt;
		bbslinkUsage1 = EbbslinkUsage1;
		bbslinkUsage2 = EbbslinkUsage2;
		bbslinkUsage3 = EbbslinkUsage3;
		bbslinkUsage4 = EbbslinkUsage4;
		break;
	}
}
