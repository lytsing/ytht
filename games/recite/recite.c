//ecnegrevid 2001.7.20
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/param.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <newt.h>
#include <stdlib.h>
#include <sys/resource.h>
#define NBOOK 16
#define NMETHOD 3
int sec=5;
int booknum=0;
int methodnum=0;
int browse();
char *user;
#define LIBDIR "/home/bbs/etc/recite"
#define LOGDIR "/home/bbs/log/recite"
struct {
	char *fn;
	char *exp;
	int n;
	int nn;
} book[NBOOK]={
	"/home/bbs/etc/recite/barron.txt", "Barron�ʱ�", 3759,1,
	LIBDIR "/d/0B0", "1997��GRE�ʻ㾫ѡ", 0,1,
	LIBDIR "/d/0B1", "GRE�ʻ������ȫ", 0,1,
	LIBDIR "/d/0B2", "GRE�����", 0,1,
	LIBDIR "/d/0B3", "TOEFL�ʻ㾫ѡ", 0,1,
	LIBDIR "/d/0B4", "TOEFL���龫ѡ", 0,1,
	LIBDIR "/d/0B5", "GMAT�ʻ㾫ѡ", 0,1,
	LIBDIR "/d/1B0", "�ļ��ʻ�", 0,1,
	LIBDIR "/d/1B1", "�����ʻ�", 0,1,
	LIBDIR "/d/1B2", "��ѧӢ�����", 0,1,
	LIBDIR "/d/1B3", "�о�����ѧ���Դʻ���", 0,1,
	LIBDIR "/d/1B4", "�о�����ѧ���Գ��ô���", 0,1,
	LIBDIR "/d/2B0", "�¸���Ӣ���һ��(��������)", 0,1,
	LIBDIR "/d/2B1", "�¸���Ӣ��ڶ���(��������)", 0,1,
	LIBDIR "/d/2B2", "�¸���Ӣ�������(��������)", 0,1,
	LIBDIR "/d/2B3", "�¸���Ӣ����Ĳ�(��������)", 0,1
};

int selexp(), selword();
struct {
	char *des;
	int (*func)();
} method[NMETHOD]={
	"������              ", browse,
	"���飬���ݵ���ѡ�����", selexp,
	"���飬���ݽ���ѡ�񵥴�", selword,
};

FILE *fplib;
int isspec;
struct oneword {
	char word[50];
	char exp[100];
	char eg[300];
};

char genbuf[2048];
main(int argn, char **argv)
{
	int b, m;
	struct rlimit rl;

	rl.rlim_cur = 100 * 1024 * 1024;
	rl.rlim_max = 100 * 1024 * 1024;
	setrlimit(RLIMIT_CORE, &rl);

	printf("\033[H\033[2J\033[m");
	if(argn==4) {
		printf("����%s��%s����, ", argv[2], argv[1]);
		user=argv[1];
		if(loadtt(&sec)<0||sec<0||sec>200) sec=5;
	} else user="guest";
	printf("ʹ��cterm�������'c', �����ն˻س�����");
	fflush(stdout);
	if('c'==getchar())
		setenv("TERM", "ansi", 1);
	else
		setenv("TERM", "vt100", 1);
	fplib=fopen(LIBDIR "/d/SEA.LIB", "r");
	newtInit();
	newtCls();
	newtDrawRootText(0, 0, "�����Ǳ������ʰ�");
	while(1) {
		if(doquery(&booknum, &methodnum)<0) break;
		if(strstr(book[booknum].fn,".txt")==NULL) {
			struct stat s;
			stat(book[booknum].fn,&s);
			book[booknum].n=s.st_size/4;
			isspec=1;
		} else isspec=0;
		method[methodnum].func();
	}
	newtFinished();
}

