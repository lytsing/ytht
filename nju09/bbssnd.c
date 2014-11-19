#include "bbslib.h"

void
filterCR(char *str)
{
	char *ptr=str;
	for(ptr=str;*str;str++) {
		if(*str=='\r')
			continue;
		*ptr = *str;
		ptr++;
	}
	*ptr = 0;
}

int
testmath(char *ptr)
{
	int i = 0;
	if (!*ptr)
		return 0;
	if (*ptr == '$')
		i = 1;
	ptr++;
	while ((ptr = strchr(ptr, '$')) != NULL) {
		if (*(ptr - 1) != '\\')
			i++;
		ptr++;
	}
	return (i + 1) % 2;
}

int
bbssnd_main()
{
	char filename[80], filename2[80], dir[80], board[80], title[80], buf[256], *content,
	    *ref;
	int r, i, sig, mark = 0, outgoing, anony, guestre = 0, usemath, use_ubb;
	int is1984, to1984 = 0;
	struct boardmem *brd;
	struct fileheader x;
	int thread = -1;
	struct userec tmp;
	int ent, ret, filetime;
	html_header(1);

	getparmboard(board, sizeof(board));
	strsncpy(title, getparm("title"), 60);
	outgoing = strlen(getparm("outgoing"));
	anony = strlen(getparm("anony"));
	usemath = strlen(getparm("usemath"));
	use_ubb = strlen(getparm("useubb"));
	brd = getboard(board);
	if (brd == 0)
		http_fatal("错误的讨论区名称");
	if (brd->ban == 2)
		http_fatal("因为版面文章超限, 本版暂时封闭.");
	strcpy(board, brd->header.filename);
	sprintf(dir, "boards/%s/.DIR", board);
	ref = getparm("ref");
	r = atoi(getparm("rid"));
	thread = atoi(getparm("th"));
	if (ref[0] == '\0') {
		thread = -1;
	} else {
		filetime = atof(ref + 2);
		ret = get_fileheader_records(dir, filetime, NA_INDEX, 1, &x, &ent, 1);
		if (ret == 0) {
			thread = x.thread;
			if (x.accessed & FH_ALLREPLY) {
				if (strncmp(x.title, "Re: ", 4))
					snprintf(title, 60, "Re: %s", x.title);
				else
					snprintf(title, 60, "%s", x.title);
				guestre = 1;
				mark = mark | FH_ALLREPLY;
			}
		}
	}
	if (!loginok || (isguest && !guestre))
		http_fatal("匆匆过客不能发表文章，请先登录");
	changemode(POSTING);

	if (!(brd->header.flag & ANONY_FLAG))
		anony = 0;
	if (brd->header.flag & IS1984_FLAG)
		is1984 = 1;
	else
		is1984 = 0;
	for (i = 0; i < strlen(title); i++)
		if (title[i] <= 27 && title[i] >= -1)
			title[i] = ' ';
	i = strlen(title) - 1;
	while (i >= 0 && isspace(title[i])) {
		title[i] = 0;
		i--;
	}
	if (title[0] == 0)
		http_fatal("文章必须要有标题");
	sig = atoi(getparm("signature"));
	content = getparm("text");
	if (usemath && testmath(content))
		mark |= FH_MATH;
	if (!has_post_perm(currentuser, brd) && !guestre)
		http_fatal("此讨论区是唯读的, 或是您尚无权限在此发表文章.");
	if (noadm4political(board))
		http_fatal("对不起,因为没有版面管理人员在线,本版暂时封闭.");

	if ((now_t - w_info->lastposttime) < 6) {
		w_info->lastposttime = now_t;
		http_fatal("两次发文间隔过密, 请休息几秒后再试");
	}
	w_info->lastposttime = now_t;
	sprintf(filename, "bbstmpfs/tmp/%d.tmp", thispid);
	sprintf(filename2, "bbstmpfs/tmp/%d.tmp2", thispid);
	filterCR(content);
	if (use_ubb)
		ubb2ansi(content, filename2);
	else
		f_write(filename2, content);
	if (!hideboard_x(brd)) {
		int dangerous =
		    dofilter(title, filename2, political_board(board));
		if (dangerous == 1)
			to1984 = 1;
		else if (dangerous == 2) {
		#ifdef POST_WARNING
			char mtitle[256];
			sprintf(mtitle, "[发表报警] %s %.60s", board, title);
			mail_file(filename, "delete", mtitle, currentuser->userid);
			updatelastpost("deleterequest");
		#endif
			mark |= FH_DANGEROUS;
		}
	}

	if (insertattachments_byfile(filename, filename2, currentuser->userid))
		mark = mark | FH_ATTACHED;

	unlink(filename2);

	if (is1984 || to1984) {
		r = post_article_1984(board, title, filename,
				      currentuser->userid,
				      currentuser->username, fromhost, sig - 1,
				      mark, outgoing, currentuser->userid,
				      thread);
	} else if (anony)
		r =
		    post_article(board, title, filename, "Anonymous",
				 "我是匿名天使", "匿名天使的家", 0, mark,
				 outgoing, currentuser->userid, thread);
	else
		r =
		    post_article(board, title, filename, currentuser->userid,
				 currentuser->username, fromhost, sig - 1, mark,
				 outgoing, currentuser->userid, thread);
	if (r <= 0)
		http_fatal("内部错误，无法发文");
	if (!is1984 && !to1984) {
		brc_initial(currentuser->userid, board);
		brc_add_readt(r);
		brc_update(currentuser->userid);
	}
	unlink(filename);
	tracelog("%s post %s %s", currentuser->userid, board, title);
	if (!(brd->header.flag & CLUB_FLAG) && !junkboard(board)) {
		memcpy(&tmp, currentuser, sizeof (tmp));
		tmp.numposts++;
		updateuserec(&tmp, 0);
	}
	if (sig != u_info->signature) {
		u_info->signature = sig;
		snprintf(buf, 20, "%d", sig);
		saveuservalue(currentuser->userid, "signature", buf);
	}
	if (to1984) {
		printf("%s<br>\n", BAD_WORD_NOTICE);
		printf("[<a href='javascript:history.go(-2)'>返回</a>]");
	} else {
		sprintf(buf, "bbsdoc?B=%d", getbnumx(brd));
		redirect(buf);
	}
	return 0;
}
