#include <dirent.h>
int versionsort(const void *a, const void *b);
#include "bbslib.h"

const static char *field[] =
    { "usernum", "userid", "realname", "dept", "addr", "phone", "assoc",
	"rereg",
	NULL
};
const static char *finfo[] =
    { "�ʺ�λ��", "�����ʺ�", "��ʵ����", "ѧУϵ��", "Ŀǰסַ",
	"����绰", "��ҵѧУ", "���״�ע��", NULL
};

#define NUMREASON 7
const static char *reason[] = { "��������д����������ʵ������",
	"��������дѧУϵ���������š�",
	"��������д������ס��ַ��",
	"���϶���ϸ���������д��",
	"��д���д���ʲ�������ϸ",
	"�����email������Ϊ��Ч��ַ��",
	"������ƴ����Ӣ����дע�ᵥ��"
};

static int
countnumforms(char *filename)
{
	int i = 0;
	char buf[256];
	FILE *fp;
	int lockfd;
	lockfd = openlockfile(".lock_new_register", O_RDONLY, LOCK_EX);
	fp = fopen(filename, "r");
	if (!fp) {
		close(lockfd);
		return 0;
	}
	while (fgets(buf, sizeof (buf), fp)) {
		if (!strncmp(buf, "usernum:", 8))
			i++;
	}
	fclose(fp);
	close(lockfd);
	return i;
}

static void
scanreg_info()
{
	int i, n;
	char buf[256];
	struct dirent **namelist;
	n = countnumforms("new_register");
	printf("Total number of forms: %d<br>", n);
	printf("<a href=bbsscanreg?STEP=1&N=20>Get 20 forms</a><br>");
	printf("<a href=bbsscanreg?STEP=1&N=30>Get 30 forms</a><br>");
	printf("<a href=bbsscanreg?STEP=1&N=40>Get 40 forms</a><br>");
	printf("<a href=bbsscanreg?STEP=1&N=60>Get 60 forms</a><br>");
	printf("Pick up list<br>");
	i = scandir(SCANREGDIR, &namelist, 0, versionsort);
	if (i <= 0) {
		if (!file_exist(SCANREGDIR))
			mkdir(SCANREGDIR, 0770);
		printf("No file for picking up<br>");
		return;
	}
	while (i--) {
		if (!strncmp(namelist[i]->d_name, "R.", 2)) {
			printf("<li><a href=bbsscanreg?STEP=1&F=%s>%s</a>",
			       namelist[i]->d_name, namelist[i]->d_name);
			sprintf(buf, "%s/%s", SCANREGDIR, namelist[i]->d_name);
			printf(" n=%d", countnumforms(buf));
			printf(" %s", Ctime(atoi(namelist[i]->d_name + 2)));
		}
		free(namelist[i]);
	}
	free(namelist);
}

