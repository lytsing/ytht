#include "bbslib.h"
#include "tshirt.h"

char *stylestr[3] = { "�ɻ�ͼ��", "��ͼ��", "��Ϳ����" };
char *fmstr[3] = { "�п�", "Ů��һ����", "Ů��Բ��" };
char *sizestr[5] = { "S��", "M��", "L��", "XL��", "XXL��" };
char *pricestr[2] = { "20Ԫ��(�͵�)", "40Ԫ��(�ߵ�)" };
char *colorstr[7] = { "��", "����", "���", "����", "ˮ��", "�ۺ�", "��" };

void
print_radio(char *cname, char *name, char *str[], int len, int select)
{
	int i;
	puts("<td valign=top>\n");
	printf("<p><p>%s��<br>", cname);
	for (i = 0; i < len; i++) {
		printf("<input type=radio name=%s value=%d %s>", name, i + 1,
		       (i == select) ? "checked" : "");
		printf("%s<br>\n", str[i]);
	}
	puts("</td>\n");
}

int
bbslt_main()
{
	int index;
	int i;
	struct tshirt t;
	html_header(1);
	if (!loginok || isguest) {
		printf("�Ҵҹ��Ͳ��ܶ���T Shirt,���ȵ�¼!<br><br>");
		printf("<script>openlog();</script>");
		http_quit();
	}
	changemode(SELECT);
	if (1 /*localtime(&now_t)->tm_hour>=14 */ ) {
		printf("�Ѿ�������!");
		printf("<br>[<a href='bbst'>�鿴��������</a>]");
		return 0;
	}
	index = atoi(getparm("index")) - 1;
	bzero(&t, sizeof (t));
	if (index >= 0) {
		FILE *fp;
		int ret;
		fp = fopen("tshirt", "r");
		if (fp == 0)
			http_fatal("����Ĳ���2");
		flock(fileno(fp), LOCK_SH);
		ret = fseek(fp, sizeof (struct tshirt) * index, SEEK_SET);
		if (ret) {
			fclose(fp);
			http_fatal("�ڲ�����1");
		}
		fread(&t, sizeof (struct tshirt), 1, fp);
		fclose(fp);
		if (strcmp(t.id, currentuser->userid))
			bzero(&t, sizeof (t));
	}

	if (currentuser->firstlogin >= 1055509320) {
		printf("������ȥ̫�����˵�...");
		http_quit();
	}
	printf("<center>%s -- ���� T Shirt [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	printf("<table width=70%%><tr>\n");
	printf
	    ("<td colspan=3><form name=form1 method=post action=bbsdt><p>����ѡ�����ͣ�</p></td><tr><tr><td><table width=75%% border=0 cellspacing=0 cellpadding=0 align=top><tr><td><input type=hidden name=index value=%d></td></tr><tr>\n",
	     index + 1);
	print_radio("ͼ��ѡ��", "style", stylestr, 3, t.style);
	print_radio("��������", "price", pricestr, 2, t.price);
	print_radio("��ʽѡ��", "fm", fmstr, 3, t.fm);
	print_radio("��Сѡ��", "size", sizestr, 5, t.size);
	print_radio("��ɫѡ��", "color", colorstr, 7, t.color);
	printf
	    ("</tr></table><p>������<input type=text name=num size=10 maxlength=3 value=%d>�������ͺŵġ�<br>������ϵ��ʽ����ַ���绰����<input type=text name=address size=40 maxlength=250 value='%s'><br><input type=submit name=Submit value=�ύ> <input type=reset name=Submit2 value=������></p></form>",
	     t.num, t.address);

	printf("</td></tr></table>");
	printf
	    ("<table><tr><td><font class=c32>����ͼ����˵��"
	     "�μ�<a href=con?B=Painter&F=M.1055771238.A target=_blank>��ƪ����</a><br>\n"
	     "������ɫ��ʽ��С��˵�� �μ�<a href=/contact/index.htm target=_blank>����<a><br>");
	printf("Ч��������������<br></font>");
	for (i = 0; i < 5; i++) {
		printf("<a href=/tshirt/%d.jpg target=_blank>Ч�� %d</a><br>\n",
		       i + 1, i + 1);
	}
	printf("��Ҫע����� �������е���϶����Ե�,����������<br>\n"
	       "20Ԫ��ֻ�а�ɫ û��Ů��<br>40Ԫ�� �п��� �� ���� ��� ���� ��ɫ,Ů��һ������ˮ�� ��� �� Ů��Բ���� ���� �ۺ� �� ��<br>��С�������ο�ǰ���˵��<br></td></tr>");
	printf
	    ("<tr><td><font color=red>������д����ϵ��ʽ�����ṫ��,ֻ���ں�����ϵT Shirt��������,������ѡ���ʱ��Ч��,��BBS����ϵ��ʽ.</font></td></tr><tr><td><font color=red>���ǽ�������ٶ�֪ͨ���ʱ�ںε���ȡTshirt.�Է���֮����7����δ����ȡ(�����������֪ͨվ����)��,����Ϊ�����Լ��ı���.</font></td></tr></table>");
	printf("<br>[<a href='bbst'>�鿴��������</a>]");
	http_quit();
	return 0;
}
