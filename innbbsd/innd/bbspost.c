/*
 * File: scanboard.c
 */
char *ProgramUsage = "\
bbspost (list|visit) bbs_home\n\
        post board_path < uid + title + Article...\n\
	post bbs_home board < uid + title + Article...\n\
        mail board_path < uid + title + passwd + realfrom + Article...\n\
        mail bbs_home board < uid + title + passwd + realfrom + Article...\n\
	cancel bbs_home board filename\n\
        expire bbs_home board days [max_posts] [min_posts]\n";
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(PalmBBS)

#include "bbs.h"
#include "ythtbbs.h"
#define INNDHOME MY_BBS_HOME "/inndlog"
//#include "../innbbsconf.h"

#define MAXLEN          1024

char *crypt();
char *homepath;
int visitflag;
char realfrom[MAXLEN];
struct bbsinfo bbsinfo;
struct BCACHE *brdshm = NULL;
struct boardmem *bcache = NULL;
void usage(void);
void report(void);
int ci_strcmp(register char *s1, register char *s2);
void search_article(char *brdname);
void search_boards(int visit);
void check_password(struct userec *record);
void check_userec(struct userec **record, char *name);
void getrealauthor(char *buf, char *author, int len);
void post_article(int usermail);
void cancel_article(char *board, char *file, char *message);
void expire_article(char *brdname, char *days_str, int maxpost, int minpost);
int main(int argc, char *argv[]);
struct boardmem *getbcache(char *bname);
int updatelastpost(char *board);
int tailbr(char *str);
int seek_in_file(char filename[STRLEN], char seekstr[STRLEN]);
int deny_me(char *path, char *userid);
int club_me(char *path, char *userid);

void
usage()
{
	puts(ProgramUsage);
	exit(0);
}

void
report()
{
	/* Function called from record.o */
	/* Please leave this function empty */
}

int
ci_strcmp(s1, s2)
register char *s1, *s2;
{
	char c1, c2;

	while (1) {
		c1 = *s1++;
		c2 = *s2++;
		if (c1 >= 'a' && c1 <= 'z')
			c1 += 'A' - 'a';
		if (c2 >= 'a' && c2 <= 'z')
			c2 += 'A' - 'a';
		if (c1 != c2)
			return (c1 - c2);
		if (c1 == 0)
			return 0;
	}
}

void
search_article(brdname)
char *brdname;
{
//fileheader定义改了, 象这样的写.DIR的方式不合适了, 而且似乎没有什么地方调用这个函数功能了, 注释掉
#if 0
	struct fileheader head;
	struct stat state;
	char index[MAXLEN], article[MAXLEN];
	int fd, num, offset, type;
	char send;

	offset = (int) &(head.filename[STRLEN - 1]) - (int) &head;
	sprintf(index, "%s/boards/%s/.DIR", homepath, brdname);
	if ((fd = open(index, O_RDWR)) < 0) {
		return;
	}
	fstat(fd, &state);
	num = (state.st_size / sizeof (head)) - 1;
	while (num >= 0) {
		lseek(fd, num * sizeof (head) + offset, 0);
		if (read(fd, &send, 1) > 0 && send == '%')
			break;
		num -= 4;
	}
	num++;
	if (num < 0)
		num = 0;
	lseek(fd, num * sizeof (head), 0);
	for (send = '%'; read(fd, &head, sizeof (head)) > 0; num++) {
		type = head.filename[STRLEN - 1];
		if (type != send && visitflag) {
			lseek(fd, num * sizeof (head) + offset, 0);
			safewrite(fd, &send, 1);
			lseek(fd, (num + 1) * sizeof (head), 0);
		}
		if (type == '\0') {
			printf("%s\t%s\t%s\t%s\n", brdname,
			       head.filename, head.owner, head.title);
		}
	}
	close(fd);
#endif
}

void
search_boards(visit)
{
	struct dirent *de;
	DIR *dirp;
	char buf[8192];

	return;

	visitflag = visit;
	sprintf(buf, "%s/boards", homepath);

	if ((dirp = opendir(buf)) == NULL) {
		printf(":Err: unable to open %s\n", buf);
		return;
	}

	printf("New article listed:\n");

	while ((de = readdir(dirp)) != NULL) {
		if (de->d_name[0] > ' ' && de->d_name[0] != '.')
			search_article(de->d_name);
	}

	closedir(dirp);
}

void
check_password(record)
struct userec *record;
{
	char passwd[MAXLEN];

