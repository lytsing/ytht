#include "bbslib.h"

int
bbscon1_main()
{
	char board[80], path[160], dir[80], file[80], filename[160], buf[80];
	struct fileheader dirinfo;
	int type;
	char *ptr;
	int ret, ent, filetime;
	changemode(READING);
	type = atoi(getparm("T"));
	switch (type) {
	case 4:
		strsncpy(buf, getparm("F"), 80);
		ptr = strrchr(buf, '/');
		if (NULL == ptr)
			http_fatal("����Ĳ���1");
		strcpy(file, ptr + 1);
		*ptr = '\0';
		sprintf(path, "boards/.1984/");
		strcat(path, buf);
		break;
	case 5:
		strsncpy(buf, getparm("F"), 80);
		ptr = strrchr(buf, '/');
		if (NULL == ptr)
			http_fatal("����Ĳ���1");
		strcpy(file, ptr + 1);
		*ptr = '\0';
		sprintf(path, "boards/.backnumbers/");
		strcat(path, buf);
		ptr = strchr(buf, '/');
		if (NULL == ptr)
			http_fatal("����Ĳ���2");
		*ptr = '\0';
		strcpy(board, buf);
		if (getboard(board) == NULL)
			http_fatal("�����������");
		break;
	default:
		http_fatal("����Ĳ���0");
		break;
	}
	if (strncmp(file, "M.", 2) && strncmp(file, "G.", 2))
		http_fatal("����Ĳ���1");
	if (strstr(file, "..") || strstr(file, "/"))
		http_fatal("����Ĳ���2");
	sprintf(filename, "%s/%s", path, file);
	if (*getparm("attachname") == '/') {
		showbinaryattach(filename);
		return 0;
	}
	filetime = atoi(file + 2);
	sprintf(dir, "%s/.DIR", path);
	ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &dirinfo, &ent, 1);
	if (ret == -1)
		http_fatal("���Ĳ����ڻ����ѱ�ɾ��");
	html_header(1);
	check_msg();
	printf("<body><center>\n");
	showcon(filename);
	printf("</center></body>\n");
	http_quit();
	return 0;
}
