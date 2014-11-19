#include "bbs.h"
#include <sys/types.h>
#include <sys/mman.h>
#include <ght_hash_table.h>
#include "www.h"
#define TOPN 2
#define TOPFN "TOPN"
#define FILTERTITLE "etc/.filtertitle_new"

struct data_s {
	ght_hash_table_t *user_hash;
	struct boardtop bt;
};

int now_t;

struct boardtop *topten = NULL;
struct boardtop *ctopten = NULL;
int allflag = 0;
struct mmapfile filtermf = { ptr:NULL, size:0 };
struct bbsinfo bbsinfo;

int
getbnum(char *board)
{
	int i;
	struct boardmem *bcache;
	bcache = bbsinfo.bcacheshm->bcache;
	for (i = 0; i < MAXBOARD && i < bbsinfo.bcacheshm->number; i++) {
		if (!strcasecmp(board, bcache[i].header.filename))
			return i;
	}
	return -1;
}

int
cmpbt(struct boardtop *a, struct boardtop *b)
{
	return b->unum - a->unum;
}

int
filtertitle(char *tofilter)
{
	if (filtermf.ptr == NULL)
		return 0;
	return filter_string(tofilter, &filtermf);
}

int
trytoinsert(char *board, struct boardtop *bt)
{
	memcpy(topten + 10, bt, sizeof (struct boardtop));
	qsort(topten, 11, sizeof (struct boardtop), (void *) cmpbt);
	if (!filtertitle(bt->title) && !filtertitle(board)) {
		memcpy(ctopten + 10, bt, sizeof (struct boardtop));
		qsort(ctopten, 11, sizeof (struct boardtop), (void *) cmpbt);
	}
	return 0;
}

