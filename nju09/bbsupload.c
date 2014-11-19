#include <sys/resource.h>
#include <sys/time.h>
#include <dirent.h>
#include "bbslib.h"

static int
fixfilename(char *str)
{
	if (!str[0] || !strcmp(str, ".") || !strcmp(str, ".."))
		return -1;
	while (*str) {
		if ((*str > 0 && *str < ' ') || isspace(*str)
		    || strchr("\\/~`!@#$%^&*()|{}[];:\"'<>,?", *str)) {
			*str = '_';
		}
		str++;
	}
	return 0;
}

static int
upload_getpathsize(char *path, int showlist)
{
	DIR *pdir;
	struct dirent *pdent;
	char fname[1024];
	int totalsize = 0, size;
	if (showlist)
		printf("�Ѿ����صĸ�����:<br>");
	pdir = opendir(path);
	if (!pdir)
		return -1;
	while ((pdent = readdir(pdir))) {
		if (!strcmp(pdent->d_name, "..") || !strcmp(pdent->d_name, "."))
			continue;
		if (strlen(pdent->d_name) + strlen(path) >= sizeof (fname)) {
			totalsize = -1;
			break;
		}
		sprintf(fname, "%s/%s", path, pdent->d_name);
		size = file_size(fname);
		if (showlist) {
			printf("<li> %s (<i>%d�ֽ�</i>) ", pdent->d_name, size);
			printf("<a href=\"bbsupload?del=%s\">ɾ��</a>",
			       nohtml(pdent->d_name));
		}
		if (size < 0) {
			totalsize = -1;
			break;
		}
		totalsize += size;
	}
	closedir(pdir);
	if (showlist) {
		printf("<br>�Ѿ����� %d �ֽ� (��� %d �ֽ�)<br>", totalsize,
		       MAXATTACHSIZE);
	}
	return totalsize;
}

int
save_attach(char *path, int totalsize)
{
	char *ptr, filename[100], filepath[256], pname[16];
	FILE *fp;
	int i;
	struct parm_file *parmFile;
	for (i = 0; i < 10; i++) {
		sprintf(pname, "file%d", i);
		parmFile = getparmfile(pname);
		if (!parmFile)
			continue;
		//errlog("%s %d", parmFile->filename, parmFile->len);
		strsncpy(filename, parmFile->filename, sizeof (filename));
		if (strlen(filename) >= 35) {
			ptr = strrchr(filename, '.');
			if (ptr) {
				int len = strlen(ptr);
				if (len > 6) {
					ptr[6] = 0;
					len = 6;
				}
				memmove(filename + 35 - len, ptr, len + 1);
			} else
				filename[35] = 0;
		}

		if (fixfilename(filename)) {
			printf("��Ч���ļ�����%s<br>", nohtml(void1(filename)));
			continue;
		}
		if (parmFile->len + totalsize > MAXATTACHSIZE) {
			printf("�ļ� %s ��������ʧ��<br>", nohtml(void1(filename)));
			continue;
		}
		sprintf(filepath, "%s/%s", path, filename);

		mkdir(path, 0760);
		fp = fopen(filepath, "w");
		fwrite(parmFile->content, 1, parmFile->len, fp);
		fclose(fp);
		printf("�ļ� %s (%d�ֽ�) ���سɹ�<br>", filename,
		       parmFile->len);
		totalsize += parmFile->len;
	}
	return totalsize;
}

static void
upload_do_del(char *path, char *filename)
{
	char filepath[1024];
	if (fixfilename(filename))
		http_fatal("��Ч���ļ�����������������ַ�(�ʺ����ſո��)");
	snprintf(filepath, sizeof (filepath), "%s/%s", path, filename);
	if (unlink(filepath) < 0) {
		printf("ɾ���ļ�ʧ�ܣ���%s��<br>\n", filename);
	}
}

void
printuploadform()
{
	printf("<script>function moreupload(){"
	       "document.getElementById('moreupload').innerHTML='"
	       "<input type=file name=file5><br>"
	       "<input type=file name=file6><br>"
	       "<input type=file name=file7><br>"
	       "<input type=file name=file8><br>"
	       "<input type=file name=file9>';}</script>");

	printf("<hr>"
	       "<form name=frmUpload action=bbsupload enctype='multipart/form-data' method=post>"
	       "���ظ���: <br>"
	       "<input type=file name=file1><br>"
	       "<input type=file name=file2><br>"
	       "<input type=file name=file3><br>"
	       "<input type=file name=file4><br>"
	       "<div id='moreupload'><a href=\"javascript:moreupload();\">�ϴ�����...</a></div>"
	       "<input type=submit value=���� "
	       "onclick=\"this.value='���������У����Ժ�...';this.disabled=true;frmUpload.submit();\">"
	       "</form> "
	       "���������Ϊ���¸��ӵ�СͼƬС����ɶ��, ��Ҫ��̫��Ķ���Ŷ, "
	       "�ļ�������Ҳ��Ҫ�������ʺ�ʲô��, �����ճ��ʧ��Ŷ.<br>"
	       "<b>���������������ⶨλ����</b>��ֻ��Ҫ�����±༭����Ԥ��"
	       "λ���϶�ͷд�ϡ�#attach 1.jpg���Ϳ�����(�����˽� 1.jpg ���������ص��ļ��� :) )"
	       "<center><input type=button value='ˢ��' onclick=\"location='bbsupload';\">&nbsp; &nbsp;"
	       "<input type=button value='���' onclick='window.close();'></center>");
}

int
bbsupload_main()
{
	char *ptr;
	char userattachpath[256];
	int totalsize;

	html_header(1);
	printf("<body><center><div class=swidth style=\"text-align:left\">");
	if (!loginok || isguest)
		http_fatal("���ȵ�¼");
	if (!user_perm(currentuser, PERM_POST))
		http_fatal("ȱ�� POST Ȩ��");

	snprintf(userattachpath, sizeof (userattachpath), PATHUSERATTACH "/%s",
		 currentuser->userid);
	mkdir(userattachpath, 0760);

	ptr = getparm("del");
	if (*ptr) {
		upload_do_del(userattachpath, ptr);
		totalsize = upload_getpathsize(userattachpath, 1);
		if (totalsize < MAXATTACHSIZE)
			printuploadform();
		printf("</body></html>");
		return 0;
	}

	totalsize = upload_getpathsize(userattachpath, 0);
	if (totalsize < 0)
		http_fatal("�޷����Ŀ¼��С");

	totalsize = save_attach(userattachpath, totalsize);

	/* Cleanup. */
	totalsize = upload_getpathsize(userattachpath, 1);
	if (totalsize < MAXATTACHSIZE)
		printuploadform();
	printf("</div></center></body></html>");
	return 0;
}