	fgets(passwd, MAXLEN, stdin);
	tailbr(passwd);
	if (checkpasswd(record->passwd, record->salt, passwd) == 0) {
		printf(":Err: user '%s' password incorrect!!\n",
		       record->userid);
		exit(0);
	}
	fgets(realfrom, sizeof (realfrom), stdin);
	tailbr(realfrom);

/*    sprintf( genbuf, "tmp/email_%s", record->userid );
    if( (fn = fopen( genbuf, "w" )) != NULL ) {
        fprintf( fn, "%s\n", realfrom );
        fclose( fn );
    }*/
/*	if (!strstr(realfrom, "bbs@")) {
		//record->ip[15] = '\0';
		//strncpy(record->realmail, realfrom, STRLEN - 16);
	}*/
/*    if( !strstr( homepath, "test" ) ) {
        record->numposts++;
    }*/
}

void
check_userec(record, name)
struct userec **record;
char *name;
{
	int pos;
	struct userec *currentuser;
	struct boardmem *bptr;
	char *ptr;

	ptr = strrchr(homepath, '/');
	if (ptr == NULL)
		ptr = homepath;
	else
		ptr++;
	bptr = getbcache(ptr);
	if (bptr == NULL)
		exit(0);
	if ((pos = getuser(name, &currentuser)) <= 0) {
		printf(":Err: unknown userid %s\n", name);
		exit(0);
	}
	strcpy(name, currentuser->userid);
	check_password(currentuser);

	if (!USERPERM(currentuser, PERM_POST))
		exit(0);
	if (!((bptr->header.level & PERM_POSTMASK)
	      || USERPERM(currentuser, bptr->header.level)
	      || (bptr->header.level & PERM_NOZAP)))
		exit(0);
	if (!
	    (USERPERM
	     (currentuser,
	      ((bptr->header.level & ~PERM_NOZAP) & ~PERM_POSTMASK))))
		exit(0);

	if (deny_me(homepath, currentuser->userid)
	    || deny_me(MY_BBS_HOME, currentuser->userid))
		exit(0);
	if ((bptr->header.flag & CLUB_FLAG)
	    && (!club_me(homepath, currentuser->userid)))
		exit(0);
	*record = currentuser;
	return;
}

void
getrealauthor(char *buf, char *author, int len)
{
	char *ptr, *f1, *f2;
	if (strncmp(buf, "寄信人: ", 8) && strncmp(buf, "发信人: ", 8))
		return;
	ptr = buf + 8;
	f1 = strsep(&ptr, " ,\n\r\t");
	if (f1)
		strsncpy(author, f1, len);
	f2 = strsep(&ptr, " ,\n\r\t");
	if (f2 && f2[0] == '<' && f2[strlen(f2) - 1] == '>' && strchr(f2, '@')) {
		f2[strlen(f2) - 1] = 0;
		strsncpy(author, f2 + 1, len - 1);
	}
	ptr = strpbrk(author, "();:!#$\"\'");
	if (ptr)
		*ptr = 0;
	strcat(author, ".");
	return;
}

void
post_article(int usermail)
{
	struct userec *record;
	struct fileheader header;
	char userid[MAXLEN], subject[MAXLEN];
	char index[MAXLEN], name[MAXLEN], article[MAXLEN];
	char buf[MAXLEN], *ptr;
	FILE *fidx;
	int fh, islocalpost, n = 0, i = 0, bypost = 0;
	time_t now;
	char local[MAXLEN];
	char tmp[256];

	sprintf(index, "%s/.DIR", homepath);
	if ((fidx = fopen(index, "r")) == NULL) {
		if ((fidx = fopen(index, "w")) == NULL) {
			printf(":Err: Unable to post in %s.\n", homepath);
			return;
		}
	}
	fclose(fidx);

	fgets(userid, sizeof (userid), stdin);
	tailbr(userid);
	fgets(subject, sizeof (subject), stdin);
	tailbr(subject);
	if (usermail) {
		check_userec(&record, userid);
	}
	if (!strcmp(userid, "post"))
		bypost = 1;
	now = time(NULL);
	n = 0;
	while (n < 30) {
		sprintf(name, "M.%ld.A", now + n);
		sprintf(article, "%s/%s", homepath, name);
		fh = open(article, O_CREAT | O_EXCL | O_WRONLY, 0644);
		if (fh != -1)
			break;
		n++;
	}
	if (fh == -1)
		return;

	printf("post to %s\n", article);
	ptr = strrchr(homepath, '/');
	(ptr == NULL) ? (ptr = homepath) : (ptr++);
	if (usermail) {
		fgets(local, sizeof (local), stdin);
		tailbr(local);
		islocalpost = strcmp(local, "localpost");
		sprintf(buf,
			"发信人: %s (%s), 信区: %s\n标  题: %s\n发信站: "
			MY_BBS_NAME " (%24.24s), %s\n\n", userid,
			record->username, ptr, subject, ctime(&now),
			islocalpost ? "转信" : "站内信件");
		write(fh, buf, strlen(buf));
	}
	while (fgets(buf, MAXLEN, stdin) != NULL) {
		if (bypost && (++i) <= 3) {
			strsncpy(tmp, buf, sizeof (tmp));
			getrealauthor(tmp, userid, 14);
		}
		write(fh, buf, strlen(buf));
	}
	if (usermail) {

		/*写入转信档 */
/*        if(islocalpost)
        {
          sprintf(outname,"%s/out.bntp",INNDHOME);
          if (foo = fopen(outname, "a"))
          {
            fprintf(foo, "%s\t%s\t%s\t%s\t%s\n", ptr,
              name, userid, record->username, subject);
            fclose(foo);
          }
        }
*/
		sprintf(buf,
			"\n--\n\033[1;36m☆ 来源:．\033[1;42m\033[1;31m"
			MY_BBS_NAME " " MY_BBS_DOMAIN "\033[0m．[FROM: %s]\n",
			realfrom);
		write(fh, buf, strlen(buf));
	}
	close(fh);

	bzero((void *) &header, sizeof (header));
	header.filetime = atoi(name + 2);
	fh_setowner(&header, userid, 0);
	strsncpy(header.title, subject, sizeof (header.title));
	fh_find_thread(&header, ptr);
	append_record(index, &header, sizeof (header));
	updatelastpost(ptr);
}