int
_topn(struct boardheader *bh)
{
	//从 bh 里面找到TOPN个thread
	int size = sizeof (struct fileheader), total;
	int i, j, k, *usernum;
	struct mmapfile mf;
	struct fileheader *ptr;
	ght_hash_table_t *p_table = NULL;
	ght_iterator_t iterator, iterator1;
	struct data_s *data;
	char owner[14];
	int start, tocount, first;
	struct boardtop *bt, *bt1;
	char topnfilename[80], tmpfn[80];
	FILE *fp;
	char DOTDIR[80];
	char path[80];
	void *key, *key1;
	char *real_title;

	sprintf(DOTDIR, "boards/%s/.DIR", bh->filename);
	sprintf(path, "boards/%s", bh->filename);
	sprintf(topnfilename, "%s/%s", path, TOPFN);
	unlink(topnfilename);
	if ((bh->flag & CLUB_FLAG) || bh->level != 0)	//特殊版面
		return 0;
	MMAP_TRY {
		if (mmapfile(DOTDIR, &mf) == -1) {
			errlog("open .DIR error, %s\n", DOTDIR);
			MMAP_RETURN(0);
		}
		if (mf.size == 0) {	//空版面
			mmapfile(NULL, &mf);
			MMAP_RETURN(0);
		}
		total = mf.size / size - 1;	//共 0 - total 条目
		start =
		    Search_Bin((struct fileheader *) mf.ptr, now_t - 86400, 0,
			       total);
		if (start < 0) {
			start = -(start + 1);
		}
		if (start > total) {	//没有新文章
			mmapfile(NULL, &mf);
			MMAP_RETURN(0);
		}
		tocount = total - start + 1;
		bt = malloc(sizeof (struct boardtop) * tocount);
		if (bt == NULL) {
			errlog("malloc failed");
			exit(-1);
		}
		p_table = ght_create(tocount);
		for (i = start; i <= total; i++) {
			ptr =
			    (struct fileheader *) (mf.ptr +
						   i *
						   sizeof (struct fileheader));
			if (strchr(ptr->owner, '.'))
				continue;
			if ((data =
			     ght_get(p_table, sizeof (int),
				     &(ptr->thread))) == NULL) {
				if ((data = malloc(sizeof (struct data_s))) ==
				    NULL) {
					errlog("malloc failed");
					exit(-1);
				}
				if ((usernum = malloc(sizeof (int))) == NULL) {
					errlog("malloc failed");
					exit(-1);
				}
				first =
				    Search_Bin((struct fileheader *) mf.ptr,
					       ptr->thread, 0, total);
				if (first < 0){
					real_title = ptr->title;
					strncpy(data->bt.firstowner, fh2owner(ptr), 14);
				} else {
					real_title =
					    ((struct fileheader *) mf.ptr +
					     first)->title;
					strncpy(data->bt.firstowner,
						fh2owner((struct fileheader *) mf.ptr + first), 14);
				}
				if (!strncmp(real_title, "Re: ", 4))
					real_title += 4;
				strncpy(data->bt.title, real_title, 60);
				data->user_hash = NULL;
				data->user_hash = ght_create(tocount);
				data->bt.unum = 1;
				(data->bt.lasttime) = ptr->filetime;
				data->bt.thread = ptr->thread;
				strncpy(data->bt.board, bh->filename, 24);
				strncpy(owner, fh2realauthor(ptr), 14);
				//strncpy(data->bt.firstowner, fh2owner(ptr), 14);
				*usernum = 0;
				ght_insert(data->user_hash, usernum,
					   sizeof (char) * strlen(owner),
					   owner);
				ght_insert(p_table, data, sizeof (int),
					   &(ptr->thread));
			} else {
				strncpy(owner, fh2realauthor(ptr), 14);
				if ((usernum =
				     ght_get(data->user_hash,
					     sizeof (char) * strlen(owner),
					     owner)) == NULL) {
					(data->bt.unum)++;
					(data->bt.lasttime) = ptr->filetime;
					if ((usernum = malloc(sizeof (int))) ==
					    NULL) {
						errlog("malloc failed");
						exit(-1);
					}
					ght_insert(data->user_hash, usernum,
						   sizeof (char) *
						   strlen(owner), owner);
				} else {
					(*usernum)++;
				}
			}
		}
	}
	MMAP_CATCH {
		mmapfile(NULL, &mf);
		MMAP_RETURN(0);
	}
	MMAP_END mmapfile(NULL, &mf);
	i = 0;
	bt1 = bt;
	for (data = ght_first(p_table, &iterator, &key); data;
	     data = ght_next(p_table, &iterator, &key)) {
		memcpy(bt1, &(data->bt), sizeof (struct boardtop));
		bt1++;
		i++;
		for (usernum = ght_first(data->user_hash, &iterator1, &key1);
		     usernum;
		     usernum = ght_next(data->user_hash, &iterator1, &key1)) {
			free(usernum);
		}
		ght_finalize(data->user_hash);
		data->user_hash = NULL;
		free(data);
	}
	ght_finalize(p_table);
	p_table = NULL;
	qsort(bt, i, sizeof (struct boardtop), (void *) cmpbt);
	sprintf(tmpfn, "%s/topntmp", path);
	fp = fopen(tmpfn, "w");
	if (!fp) {
		errlog("touch %s error", tmpfn);
		exit(1);
	}
	for (j = 0, k = 0, bt1 = bt; j < TOPN && j < i; j++, bt1++) {
		if (allflag)
			trytoinsert(bh->filename, bt1);
		if (bt1->unum < 15)
			continue;
		if (k == 0) {
			fprintf(fp, "<table width=80%%>");
		}
		k++;
		fprintf(fp,
			"<tr><td><font color=red>HOT</font></td><td><a href=bbsnt?B=%d&th=%d>%s",
			getbnum(bh->filename), bt1->thread, nohtml(void1(bt1->title)));
		fprintf(fp, "</a></td><td>[讨论人数:%d] </td></tr>\n ",
			bt1->unum);
	}
	if (k)
		fprintf(fp, "</table>");
	fclose(fp);
	if (k)
		rename(tmpfn, topnfilename);
	free(bt);
	return 0;
}

int
top_all_dir()
{
	return new_apply_record(".BOARDS", sizeof (struct boardheader),
				(void *) _topn, NULL);
}

