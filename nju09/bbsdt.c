#include "bbslib.h"
#include "tshirt.h"
static int do_ts(struct tshirt *t, int index);

int
bbsdt_main()
{
	struct tshirt t;
	int index;
	html_header(1);
	changemode(SELECT);
	/*if(localtime(&now_t)->tm_hour>=14) */ http_fatal("�Ѿ�������!");
	index = atoi(getparm("index")) - 1;
	t.style = atoi(getparm("style")) - 1;
	t.fm = atoi(getparm("fm")) - 1;
	t.size = atoi(getparm("size")) - 1;
	t.price = atoi(getparm("price")) - 1;
	t.color = atoi(getparm("color")) - 1;
	t.num = atoi(getparm("num"));
	t.dtime = now_t;
	strsncpy(t.address, getparm("address"), 200);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܽ��б������");
	if (0 == t.num)
		bzero(&t, sizeof (struct tshirt));
	if (t.style < 0 || t.fm < 0 || t.size < 0 || t.price < 0
	    || t.color < 0 || t.num < 0 || t.style > 2 || t.fm > 2
	    || t.size > 4 || t.price > 1 || t.color > 6 || (0 == t.num
							    && index < 0))
		http_fatal("����Ĳ���1");

	if (0 == t.price && 0 != t.num
	    && (0 != t.color || t.fm != 0 || t.size == 0 || t.size == 4))
		http_fatal("�͵���ֻ�а�ɫ��,�п��,����û��S XXL��");
	else if (1 == t.price && 0 == t.fm
		 && ((t.color < 6 && t.color > 3) || t.size == 0
		     || t.size ==
		     4)) http_fatal("�ߵ��п�û��ˮ���ͽۺ�,����û��S XXL��");
	else if (1 == t.price && 1 == t.fm
		 && ((t.color != 2 && t.color != 4 && t.color != 6)
		     || t.size != 1))
		http_fatal("Ůһ����ֻ��ˮ���ʹ��ɫ�ͺ�ɫ,ֻ��Mһ����");
	else if (1 == t.price && 2 == t.fm
		 &&
		 ((t.color != 3 && t.color != 5 && t.color != 0 && t.color != 6)
		  || (t.size != 1 && t.size != 2)))
		http_fatal
		    ("ŮԲ���M��L������,����ֻ�а� ���� �ۺ� �� ������ɫ");
	if (t.num >= 100)
		http_fatal("Ҫ����װ����?�Ǹ�,������վ�񵥶���������?");
	printf("<center>%s -- ���� T Shirt [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	printf("<table><td>");
	strncpy(t.id, currentuser->userid, IDLEN + 1);
	do_ts(&t, index);
	printf
	    ("��Ҫ�� %s %s %s %s %s ��վ�� %d ������������ϵ��ʽ��:%s",
	     stylestr[t.style], fmstr[t.fm], colorstr[t.color],
	     sizestr[t.size], pricestr[t.price], t.num, t.address);
	printf("</td></table>");
	printf("<br>[<a href='bbst'>�鿴��������</a>]");
	http_quit();
	return 0;
}

static int
do_ts(struct tshirt *t, int index)
{
	FILE *fp;
	struct tshirt tmp;
	int ret;
	fp = fopen("tshirt", "r+");
	if (fp == 0)
		http_fatal("����Ĳ���2");
	flock(fileno(fp), LOCK_EX);
	if (index < 0)
		ret = fseek(fp, 0, SEEK_END);
	else {
		ret = fseek(fp, sizeof (struct tshirt) * index, SEEK_SET);
		if (ret) {
			fclose(fp);
			http_fatal("�ڲ�����1");
		}
		fread(&tmp, sizeof (struct tshirt), 1, fp);
		if (strcmp(tmp.id, currentuser->userid)) {
			fclose(fp);
			http_fatal("�ڲ�����2");
		}
		ret = fseek(fp, sizeof (struct tshirt) * index, SEEK_SET);
	}
	if (ret) {
		fclose(fp);
		http_fatal("�ڲ�����!");
	}
	fwrite(t, sizeof (struct tshirt), 1, fp);
	fclose(fp);
	return 0;
}