char* question(char *des, char *initvalue)
{
	newtComponent form, label, entry;
	char * entryValue;
	static char value[1024];
	newtCenteredWindow(50, 9, NULL);
	label=newtLabel(1, 1, des);
	entry = newtEntry(2, 3, initvalue, 20, &entryValue, 
			NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);
	form = newtForm(NULL, NULL, 0);
	newtFormAddHotKey(form, '\r');
	newtFormAddHotKey(form, '\n');
	newtFormAddComponents(form, label, entry, NULL);
	newtRunForm(form);
	newtClearKeyBuffer();
	strncpy(value,entryValue, 1023);
	value[1023]=0;
	newtFormDestroy(form);
	newtPopWindow();
	return value;
}


int doquery(int *b, int *m)
{
	int i;
	struct newtExitStruct es;
	newtComponent form, rbb[20], rbm[20];
	newtCenteredWindow(60, 20, NULL);
	newtPushHelpLine("������ƶ����ո��ѡ�����س�ȷ����'q'�˳�");
	for(i=0;i<NBOOK&&i<20;i++) {
		rbb[i]=newtRadiobutton(1, i+1, book[i].exp, i==booknum, (i==0)?NULL:rbb[i-1]);
	}
	for(i=0;i<NMETHOD&&i<20;i++)
		rbm[i]=newtRadiobutton(31, i+1, method[i].des, i==methodnum, (i==0)?NULL:rbm[i-1]);
	form = newtForm(NULL, NULL, 0);
	newtFormAddHotKey(form, '\r');
	newtFormAddHotKey(form, '\n');
	newtFormAddHotKey(form, 'q');
	for(i=0;i<NBOOK&&i<20;i++)
		newtFormAddComponent(form,rbb[i]);
	for(i=0;i<NMETHOD&&i<20;i++)
		newtFormAddComponent(form,rbm[i]);
	newtFormRun(form,&es);
	newtClearKeyBuffer();
	for(i=0;i<NBOOK&&i<20;i++)
		if (newtRadioGetCurrent(rbb[0]) == rbb[i]) {
			*b=i;
			break;
		}
	for(i=0;i<NMETHOD&&i<20;i++)
		if (newtRadioGetCurrent(rbm[0]) == rbm[i]) {
			*m=i;
			break;
		}
	newtPopHelpLine();
	newtPopWindow();
	newtFormDestroy(form);
	if(es.reason==NEWT_EXIT_HOTKEY&&es.u.key=='q')
		return -1;
	return 0;
}

int seekto(FILE *fp, int i)
{
	if(isspec) {
		fseek(fp, (i-1)*4, SEEK_SET);
		return ftell(fp)/4+1;
	}
	fseek(fp, 0, SEEK_SET);
	i--;
	while(i) {
		fgets(genbuf, sizeof(genbuf), fp);
		fgets(genbuf, sizeof(genbuf), fp);
		i--;
	}
}

int readword(FILE*fp, struct oneword *w)
{
	char buf[1024],*p1, *p2;
	int i;
	if(isspec) {
		fread(&i, 4, 1, fp);
		fseek(fplib, i, SEEK_SET);
		i=fread(buf, 1, 1023, fplib);
		if(i<0) return -1;
		buf[i]=0;
		p1=buf;
		if(*p1=='$') p1++;
		p2=strchr(p1, '$');
		if(p2!=NULL) *p2=0;
		p2=strchr(p1,'[');
		if(p2!=NULL) {
			*p2=0;
			p2++;
		} else p2=p1;
		strcpy(w->word, p1);
		p1=strchr(p2,']');
		if(p1!=NULL) {
			*p1=0;
			p1++;
		} else p1=p2;
		strcpy(w->exp,p1);
		w->eg[0]=0;
		return 0;
	}
	
	if(fgets(buf, 1024, fp)==NULL) return -1;
	p1=strchr(buf, '\r');
	if(p1!=NULL) *p1=0;
	p1=strchr(buf, '\n');
	if(p1!=NULL) *p1=0;
	p1=strchr(buf, ' ');
	p1++;
	p2=strchr(p1, ' ');
	*p2=0;
	p2++;
	strncpy(w->word, p1, sizeof(w->word));
	w->word[sizeof(w->word)-1]=0;
	strncpy(w->exp, p2, sizeof(w->exp));
	w->exp[sizeof(w->exp)-1]=0;
	if(fgets(buf, 1024, fp)==NULL) return -1;
	p1=strchr(buf, '\r');
	if(p1!=NULL) *p1=0;
	p1=strchr(buf, '\n');
	if(p1!=NULL) *p1=0;
	strncpy(w->eg,buf,sizeof(w->eg));
	w->eg[sizeof(w->eg)-1]=0;
	return 0;
}