int
html_topten(int mode, char *file)
{
	struct boardtop *bt;
	int j;
	FILE *fp;
	if (mode)
		bt = ctopten;
	else
		bt = topten;
	fp = fopen("wwwtmp/topten.tmp", "w");
	if (!fp) {
		errlog("topten write error");
		exit(1);
	}
	fprintf(fp, "<body><center>%s -- 今日十大热门话题\n<hr>\n",
		MY_BBS_NAME);
	fprintf(fp, "<table border=1>\n");
	fprintf
	    (fp,
	     "<tr><td>名次</td><td>讨论区</td><td>标题</td><td>作者</td><td>人数</td></tr>\n");
	for (j = 0; j < 10 && bt->unum != 0; j++, bt++) {
		fprintf(fp,
			"<tr><td>第 %d 名</td><td><a href=bbshome?B=%d>%s</a></td>"
			"<td><a href='bbsnt?B=%d&th=%d'>%s</a></td><td>"
			"<a href='qry?U=%s'>%s</a></td><td>%d</td></tr>\n",
			j + 1, getbnum(bt->board), bt->board, getbnum(bt->board), bt->thread,
			nohtml(void1(bt->title)), bt->firstowner, bt->firstowner, bt->unum);
	}
	fprintf(fp, "</table><center></body>");
	fclose(fp);
	rename("wwwtmp/topten.tmp", file);
	return 0;
}

int
telnet_topten(int mode, char *file)
{
	struct boardtop *bt;
	int j;
	FILE *fp;
	char buf[40];
	char *p;
	if (mode)
		bt = ctopten;
	else
		bt = topten;
	fp = fopen("etc/topten.tmp", "w");
	if (!fp) {
		errlog("topten write error");
		exit(1);
	}
	fprintf(fp,
		"                \033[1;34m-----\033[37m=====\033[41m 本日十大热门话题 \033[40m=====\033[34m-----\033[0m\n\n");
	for (j = 0; j < 10 && bt->unum != 0; j++, bt++) {
		strcpy(buf, ctime(&(bt->lasttime)));
		buf[20] = '\0';
		p = buf + 4;
		fprintf(fp,
			"\033[1;37m第\033[1;31m%3d\033[37m 名 \033[37m信区 : \033[33m%-16s\033[37m【\033[32m%s\033[37m】\033[33m%-13s\033[36m%4d\033[37m 人\n     \033[37m标题 : \033[1;44;37m%-60.60s\033[40m\n",
			j + 1, bt->board, p, bt->firstowner, bt->unum, bt->title);
	}
	fclose(fp);
	rename("etc/topten.tmp", file);
	return 0;
}

int
main(int argc, char **argv)
{
	struct boardheader bh;
	char *name;

	if (initbbsinfo(&bbsinfo) < 0) {
		printf("Failed to attach bbsinfo.\n");
		return 0;
	}

	while (1) {
		int c;
		c = getopt(argc, argv, "ah");
		if (c == -1)
			break;
		switch (c) {
		case 'a':
			allflag = 1;
			break;
		case 'h':
			printf
			    ("%s [-a|boardname]\n  do board top %d article\n",
			     argv[0], TOPN);
			return 0;
		case '?':
			printf
			    ("%s:Unknown argument.\nTry `%s -h' for more information.\n",
			     argv[0], argv[0]);
			return 0;
		}
	}
	chdir(MY_BBS_HOME);
	now_t = time(NULL);
	if (optind < argc) {
		name = argv[optind++];
		if (optind < argc) {
			printf
			    ("%s:Too many arguments.\nTry `%s -h' for more information.\n",
			     argv[0], argv[0]);
			return 0;
		}
		strncpy(bh.filename, name, STRLEN);
		_topn(&bh);
	}
	if (allflag) {
		topten = calloc(11, sizeof (struct boardtop));
		if (NULL == topten) {
			errlog("malloc failed");
			exit(1);
		}
		ctopten = calloc(11, sizeof (struct boardtop));
		if (NULL == ctopten) {
			errlog("malloc failed");
			exit(1);
		}
		if (file_exist(FILTERTITLE)) {
			if (mmapfile(FILTERTITLE, &filtermf) == -1) {
				errlog("mmap failed");
				exit(1);
			}
		}

		top_all_dir();
		telnet_topten(0, "etc/posts/day");
		telnet_topten(1, "etc/dayf");
		html_topten(0, "wwwtmp/topten");
		html_topten(1, "wwwtmp/ctopten");
		//要exit了，我就不free了，呵呵
	}
	return 0;
}