void
cancel_article(board, file, message)
char *board, *file, *message;
{
	struct fileheader header;
	struct stat state;
	char dirname[MAXLEN];
	char buf[MAXLEN];
	long size, filetime, now;
	int fd, lower, ent;

	if (file == NULL || file[0] != 'M' || file[1] != '.' ||
	    (filetime = atoi(file + 2)) <= 0)
		return;
	size = sizeof (header);
	sprintf(dirname, "%s/boards/%s/.DIR", homepath, board);
	if ((fd = open(dirname, O_RDWR)) == -1)
		return;
	flock(fd, LOCK_EX);
	fstat(fd, &state);
	ent = ((long) state.st_size) / size;
	lower = 0;
	while (1) {
		ent -= 8;
		if (ent <= 0 || lower >= 2)
			break;
		lseek(fd, size * ent, SEEK_SET);
		if (read(fd, &header, size) != size) {
			ent = 0;
			break;
		}
		now = header.filetime;
		lower = (now < filetime) ? lower + 1 : 0;
	}
	if (ent < 0)
		ent = 0;
	while (read(fd, &header, size) == size) {
		if (filetime == header.filetime) {
			sprintf(buf, "-%s", header.owner);
			fh_setowner(&header, buf, 0);
			if (message && *message)
				strsncpy(header.title,
					 "<< E-mail post 文章审核中... >>",
					 sizeof (header.title));
			else
				strsncpy(header.title, "<< article canceled >>",
					 sizeof (header.title));
			lseek(fd, -size, SEEK_CUR);
			safewrite(fd, &header, size);
			break;
		}
		if (header.filetime > filetime)
			break;
	}
	flock(fd, LOCK_UN);
	close(fd);
}

void
expire_article(brdname, days_str, maxpost, minpost)
char *brdname, *days_str;
{
	//据认为, 这个函索没有用了
#if 0
	struct fileheader head;
	struct stat state;
	char lockfile[MAXLEN], index[MAXLEN];
	char tmpfile[MAXLEN], delfile[MAXLEN];
	int days, total;
	int fd, fdr, fdw, done, keep;
	int duetime, ftime;

	days = atoi(days_str);
	if (days < 1) {
		printf(":Err: expire time must more than 1 day.\n");
		return;
	} else if (maxpost < 100) {
		printf(":Err: maxmum posts number must more than 100.\n");
		return;
	}
	sprintf(lockfile, "%s/.dellock", homepath, brdname);
	sprintf(index, "%s/boards/%s/.DIR", homepath, brdname);
	sprintf(tmpfile, "%s/.tmpfile", homepath, brdname);
	sprintf(delfile, "%s/.deleted", homepath, brdname);

	if ((fd = open(lockfile, O_RDWR | O_CREAT | O_APPEND, 0644)) == -1)
		return;
	flock(fd, LOCK_EX);
	unlink(tmpfile);

	duetime = time(NULL) - days * 24 * 60 * 60;
	done = 0;
	if ((fdr = open(index, O_RDONLY, 0)) > 0) {
		fstat(fdr, &state);
		total = state.st_size / sizeof (head);
		if ((fdw = open(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0644)) >
		    0) {
			while (read(fdr, &head, sizeof head) == sizeof head) {
				done = 1;
				ftime = atoi(head.filename + 2);
				if (head.owner[0] == '-')
					keep = 0;
				else if (head.accessed[0] & FILE_MARKED
					 || total <= minpost)
					keep = 1;
				else if (ftime < duetime || total > maxpost)
					keep = 1;
				else
					keep = 1;
				if (keep) {
					if (safewrite(fdw, &head, sizeof head)
					    == -1) {
						done = 0;
						break;
					}
				} else {
					printf("Unlink %s\n", head.filename);
					if (head.owner[0] == '-')
						printf("Unlink %s.cancel\n",
						       head.filename);
					total--;
				}
			}
			close(fdw);
		}
		close(fdr);
	}
	if (done) {
		unlink(delfile);
		if (rename(index, delfile) != -1) {
			rename(tmpfile, index);
		}
	}
	flock(fd, LOCK_UN);
	close(fd);
#endif
}