int browse()
{
	newtComponent form, label, labelword, textexplain, textsample;
	struct newtExitStruct es;
	FILE *fp;
	struct oneword w;
	char buf[1024], ch;
	int i;
	
	if(loadnn(&book[booknum].nn)<0||book[booknum].nn<=0)
		book[booknum].nn=1;
	newtCenteredWindow(76, 20, NULL);
	sprintf(genbuf, "%s ����%d�ʣ��ӵڼ�����ʼ?", book[booknum].exp, book[booknum].n);
	sprintf(buf, "%d", book[booknum].nn);
	i=atoi(question(genbuf,buf));
	if(i<=0) i=1;
	if(i>book[booknum].n) i=book[booknum].n;
	label=newtCompactButton(25, 1, "�������"); //һ��Form����Ҫ��һ����������Ķ���,�����pageup/downʱcoredump
	labelword=newtLabel(1, 3, "word");
	textexplain=newtTextbox(1,4, 75, 2, NEWT_FLAG_WRAP);
	textsample=newtTextbox(1,6, 75, 4, NEWT_FLAG_WRAP);
	form = newtForm(NULL, NULL, 0);
	newtFormAddHotKey(form, ' ');
	newtFormAddHotKey(form, '\n');
	newtFormAddHotKey(form, '\r');
	newtFormAddHotKey(form, 'q');
	newtFormAddHotKey(form, 'g');
	newtFormAddHotKey(form, 't');
	newtFormAddComponents(form, label, labelword, textexplain, textsample, NULL);
	newtPushHelpLine("������� �ո����һ�� 'q'�˳� 'g'��ת 't'�趨�Զ����ʱ��");
	fp=fopen(book[booknum].fn, "r");
	if(fp==NULL) {
		newtPopWindow();
		newtFormDestroy(form);
		return;
	}
	seekto(fp,i);
	while(1) {
		if(readword(fp, &w)<0||i>book[booknum].n) goto FINISHED;
		if(sec==0) sprintf(genbuf, "������� ��%d��", i);
		else sprintf(genbuf, "�Զ��������(���%d��) ��%d��", sec, i);
		newtLabelSetText(label,genbuf);
		newtLabelSetText(labelword,w.word);
		newtTextboxSetText(textexplain, w.exp);
		newtTextboxSetText(textsample, w.eg);
		newtFormSetTimer(form, sec*1000);
		newtFormRun(form, &es);
		newtClearKeyBuffer();
		if(es.reason==NEWT_EXIT_HOTKEY&&es.u.key=='q')
			break;
		if(es.reason==NEWT_EXIT_HOTKEY&&es.u.key=='g') {
			sprintf(genbuf, "%s ����%d�ʣ��ӵڼ�����ʼ?",
					book[booknum].exp, book[booknum].n);
			sprintf(buf, "%d", i);
			i=atoi(question(genbuf,buf));
			if(i<=0) i=1;
			seekto(fp,i);
			continue;
		}
		if(es.reason==NEWT_EXIT_HOTKEY&&es.u.key=='t') {
			sprintf(genbuf, "�趨�Զ�������, ����0���Զ����");
			sprintf(buf, "%d", sec);
			sec=atoi(question(genbuf,buf));
			if(sec<=0||sec>200) sec=0;
			savett(sec);
			continue;
		}
	
		i++;
	}
FINISHED:
	book[booknum].nn=i;
	savenn(book[booknum].nn);
	fclose(fp);
	newtPopHelpLine();
	newtPopWindow();
	newtFormDestroy(form);
}

