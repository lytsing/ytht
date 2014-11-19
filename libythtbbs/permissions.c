#include "ythtbbs.h"
const char *const permstrings[] = {
	"����Ȩ��",		/* PERM_BASIC */
	"����������",		/* PERM_CHAT */
	"������������",		/* PERM_PAGE */
	"��������",		/* PERM_POST */
	"ʹ����������ȷ",	/* PERM_LOGINOK */
	"��ֹʹ��ǩ����",	/* PERM_DENYSIG */
	"������",		/* PERM_CLOAK */
	"����������",		/* PERM_SEECLOAK */
	"�ʺ����ñ���",		/* PERM_XEMPT */
	"�༭��վ����",		/* PERM_WELCOME */
	"����",			/* PERM_BOARDS */
	"�ʺŹ���Ա",		/* PERM_ACCOUNTS */
	"��վ�ٲ�",		/* PERM_ARBITRATE */
	"ͶƱ����Ա",		/* PERM_OVOTE */
	"ϵͳά������Ա",	/* PERM_SYSOP */
	"Read/Post ����",	/* PERM_POSTMASK */
	"�������ܹ�",		/* PERM_ANNOUNCE */
	"�������ܹ�",		/* PERM_OBOARDS */
	"������ܹ�",		/* PERM_ACBOARD */
	"���� ZAP(������ר��)",	/* PERM_NOZAP */
	"ǿ�ƺ���",		/* PERM_FORCEPAGE */
	"�ӳ�����ʱ��",		/* PERM_EXT_IDLE */
	"������",		/* PERM_SPECIAL1 */
	"����Ȩ�� 2",		/* PERM_SPECIAL2 */
	"����Ȩ�� 3",		/* PERM_SPECIAL3 */
	"����",			/* PERM_SPECIAL4 */
	"��վ�����",		/* PERM_SPECIAL5 */
	"��վ������",		/* PERM_SPECIAL6 */
	"����Ȩ�� 7",		/* PERM_SPECIAL7 */
	"�����ļ�",		/* PERM_SPECIAL8 */
	"��ֹ����Ȩ",		/* PERM_DENYMAIL */
};

const char *const user_definestr[NUMDEFINES] = {
	"�������ر�ʱ���ú��Ѻ���",	/* DEF_FRIENDCALL */
	"���������˵�ѶϢ",	/* DEF_ALLMSG */
	"���ܺ��ѵ�ѶϢ",	/* DEF_FRIENDMSG */
	"�յ�ѶϢ��������",	/* DEF_SOUNDMSG */
	"ʹ�ò�ɫ",		/* DEF_COLOR */
	"��ʾ�����",		/* DEF_ACBOARD */
	"��ʾѡ����ѶϢ��",	/* DEF_ENDLINE */
	"�༭ʱ��ʾ״̬��",	/* DEF_EDITMSG */
	"ѶϢ������һ��/����ģʽ",	/* DEF_NOTMSGFRIEND */
	"ѡ������һ��/����ģʽ",	/* DEF_NORMALSCR */
	"������������ New ��ʾ",	/* DEF_NEWPOST */
	"�Ķ������Ƿ�ʹ���ƾ�ѡ��",	/* DEF_CIRCLE */
	"�Ķ������α�ͣ�ڵ�һƪδ��",	/* DEF_FIRSTNEW */
	"��վʱ��ʾ��������",	/* DEF_LOGFRIEND */
	"��վʱ��ʾ����¼",	/* DEF_INNOTE */
	"��վʱ��ʾ����¼",	/* DEF_OUTNOTE */
	"��վʱѯ�ʼĻ�����ѶϢ",	/* DEF_MAILMSG */
	"ʹ���Լ�����վ����",	/* DEF_LOGOUT */
	"���������֯�ĳ�Ա",	/* DEF_SEEWELC1 */
	"������վ֪ͨ",		/* DEF_LOGINFROM */
	"�ۿ����԰�",		/* DEF_NOTEPAD */
	"��Ҫ�ͳ���վ֪ͨ������",	/* DEF_NOLOGINSEND */
	"����ʽ����",		/* DEF_THESIS */
	"�յ�ѶϢ�Ⱥ��Ӧ�����",	/* DEF_MSGGETKEY */
	"�������ִ���",		/* DEF_DELDBLCHAR */
	"ʹ��GB���Ķ�",		/* DEF_USEGB KCN 99.09.03 */
	"ʹ�ö�̬����",		/* DEF_ANIENDLINE */
	"���η��ʰ�����ʾ���뾫����",	/* DEF_INTOANN */
	"��������ʱ��ʱ����MSG",	/* DEF_POSTNOMSG */
	"��վʱ�ۿ�ͳ����Ϣ",	/* DEF_SEESTATINLOG */
	"���˿������˷�����Ϣ",	/* DEF_FILTERXXX */
	"��ȡվ���ż�"		/* DEF_INTERNETMAIL */
};