static void
scanreg_readforms()
{
	FILE *fp;
	int n, count = 0;
	char buf[256], *ptr;
	char filename[256];
	char fdata[8][STRLEN];
	struct userec *urec;
	struct userdata udata;
	struct in_addr in;
	struct REGINFO ri;
	char errmsg[100];
	int unum;
	int rereg;
	ptr = getparm("F");
	if (*ptr) {
		if (strstr(ptr, "..") || strchr(ptr, '/')
		    || strncmp(ptr, "R.", 2))
			http_fatal("Wrong parameter!");
		snprintf(filename, sizeof (filename), SCANREGDIR "/%s", ptr);
		if (!file_exist(filename))
			http_fatal("No such file!");
		count = countnumforms(filename);
		if (!count) {
			unlink(filename);
			http_fatal("No form in this list (%s)", ptr);
		}
	} else {
		count = countnumforms(NEWREGFILE);
		if (!count)
			http_fatal("No new form");
		n = atoi(getparm("N"));
		if (n < 10 || n > 60)
			n = 20;
		if (getregforms(filename, n, currentuser->userid) < 0)
			http_fatal("failed to get reg forms!");
	}
	count = countnumforms(filename);
	if (!count)
		http_fatal("No forms in this list");
	count = 0;
	printf("<center><form action=bbsscanreg name=regformlist method=post>");
	printf("<input type=hidden name=STEP value='2'>");
	printf("<input type=hidden name=F value=%s>",
	       strrchr(filename, '/') + 1);
	printf("<table border=1>");
	fp = fopen(filename, "r");
	while (fgets(buf, STRLEN, fp) != NULL) {
		if ((ptr = strstr(buf, ": ")) != NULL) {
			*ptr = '\0';
			for (n = 0; field[n] != NULL; n++) {
				if (strcmp(buf, field[n]) != 0)
					continue;
				strsncpy(fdata[n], ptr + 2, sizeof (fdata[n]));
				if ((ptr = strchr(fdata[n], '\n')) != NULL)
					*ptr = '\0';
			}
			continue;
		}
		if ((unum = getuser(fdata[1], &urec)) <= 0
		    || (atoi(fdata[0]) != unum)) {
			urec = NULL;
			printf("ϵͳ����, ���޴��ʺ�.%d\n\n", unum);
			for (n = 0; field[n] != NULL; n++)
				printf("%s     : %s\n", finfo[n],
				       nohtml(void1(fdata[n])));
			memset(fdata, 0, sizeof (fdata));
			continue;

		}
		strsncpy(ri.userid, urec->userid, 100);
		strsncpy(ri.realname, fdata[2], 100);
		strsncpy(ri.career, fdata[3], 100);
		strsncpy(ri.addr, fdata[4], 100);
		rereg = atoi(fdata[7]);
		switch (rereg ? (errmsg[0] = 0, 1) : checkreg(&ri, errmsg)) {
		case 0:
			if (!(urec->userlevel & PERM_LOGINOK)) {
				printf
				    ("<input type=hidden name=userid%d value='%s'>",
				     count, urec->userid);
				printf
				    ("<input type=hidden name=result%d value=APASS>",
				     count);
				count++;
			}
			break;
		case 2:
			if (!(urec->userlevel & PERM_LOGINOK)) {
				printf
				    ("<input type=hidden name=userid%d value='%s'>",
				     count, urec->userid);
				printf
				    ("<input type=hidden name=apsofresult%d value='%s'>",
				     count, nohtml(errmsg));
				count++;
				errlog("scanreg refuse reason: %s", errmsg);
			}
			break;
		case 1:
		default:
			loaduserdata(fdata[1], &udata);
			printf("<tr><td width=30><nobr>");
			printf("�ʺ�λ��: %s<br>", fdata[0]);
			printf
			    ("&nbsp; &nbsp; ����: <font class=blu>%s</font><br>",
			     urec->userid);
			printf("��ʵ����: <font class=blu>%s</font><br>",
			       fdata[2]);
			printf("��ססַ: <font class=blu>%s</font><br>",
			       fdata[4]);
			printf("ѧУϵ��: <font class=blu>%s</font><br>",
			       fdata[3]);
			printf("��������: %s<br>", udata.email);
			printf("����绰: %s<br>", fdata[5]);
			printf("��ҵѧУ: %s<br>", fdata[6]);
			printf("�ʺŽ�������: %s<br>", Ctime(urec->firstlogin));
			printf("�����������: %s<br>", Ctime(urec->lastlogin));
			in.s_addr = urec->lasthost;
			printf
			    ("������ٻ���: <a href=/cgi-bin/cgifreeip?type=1&ip=%s>%s</a>",
			     inet_ntoa(in), inet_ntoa(in));
			printf("</td><td valign=top>");
			if (urec->userlevel & PERM_LOGINOK) {
				printf("���ʺŲ�������дע�ᵥ.");
			} else {
				if (!rereg)
					errlog("scanreg recheck reason: %s", errmsg);
				printf("%s<br>", errmsg);
				printf
				    ("<input type=hidden name=userid%d value='%s'>",
				     count, urec->userid);
				printf
				    ("<input type=radio name=result%d value=PASS checked>PASS",
				     count);
				printf
				    (" <input type=radio name=result%d value=SKIP>SKIP",
				     count);
				printf
				    (" <input type=radio name=result%d value=DEL>DEL",
				     count);
				printf
				    ("&nbsp;&nbsp;&nbsp;&nbsp;<input type=button value=������ onclick=\"javascript:window.open('bbsscanreg_findsurname', 'findsurname', 'toolbar=0, scrollbars=1, location=0, statusbar=1, menubar=0, resizable=1, width=320, height=100');\"><br>\n");
				for (n = 0; n < NUMREASON; n++) {
					printf
					    ("<input type=radio name=result%d value=%d>%s<br>",
					     count, n, nohtml(reason[n]));
				}
				printf
				    ("�����ܾ����ɣ�������40�֣�<input type=text name=psofresult%d ><br>",
				     count);
				count++;
			}
			break;
		}
		printf("</td></tr>");
		memset(fdata, 0, sizeof (fdata));
	}
	printf
	    (" </table><input type=submit onclick=\"javascript:{return ! confirm('��ʵ�һ�����ݽ�,��ֻ�ǰ�����!');}\" value=submit></form>");
	fclose(fp);
}