int makerand(int w[], int n, int max)
{
	int i,j, l;
	for(i=0;i<n;i++) {
		l=rand()%max+1;
		for(j=0;j<i;j++)
			if(w[j]==l)
				break;
		if(i>j) {
			i--;
			continue;
		}
		w[i]=l;
	}
}

int testreport(char *test, int i, int n)
{
	newtComponent form, text;
	int score=i*100/n;
	time_t t=time(NULL);
	newtCenteredWindow(65, 12, NULL);
	sprintf(genbuf, "\n                       ���鱨��\n\n"
		 	"�װ���%s:\n"
			"    ��ι���<<%s>>��%s������, ��%d����������������%d��, "
			"���÷�Ϊ%d (����100)%s\n\n"
			"                                   ϡ���Ϳ������\n"
			"                                 %s",
			user, book[booknum].exp, test, n, i, score, 
		       	score==100?"! ����! ̫������! ���������ĸ���������?":
			score>=90?", �����ܸ�ѽ��ף����! ���ǲ�Ҫ����Ŷ, ע�Ᵽ�� :)":
			score>=80?", ���ʱ��Ļ�����, ��������~":
			score>=60?", ������, ��Ŭ��һ�����һЩ�� :)":
			". ������Ҫ��, ������, �����ʲ��ǳ�Ϧ֮��Ӵ.",
			ctime(&t));
	text=newtTextbox(3,1, 60, 11, NEWT_FLAG_WRAP);
	newtTextboxSetText(text, genbuf);
	form=newtForm(NULL,NULL,0);
	newtFormAddComponent(form,text);
	newtFormAddHotKey(form,'q');
	newtPushHelpLine("'q'����");
	newtRunForm(form);
	newtClearKeyBuffer();
	newtPopHelpLine();
	newtPopWindow();
	newtFormDestroy(form);
}

int doselect(char *des, char *word, char s[][100], int n)
{
	newtComponent form, label, labelword, rb[20];
	struct newtExitStruct es;
	int i;
	if(n>20) return n;
	des[40]=0;
	word[40]=0;
	label=newtLabel(10, 1, des);
	labelword=newtLabel(2, 3, word);
	for(i=0;i<n;i++) {
		s[i][70]=0;
		rb[i]=newtRadiobutton(1, i+4, s[i], 0, (i==0)?NULL:rb[i-1]);
	}
	form=newtForm(NULL,NULL,0);
	newtFormAddHotKey(form, 'q');
	newtFormAddHotKey(form, '\r');
	newtFormAddHotKey(form, '\n');
	newtFormAddComponents(form, label, labelword, NULL);
	for(i=0;i<n;i++)
		newtFormAddComponent(form, rb[i]);
	newtPushHelpLine("������ƶ����ո��ѡ�����س�ȷ����'q'�˳�");
	newtFormRun(form, &es);
	newtClearKeyBuffer();
	newtPopHelpLine();
	if(es.reason==NEWT_EXIT_HOTKEY&&es.u.key=='q')
		i=-1;
	else for(i=0;i<n;i++)
		if (newtRadioGetCurrent(rb[0]) == rb[i]) {
			break;
		}
	newtFormDestroy(form);
	return i;
}

