#include "bbslib.h"

int
bbsdel_main()
{
	//��ԭ��
	struct fileheader f;
	struct userec *u, tmp;
	char dir[80], board[80], file[80], *id;
	int num = 0, filetime;
	struct boardmem *brd;
	int ret;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("���ȵ�¼");
	changemode(EDIT);
	id = currentuser->userid;
	getparmboard(board, sizeof(board));
	strsncpy(file, getparm("F"), 30);
	if (!file[0])
		strsncpy(file, getparm("file"), 20);
	if (strncmp(file, "M.", 2) && strncmp(file, "G.", 2))
		http_fatal("����Ĳ���");
	if (strstr(file, ".."))
		http_fatal("����Ĳ���");
	filetime = atoi(file + 2);
	brd = getboard(board);
	if (brd == 0)
		http_fatal("�������");
	sprintf(dir, "boards/%s/.DIR", board);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &f, &num, 1);
	if (ret == -1)
		http_fatal("������ļ���");
	if (strcmp(id, f.owner))
//                  && !has_BM_perm(currentuser, brd))
		http_fatal("����Ȩɾ������");
	if (!strcmp(board, "syssecurity"))
		http_fatal("��Ȩɾ��ϵͳ��¼");
	delete_file(dir, sizeof (struct fileheader), num + 1, NULL);
	cancelpost(board, currentuser->userid, &f,
		   !strcmp(currentuser->userid, f.owner));
	updatelastpost(board);
	printf("ɾ���ɹ�.<br><a href='doc?B=%d&S=%d'>���ر�������</a>",
	     getbnumx(brd), num - 10);
	if (!strcmp(id, f.owner)) {
		if (getuser(f.owner, &u) > 0) {
			if (brd->header.clubnum == 0 && !junkboard(board)) {
				if (u->numposts > 0) {
					memcpy(&tmp, u, sizeof (tmp));
					tmp.numposts--;
					updateuserec(&tmp, 0);
				}
			}
		}
	}
	return 0;
}
