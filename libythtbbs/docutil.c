#include <stdio.h>
#include <regex.h>
#include "ythtbbs.h"

int eff_size_isjunk = 0;
static const char junkstr[] =
    "��һ��" "|����ʮ��" "|��!" "|����" "|����" "|��Ŷ" "|��ѽ" "|�Ҷ�" "|TMD"
    "|����" "|��" "|��~" "|��һ��" "|^ *�� *\r*$" "|^ *��" "|��!" "|����"
    "|^ *kao" "|��һ��" "|^ *ding *\r*$" "|^ *re *\r*$" "|������$" "|!!!";
static int
mem_eff_size(char *ptr0, int size0)
{
	int i, size, size2 = 0;
	struct memline ml;
	char *ptr, buf[250];
	regex_t reg;
	regcomp(&reg, junkstr,
		REG_EXTENDED | REG_ICASE | REG_NEWLINE | REG_NOSUB);
	eff_size_isjunk = 0;
	if (size0 > 3000 || size0 == 0) {
		size = size0;
		goto END;
	}
	size = 0;
	memlineinit(&ml, ptr0, size0);
	for (i = 0; i < 10; i++) {
		if (!memlinenext(&ml))
			break;
		ptr = ml.text;
		while (ptr < ml.text + ml.len && strchr(" \t\n\r", *ptr))
			ptr++;
		if (ptr == ml.text + ml.len)
			break;
	}
	while (memlinenext(&ml)) {
		if (ml.len > 2) {
			if (!strncmp(ml.text, "--\n", 3)
			    || !strncmp(ml.text, "--\r", 3))
				break;
			if (!strncmp(ml.text, ": ", 2))
				continue;
		}
		if (ml.len > 4 && !strncmp(ml.text, "�� ��", 5)
		    && memmem(ml.text, ml.len, "�Ĵ������ᵽ: ��", 16))
			continue;
		if (memmem(ml.text, ml.len, "�� ��Դ:��", 10) ||
		    memmem(ml.text, ml.len, "�� �޸�:��", 10))
			continue;
		for (i = 0; i < ml.len; i++) {
			if (ml.text[i] < 0)
				size2++;
			if (strchr(" \t\n\r", ml.text[i]))
				size2 += 2;
		}
		if (!eff_size_isjunk && size < 40) {
			int len = min(ml.len, sizeof (buf) - 1);
			memcpy(buf, ml.text, len);
			buf[len] = 0;
			if (0 == regexec(&reg, buf, 0, NULL, 0))
				eff_size_isjunk = 1;
		}
		size += ml.len;
	}
      END:
	size = size - size2 / 2;
	if (size <= 0)
		size = 1;
	regfree(&reg);
	if (size > 40)
		eff_size_isjunk = 0;
//�����Զ���ɾ����ǵĻ�����ֵ eff_size_isjunk Ϊ 0
#if 1
	eff_size_isjunk = 0;
#endif 
	return size;
}

int
eff_size(char *file)
{
	struct mmapfile mf = { ptr:NULL };
	int size = 1;
	MMAP_TRY {
		if (mmapfile(file, &mf) < 0)
			MMAP_RETURN(1);
		size = mem_eff_size(mf.ptr, mf.size);
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	return size;
}

char *
getdocauthor(char *filename, char *author, int len)
{
	char buf[256], *ptr, *f1, *f2;
	int i = 0;
	FILE *fp;
	author[0] = 0;
	fp = fopen(filename, "r");
	if (!fp)
		return author;
	while (i++ < 5) {
		if (!fgets(buf, sizeof (buf), fp))
			break;
		if (strncmp(buf, "������: ", 8) && strncmp(buf, "������: ", 8))
			continue;
		ptr = buf + 8;
		f1 = strsep(&ptr, " ,\n\r\t");
		if (f1)
			strsncpy(author, f1, len);
		f2 = strsep(&ptr, " ,\n\r\t");
		if (f2 && f2[0] == '<' && f2[strlen(f2) - 1] == '>'
		    && strchr(f2, '@')) {
			f2[strlen(f2) - 1] = 0;
			strsncpy(author, f2 + 1, len);
		}
		ptr = strpbrk(author, "();:!#$\"\'");
		if (ptr)
			*ptr = 0;
		break;
	}
	fclose(fp);
	return author;
}

int
keepoldheader(FILE * fp, int dowhat)
{
	static char (*tmpbuf)[STRLEN] = NULL;
	static int hash = 0;
	int i;
	switch (dowhat) {
	case SKIPHEADER:
	case KEEPHEADER:
		hash = i = 0;
		if (NULL == tmpbuf)
			tmpbuf = malloc(5 * STRLEN);
		if (NULL == tmpbuf)
			return -1;
		while (fgets(tmpbuf[i], STRLEN, fp)) {
			i++;
			if (!strcmp(tmpbuf[i - 1], "\n")
			    || !strcmp(tmpbuf[i - 1], "\r\n") || i > 4)
				break;
		}
		if (i < 4 || (strncmp(tmpbuf[0], "������: ", 8) &&
			      strncmp(tmpbuf[0], "������: ", 8)) ||
		    strncmp(tmpbuf[1], "��  ��: ", 8)) {
			fseek(fp, 0, SEEK_SET);
			i = 0;
			goto RET1;
		}
	      RET1:if (SKIPHEADER == dowhat) {
			free(tmpbuf);
			tmpbuf = NULL;
		}
		hash = i;
		return 0;
	case RESTOREHEADER:
		if (!tmpbuf)
			return -2;
		for (i = 0; i < hash; i++)
			fputs(tmpbuf[i], fp);
		free(tmpbuf);
		tmpbuf = NULL;
		hash = 0;
		return 0;
	}
	return -3;
}