#define MAXTEST 150
int selexp()
{
	int wl[MAXTEST], it[4], c[MAXTEST], cr, i, j, k, k0, ntest;
	char des[100], s[4][100], word[50];
	FILE *fp;
	struct oneword w0;
	bzero(c,sizeof(c));
	fp=fopen(book[booknum].fn, "r");
	if(fp==NULL) {
		return;
	}
	ntest=atoi(question("�����ٵ���? (����10����,���150����)","50"));
	if(ntest<10) ntest=10;
	else if(ntest>150) ntest=150;	
	newtCenteredWindow(76, 20, NULL);
	srand(getpid()+time(NULL));
	makerand(wl, ntest, book[booknum].n);
	for(i=0,cr=0;i<ntest;i++) {
		makerand(it, 4, book[booknum].n);
		k0=rand()%4;
		it[k0]=wl[i];
		for(j=0;j<4;j++) {
			seekto(fp, it[j]);
			readword(fp, &w0);
			strcpy(s[j], w0.exp);
			memset(s[j]+strlen(s[j]),' ', sizeof(s[j])-strlen(s[j]));
			s[j][sizeof(s[j])-1]=0;
			if(j==k0)
				strcpy(word, w0.word);
		}
		sprintf(des, "ѡ����ȷ�Ľ��� ��%d�� (��%d������%d��)", i+1, ntest, cr);
		k=doselect(des, word, s, 4);
		if(k==k0) {
			c[i]=1;
			cr++;
		}
		if(k==-1) {
			newtPopWindow();
			return;
		}
	}
	testreport("ѡ�񵥴ʵ���ȷ����",cr,ntest);
	newtPopWindow();
}

int selword()
{
	int wl[MAXTEST], it[4], c[MAXTEST], cr, i, j, k, k0, ntest;
	char des[100], s[4][100], exp[100];
	FILE *fp;
	struct oneword w0;
	bzero(c,sizeof(c));
	fp=fopen(book[booknum].fn, "r");
	if(fp==NULL) {
		return;
	}
	ntest=atoi(question("�����ٵ���? (����10����,���150����)","50"));
	if(ntest<10) ntest=10;
	else if(ntest>150) ntest=150;
	newtCenteredWindow(76, 20, NULL);
	srand(getpid()+time(NULL));
	makerand(wl, ntest, book[booknum].n);
	for(i=0,cr=0;i<ntest;i++) {
		makerand(it, 4, book[booknum].n);
		k0=rand()%4;
		it[k0]=wl[i];
		for(j=0;j<4;j++) {
			seekto(fp, it[j]);
			readword(fp, &w0);
			strcpy(s[j], w0.word);
			memset(s[j]+strlen(s[j]),' ', sizeof(s[j])-strlen(s[j]));
			s[j][sizeof(s[j])-1]=0;
			if(j==k0)
				strcpy(exp, w0.exp);
		}
		sprintf(des, "ѡ����ϵĵ��� ��%d�� (��%d������%d��)", i+1, ntest, cr);
		k=doselect(des, exp, s, 4);
		if(k==k0) {
			c[i]=1;
			cr++;
		}
		if(k==-1) {
			newtPopWindow();
			return;
		}
	}
	testreport("���ݺ���ѡ����",cr,ntest);
	newtPopWindow();
}

int loadtt(int *tt)
{
	char buf[MAXPATHLEN];
	sprintf(buf, LOGDIR "/nn/%s", user);
	return readvaluef(buf, 50, "t", tt);
}

int savett(int tt)
{
	char buf[MAXPATHLEN];
	sprintf(buf, LOGDIR "/nn/%s", user);
	return additemf(buf, 50, "t", tt);
}
int loadnn(int *nn)
{
	char buf[MAXPATHLEN];
	sprintf(buf, LOGDIR "/nn/%s", user);
	return readvaluef(buf, 50, strrchr(book[booknum].fn,'/')+1, nn);
}

int savenn(int nn)
{
	char buf[MAXPATHLEN];
	sprintf(buf, LOGDIR "/nn/%s", user);
	return additemf(buf, 50, strrchr(book[booknum].fn,'/')+1, nn);
} 