static void
scanreg_done()
{
	const char *blk[] = {
		"��", "��", "��", "��", "��", "��", "��", "��", " "
	};

	const char *(results[60]), *(userids[60]), *(ps[60]);
	char fdata[8][STRLEN];
	char buf[256], filename[256], dellog[STRLEN * 8], logname[STRLEN], *ptr;
	int nresult = 0, i, n, lockfd, unum, percent, j, loglength;
	int npass = 0, nskip = 0, ndel = 0, nreject = 0, napass = 0, nareject =
	    0;
	FILE *fp, *scanreglog;
	struct userec *urec;
	struct userec tmpu;
	struct userdata udata;
	time_t dtime;
	struct tm *t;
	for (i = 0; i < 60; i++) {
		sprintf(buf, "apsofresult%d", i);
		ps[nresult] = getparm(buf);
		if (ps[nresult][0] && strlen(ps[nresult]) < STRLEN) {
			results[nresult] = ps[nresult];
			nareject++;
		} else {
			sprintf(buf, "psofresult%d", i);
			ps[nresult] = getparm(buf);
			if (strlen(ps[nresult]) > 0
			    && strlen(ps[nresult]) < STRLEN) {
				results[nresult] = ps[nresult];
			} else {
				sprintf(buf, "result%d", i);
				results[nresult] = getparm(buf);
				if (!*results[nresult])
					continue;
				if (isdigit(*results[nresult])) {
					n = atoi(results[nresult]);
					if (n < 0 || n >= NUMREASON)
						continue;
					results[nresult] = reason[n];
				}
			}
		}
		sprintf(buf, "userid%d", i);
		userids[nresult] = getparm(buf);
		if (!*userids[nresult])
			continue;
		nresult++;
	}
	sprintf(logname, "bbstmpfs/tmp/wwwscanreglog.%d.%s", thispid,
		currentuser->userid);
	if ((scanreglog = fopen(logname, "w")) != NULL) {
		fprintf(scanreglog, "ɾ������������\n");
	} else {
		errlog("open scanreg tmpfile error.");
	}
	for (i = 0; i < nresult; i++) {
		printf("%s %s<br>", results[i], userids[i]);
		if ((!strcmp(results[i], "SKIP"))
		    || (!strcmp(results[i], "DEL")))
			fprintf(scanreglog, "%s %s\n", results[i], userids[i]);
	}
	ptr = getparm("F");
	if (strstr(ptr, "..") || strchr(ptr, '/') || strncmp(ptr, "R.", 2))
		http_fatal("Wrong parameter!");
	snprintf(filename, sizeof (filename), SCANREGDIR "/%s", ptr);

	fp = fopen(filename, "r");
	if (!fp)
		http_fatal("File Not Found!");
	lockfd = openlockfile(".lock_new_register", O_RDONLY, LOCK_EX);
	while (fgets(buf, STRLEN, fp) != NULL) {
		if ((ptr = strstr(buf, ": ")) != NULL) {
			*ptr = '\0';
			for (i = 0; field[i] != NULL; i++) {
				if (strcmp(buf, field[i]) != 0)
					continue;
				strsncpy(fdata[i], ptr + 2, sizeof (fdata[i]));
				if ((ptr = strchr(fdata[i], '\n')) != NULL)
					*ptr = '\0';
			}
			continue;
		}
		if ((unum = getuser(fdata[1], &urec)) <= 0
		    || (atoi(fdata[0]) != unum)) {
			bzero(fdata, sizeof (fdata));
			continue;
		}
		loaduserdata(fdata[1], &udata);
		if (urec->userlevel & PERM_LOGINOK)
			continue;
		for (n = 0; n < nresult; n++) {
			if (!strcmp(userids[n], urec->userid))
				break;
		}
		if (n >= nresult || !strcmp(results[n], "SKIP")) {
			FILE *fp;
			char filename[256], buf[256];
			int t;
			strcpy(filename, SCANREGDIR);
			sprintf(buf, "R.%%d.SKIP.%s", fdata[1]);
			t = trycreatefile(filename, buf, time(NULL), 5);
			if (t >= 0)
				if ((fp = fopen(filename, "w")) != NULL) {
					for (n = 0; field[n] != NULL; n++) {
						fprintf(fp, "%s: %s\n",
							field[n], fdata[n]);
					}
					fprintf(fp, "----\n");
					fclose(fp);
				}
			bzero(fdata, sizeof (fdata));
			nskip++;
			continue;
		}
		if (!strcmp(results[n], "DEL")) {
			FILE *fp;
			char buf2[STRLEN];
			sprintf(dellog, "ע����Ϣ����\n");
			loglength = 0;
			for (i = 0; i < 8; i++) {
				if (loglength >= sizeof (dellog))
					break;
				loglength +=
				    snprintf(dellog + loglength,
					     sizeof (dellog) - loglength,
					     "%s:%s\n", finfo[i], fdata[i]);
			}
			ndel++;
			bzero(fdata, sizeof (fdata));
			memcpy(&tmpu, urec, sizeof (struct userec));
			tmpu.userlevel &= ~PERM_DEFAULT;
			updateuserec(&tmpu, 0);
			sprintf(buf, "%s ɾ�� %s ��ע�ᵥ", currentuser->userid,
				urec->userid);
			securityreport(buf, dellog);
			sprintf(buf2, "bbstmpfs/tmp/scanregdellog.%d.%s",
				thispid, currentuser->userid);
			if ((fp = fopen(buf2, "w")) != NULL) {
				fprintf(fp, dellog);
				fclose(fp);
			}
			post_article("IDScanRecord", buf, buf2,
				     currentuser->userid, currentuser->username,
				     fromhost, -1, 0, 0, currentuser->userid,
				     -1);
			unlink(buf2);
			continue;
		}
		if (!strcmp(results[n], "APASS")) {
			FILE *fout;
			char buf2[256];
			i = strlen(fdata[5]);
			if (i + strlen(fdata[3]) > 60) {
				if (i > 40)
					fdata[5][i = 40] = '\0';
				fdata[3][60 - i] = '\0';
			}
			strsncpy(udata.realname, fdata[2],
				 sizeof (udata.realname));
			strsncpy(udata.address, fdata[4],
				 sizeof (udata.address));
			sprintf(buf, "%s$%s@�Զ�����ϵͳ", fdata[3], fdata[5]);
			strsncpy(udata.realmail, buf, sizeof (udata.realmail));
			memcpy(&tmpu, urec, sizeof (struct userec));
			tmpu.userlevel |= PERM_DEFAULT;	// by ylsdd
			updateuserec(&tmpu, 0);
			saveuserdata(urec->userid, &udata);
			sethomefile(buf, urec->userid, "sucessreg");
			f_write(buf, "\n");
			sethomefile(buf, urec->userid, "register");
			if (file_exist(buf)) {
				sethomefile(buf2, urec->userid, "register.old");
				rename(buf, buf2);
			}
			if ((fout = fopen(buf, "w")) != NULL) {
				for (i = 0; field[i] != NULL; i++)
					fprintf(fout, "%s: %s\n", field[i],
						fdata[i]);
				fprintf(fout, "Date: %s\n", Ctime(time(NULL)));
				fprintf(fout, "Approved: �Զ�����ϵͳ\n");
				fclose(fout);
			}
			mail_file("etc/s_fill", urec->userid,
				  "������ͨ�������֤", "SYSOP");
			mail_file("etc/s_fill2", urec->userid,
				  "��ӭ����" MY_BBS_NAME "���ͥ", "SYSOP");
			sethomefile(buf, urec->userid, "mailcheck");
			unlink(buf);
			sprintf(buf, "�� %s ͨ�����ȷ��(�Զ�).", urec->userid);
			napass++;
			securityreport(buf, buf);
			continue;
		}
		if (!strcmp(results[n], "PASS")) {
			FILE *fout;
			char buf2[256];
			i = strlen(fdata[5]);
			if (i + strlen(fdata[3]) > 60) {
				if (i > 40)
					fdata[5][i = 40] = '\0';
				fdata[3][60 - i] = '\0';
			}
			strsncpy(udata.realname, fdata[2],
				 sizeof (udata.realname));
			strsncpy(udata.address, fdata[4],
				 sizeof (udata.address));
			sprintf(buf, "%s$%s@%s", fdata[3], fdata[5],
				currentuser->userid);
			strsncpy(udata.realmail, buf, sizeof (udata.realmail));
			memcpy(&tmpu, urec, sizeof (struct userec));
			tmpu.userlevel |= PERM_DEFAULT;	// by ylsdd
			updateuserec(&tmpu, 0);
			saveuserdata(urec->userid, &udata);
			sethomefile(buf, urec->userid, "sucessreg");
			f_write(buf, "\n");
			sethomefile(buf, urec->userid, "register");
			if (file_exist(buf)) {
				sethomefile(buf2, urec->userid, "register.old");
				rename(buf, buf2);
			}
			if ((fout = fopen(buf, "w")) != NULL) {
				for (i = 0; field[i] != NULL; i++)
					fprintf(fout, "%s: %s\n", field[i],
						fdata[i]);
				fprintf(fout, "Date: %s\n", Ctime(time(NULL)));
				fprintf(fout, "Approved: %s\n",
					currentuser->userid);
				fclose(fout);
			}
			system_mail_file("etc/s_fill", urec->userid,
				  "������ͨ�������֤", currentuser->userid);
			system_mail_file("etc/s_fill2", urec->userid,
				  "��ӭ����" MY_BBS_NAME "���ͥ",
				  currentuser->userid);
			sethomefile(buf, urec->userid, "mailcheck");
			unlink(buf);
			sprintf(buf, "�� %s ͨ�����ȷ��.", urec->userid);
			npass++;
			securityreport(buf, buf);
		} else {
			snprintf(buf, sizeof (buf), "<ע��ʧ��>%s", results[n]);
			system_mail_file("etc/f_fill", urec->userid, buf,
				  currentuser->userid);
			nreject++;
		}
		bzero(fdata, sizeof (fdata));
	}
	nreject = nreject - nareject;
	if (scanreglog != NULL) {
		fprintf(scanreglog, "\n������ %3d ��ע�ᵥ\n\n", nresult);
		nresult = nresult - napass - nareject;
		fprintf(scanreglog, "ϵͳ�Զ����� %3d ��\n", napass + nareject);
		fprintf(scanreglog, "�Զ�ͨ�� %3d �����˻� %3d ��\n", napass,
			nareject);
		fprintf(scanreglog, "\033[32mͨ��\033[0m %3d�� Լռ", npass);
		percent = (npass * 100) / ((nresult <= 0) ? 1 : nresult);
		for (j = percent / 16; j > 0; j--)
			fprintf(scanreglog, "%s", blk[0]);
		fprintf(scanreglog, "%s%d%%\n", blk[8 - (percent % 16) / 2],
			percent);
		fprintf(scanreglog, "\033[33m�˻�\033[0m %3d�� Լռ", nreject);
		percent = (nreject * 100) / ((nresult <= 0) ? 1 : nresult);
		for (j = percent / 16; j > 0; j--)
			fprintf(scanreglog, "%s", blk[0]);
		fprintf(scanreglog, "%s%d%%\n", blk[8 - (percent % 16) / 2],
			percent);
		fprintf(scanreglog, "���� %3d�� Լռ", nskip);
		percent = (nskip * 100) / ((nresult <= 0) ? 1 : nresult);
		for (j = percent / 16; j > 0; j--)
			fprintf(scanreglog, "%s", blk[0]);
		fprintf(scanreglog, "%s%d%%\n", blk[8 - (percent % 16) / 2],
			percent);
		fprintf(scanreglog, "\033[31mɾ��\033[0m %3d�� Լռ", ndel);
		percent = (ndel * 100) / ((nresult <= 0) ? 1 : nresult);
		for (j = percent / 16; j > 0; j--)
			fprintf(scanreglog, "%s", blk[0]);
		fprintf(scanreglog, "%s%d%%\n", blk[8 - (percent % 16) / 2],
			percent);
		fclose(scanreglog);
		time(&dtime);
		t = localtime(&dtime);
		sprintf(buf, "WWW �ʺ�������¼-%d-%02d-%02d-%02d:%02d:%02d",
			1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
		post_article("IDScanRecord", buf, logname, currentuser->userid,
			     currentuser->username, fromhost, -1, 0, 0,
			     currentuser->userid, -1);
		unlink(logname);
		tracelog("%s pass %d %d %d %d", currentuser->userid, npass,
			 nreject, nskip, ndel);
	}
	unlink(filename);
	fclose(fp);
	close(lockfd);
}

int
bbsscanreg_main()
{
	html_header(1);
	check_msg();
	printf("<body>");
	if (!loginok || isguest || !USERPERM(currentuser, PERM_ACCOUNTS))
		http_fatal("unknown request");
	printf("<nobr><center>%s -- scan reg form</center><hr>\n", BBSNAME);
	switch (atoi(getparm("STEP"))) {
	case 1:
		scanreg_readforms();
		break;
	case 2:
		scanreg_done();
		printf("<a href=bbsscanreg>Continue</a>");
		break;
	case 0:
	default:
		scanreg_info();
	}
	http_quit();
	return 0;
}
