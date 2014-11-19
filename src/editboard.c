#include "bbs.h"
#include "bbstelnet.h"

int
chk_editboardperm(struct boardheader *bh)
{
	if (USERPERM(currentuser, PERM_BLEVELS))
		return YEA;
	if (!USERPERM(currentuser, PERM_SPECIAL4))
		return NA;
	if (issecm(bh->sec1, currentuser->userid))
		return YEA;
	return NA;
}

int
editboard(char *bname)
{
	struct boardheader bh;
	char ch;
	int pos;
	if (!
	    (pos =
	     new_search_record(BOARDS, &bh, sizeof (bh),
			       (void *) cmpbnames, bname))) {
		prints("���������������");
		pressreturn();
		return 0;
	}
	if (!chk_editboardperm(&bh))
		return 0;
	move(0, 0);
	clear();
	prints("\033[1;5m�޸� %s ��������\033[m\n", bh.filename);
	prints("(A) �������� (B) ������ְ ��ѡ��: ");
	ch = toupper(egetch());
	if (ch != 'A' && ch != 'B')
		return 0;
	if (ch == 'A')
		do_ordainBM(NULL, bh.filename);
	else if (ch == 'B')
		do_retireBM(NULL, bh.filename);
	return 0;
}