char *
fixhomepath(char *home, char *board)
{
	static char buf[1024 * 2];
	char *defstr = "/dev/null", bname[STRLEN] = "test";
	struct boardmem *bmem = getbcache(board);
	if (bmem)
		strsncpy(bname, bmem->header.filename, sizeof (bname));
	if (snprintf(buf, sizeof (buf), "%s/boards/%s", home, bname) >=
	    sizeof (buf))
		return defstr;
	return buf;
}

int
main(argc, argv)
char *argv[];
{
	char *progmode;
	int max, min;

	if (argc < 3)
		usage();
	progmode = argv[1];
	homepath = argv[2];
	if (initbbsinfo(&bbsinfo) < 0) {
		printf(":Err: unable to attach shm.\n");
		exit(0);
	}
	if (uhash_uptime() == 0) {
		printf(":Err: unable access uhash shm.\n");
		exit(0);
	}
	brdshm = bbsinfo.bcacheshm;
	bcache = bbsinfo.bcache;
	if (ci_strcmp(progmode, "list") == 0) {
		search_boards(0);
	} else if (ci_strcmp(progmode, "visit") == 0) {
		search_boards(1);
	} else if (ci_strcmp(progmode, "post") == 0) {
		if (argc >= 4)
			homepath = fixhomepath(homepath, argv[3]);
		post_article(0);
	} else if (ci_strcmp(progmode, "mail") == 0) {
		if (argc >= 4)
			homepath = fixhomepath(homepath, argv[3]);
		post_article(1);
	} else if (ci_strcmp(progmode, "cancel") == 0) {
		if (argc < 5)
			usage();
		if (argc >= 6)
			cancel_article(argv[3], argv[4], argv[5]);
		else
			cancel_article(argv[3], argv[4], NULL);
	} else if (ci_strcmp(progmode, "expire") == 0) {
		if (argc < 5)
			usage();
		max = atoi(argc > 5 ? argv[5] : "9999");
		min = atoi(argc > 6 ? argv[6] : "10");
		expire_article(argv[3], argv[4], max, min);
	}
	return 0;
}

#else
int
main()
{
	printf("You must use the bbspost in palmbbs source\n");
	return 0;
}

#endif

struct boardmem *
getbcache(bname)
char *bname;
{
	int i;
	int numboards;
	numboards = brdshm->number;
	for (i = 0; i < numboards; i++)
		if (!strncasecmp(bname, bcache[i].header.filename, STRLEN))
			return &bcache[i];
	return NULL;
}

int
updatelastpost(char *board)
{
	struct boardmem *bptr;
	bptr = getbcache(board);
	if (bptr == NULL)
		return -1;
	getlastpost(bptr->header.filename, &bptr->lastpost, &bptr->total);
	return 0;
}

int
tailbr(char *str)
{
	int len;
	len = strlen(str);
	while (len && (str[len - 1] == '\r' || str[len - 1] == '\n'))
		str[--len] = 0;
	return 0;
}

int
seek_in_file(filename, seekstr)
char filename[STRLEN], seekstr[STRLEN];
{

	FILE *fp;
	char buf[STRLEN];
	char *namep;
	if ((fp = fopen(filename, "r")) == NULL)
		return 0;
	while (fgets(buf, STRLEN, fp) != NULL) {
		namep = (char *) strtok(buf, ": \n\r\t");
		if (namep != NULL && strcasecmp(namep, seekstr) == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

int
deny_me(char *path, char *userid)
{
	char buf[STRLEN];
	sprintf(buf, "%s/deny_users", path);
	return seek_in_file(buf, userid);
}

int
club_me(char *path, char *userid)
{
	char buf[STRLEN];
	sprintf(buf, "%s/club_users", path);
	return seek_in_file(buf, userid);
}
