#include "bbslib.h"
#include "self_photo.h"

#ifdef SELF_PHOTO_VOTE
static int
set_score(int type, int vote_type, int item, char *user, int score)
{
	char buf[256];
	int fd, oldscore = 0;
	snprintf(buf, sizeof (buf), "self_photo_vote/%d-%d-.%d.A/%s", type,
		 vote_type, item, user);
	fd = open(buf, O_RDWR | O_CREAT, 0600);
	if (fd < 0) {
		http_fatal(buf);
		return -1;
	}
	buf[0] = 0;
	if (read(fd, buf, 20) >= 0)
		oldscore = atoi(buf);
	sprintf(buf, "%d\n", score);
	lseek(fd, 0, SEEK_SET);
	write(fd, buf, strlen(buf));
	close(fd);
	return oldscore;
}
#endif
int
bbsself_photo_vote_main()
{
#ifndef SELF_PHOTO_VOTE
	http_fatal("�Ѿ�������!");
	return 0;
#else
	int vote_type, type, score, item, oldscore;
	html_header(1);
	if (!loginok || isguest) {
		printf("�Ҵҹ��Ͳ���ͶƱ,���ȵ�¼!<br><br>");
		printf("<script>openlog();</script>");
		return 0;
	}
	vote_type = atoi(getparm("v"));
	type = atoi(getparm("t"));
	score = atoi(getparm("s"));
	item = atoi(getparm("i") + 3);
	changemode(VOTING);
	if (!USERPERM(currentuser, PERM_VOTE))
		http_fatal("�Բ�������ȨͶƱ");
	if (score < 0 || score > 10)
		http_fatal("�Բ���, �������̫������!");
	oldscore = set_score(type, vote_type, item, currentuser->userid, score);
	if (oldscore >= 0)
		printf("��������������Ƭ�����۴�%s�����%d�ָı�Ϊ%d��",
		       self_an_vote_ctype[vote_type], oldscore, score);
	else
		printf("�ڲ�����,����ϵϵͳά��!");
	printf("<script>setTimeout(\"close()\",1500);</script>");
	return 0;
#endif
}
