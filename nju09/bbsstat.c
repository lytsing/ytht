#include "bbslib.h"

struct mystat {
	int key;
	int num;
};

int
search_stat(void *ptr, int size, int key)
{
	int total, low, high, mid;
	struct mystat *totest;
	total = size / sizeof (struct mystat);
	low = 0;
	high = total - 1;
	while (low < high) {
		mid = (low + high) / 2;
		if (mid < 0 || mid >= total)
			break;
		totest = (struct mystat *) (ptr + mid * sizeof (struct mystat));
		if (key == totest->key)
			return totest->num;
		else if (key > totest->key)
			high = mid - 1;
		else
			low = mid + 1;
	}
	if (low > total - 1)
		low = total - 1;
	totest = (struct mystat *) (ptr + low * sizeof (struct mystat));
	return totest->num;

}

int
bbsstat_main()
{
	int logins = 0, posts = 0, stays = 0, lifes = 0, total = 0;
	struct mmapfile mf = { ptr:NULL };
	struct mystat *ms;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ���������");
	if(!file_exist("wwwtmp/stat_age"))
		http_fatal("��Ǹ,ϵͳ���ݲ���");
	changemode(GMENU);

	printf("<body><center>%s -- ��������ͳ�� [ʹ����: %s]<hr>\n",
	       BBSNAME, currentuser->userid);
	printf
	    ("<table width=320><tr><td>��Ŀ<td>��ֵ<td>ȫվ����<td>��Ա���\n");
	mmapfile("wwwtmp/stat_age", &mf);
	ms = (struct mystat *) (mf.ptr + (mf.size - sizeof (struct mystat)));
	total = ms->num;
	lifes =
	    search_stat(mf.ptr, mf.size,
			(now_t - currentuser->firstlogin) / 86400);
	mmapfile(NULL, &mf);
	mf.ptr = NULL;
	printf("<tr><td>��վ����<td>%ld��<td>%d<td>TOP %5.2f%%",
	       (now_t - currentuser->firstlogin) / 86400, lifes,
	       (lifes * 100.) / total);
	mmapfile("wwwtmp/stat_login", &mf);
	logins = search_stat(mf.ptr, mf.size, (currentuser->numlogins));
	mmapfile(NULL, &mf);
	mf.ptr = NULL;
	printf("<tr><td>��վ����<td>%d��<td>%d<td>TOP %5.2f%%",
	       currentuser->numlogins, logins, logins * 100. / total);
	mmapfile("wwwtmp/stat_post", &mf);
	posts = search_stat(mf.ptr, mf.size, (currentuser->numposts));
	mmapfile(NULL, &mf);
	mf.ptr = NULL;
	printf("<tr><td>��������<td>%d��<td>%d<td>TOP %5.2f%%",
	       currentuser->numposts, posts, posts * 100. / total);
	mmapfile("wwwtmp/stat_stay", &mf);
	stays = search_stat(mf.ptr, mf.size, (currentuser->stay / 60 / 60));
	mmapfile(NULL, &mf);
	mf.ptr = NULL;
	printf("<tr><td>����ʱ��<td>%ld����<td>%d<td>TOP %5.2f%%",
	       currentuser->stay / 60, stays, stays * 100. / total);
	printf("</table><br>���û���: %d</center></body>", total);
	return 0;
}
