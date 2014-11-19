#include "ythtbbs.h"

char *
bm2str(buf, bh)
char *buf;
struct boardheader *bh;
{
	int i;
	buf[0] = 0;
	for (i = 0; i < 4; i++)
		if (bh->bm[i][0] == 0)
			break;
		else {
			if (i != 0)
				strcat(buf, " ");
			strcat(buf, bh->bm[i]);
		}
	return buf;
}

char *
sbm2str(buf, bh)
char *buf;
struct boardheader *bh;
{
	int i;
	buf[0] = 0;
	for (i = 4; i < BMNUM; i++)
		if (bh->bm[i][0] == 0)
			break;
		else {
			if (i != 0)
				strcat(buf, " ");
			strcat(buf, bh->bm[i]);
		}
	return buf;
}

int
chk_BM(struct userec *user, struct boardheader *bh, int isbig)
{
	int i;
	for (i = 0; i < 4; i++) {
		if (bh->bm[i][0] == 0)
			break;
		if (!strcmp(bh->bm[i], user->userid)
		    && bh->hiretime[i] >= user->firstlogin)
			return i + 1;
	}
	if (isbig)
		return 0;
	for (i = 4; i < BMNUM; i++) {
		if (bh->bm[i][0] == 0)
			break;
		if (!strcmp(bh->bm[i], user->userid)
		    && bh->hiretime[i] >= user->firstlogin)
			return i + 1;
	}
	return 0;
}

int
chk_BM_id(char *user, struct boardheader *bh)
{
	int i;
	for (i = 0; i < BMNUM; i++) {
		if (bh->bm[i][0] == 0) {
			if (i < 4) {
				i = 3;
				continue;
			}
			break;
		}
		if (!strcmp(bh->bm[i], user))
			return i + 1;
	}
	return 0;
}

int
bmfilesync(struct userec *user)
{
	char path[256];
	struct myparam1 mp;
	sethomefile(path, user->userid, "mboard");
	if (file_time(path) > file_time(".BOARDS"))
		return 0;
	memcpy(&(mp.user), user, sizeof (struct userec));
	mp.fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0600);	//touch a new file
	if (mp.fd == -1) {
		errlog("touch new mboard error");
		return -1;
	}
	mp.bid = 0;
	new_apply_record(".BOARDS", sizeof (struct boardheader),
			 (void *) fillmboard, &mp);
	close(mp.fd);
	return 0;
}

int
fillmboard(struct boardheader *bh, struct myparam1 *mp)
{
	struct boardmanager bm;
	int i;
	if ((i = chk_BM(&(mp->user), bh, 0))) {
		bzero(&bm, sizeof (bm));
		strncpy(bm.board, bh->filename, 24);
		bm.bmpos = i - 1;
		bm.bid = mp->bid;
		write(mp->fd, &bm, sizeof (bm));
	}
	(mp->bid)++;
	return 0;
}

static inline int
setbmhat(struct boardmem *bmem, char *userid, int online, int invisible)
{
	int bmpos;
	bmpos = chk_BM_id(userid, &bmem->header) - 1;
	if (bmpos < 0)
		return -1;
	if (online) {
		bmem->bmonline |= (1 << bmpos);
		if (invisible)
			bmem->bmcloak |= (1 << bmpos);
		else
			bmem->bmcloak &= ~(1 << bmpos);
	} else {
		bmem->bmonline &= ~(1 << bmpos);
		bmem->bmcloak &= ~(1 << bmpos);
	}
	return 0;
}

int
dosetbmstatus(struct boardmem *bcache, char *userid, int online, int invisible)
{
	int i;
	if(!bcache)
		return -1;
	for (i = 0; i < MAXBOARD; i++) {
		setbmhat(&bcache[i], userid, online, invisible);
	}
	return 0;
}

int
getlastpost(char *board, int *lastpost, int *total)
{
	struct fileheader fh;
	struct stat st;
	char filename[STRLEN * 2];
	int fd, atotal;

	snprintf(filename, sizeof (filename), MY_BBS_HOME "/boards/%s/.DIR",
		 board);
	if ((fd = open(filename, O_RDONLY)) < 0) {
		*lastpost = 0;
		*total = 0;
		return 0;
	}
	fstat(fd, &st);
	atotal = st.st_size / sizeof (fh);
	if (atotal <= 0) {
		*lastpost = 0;
		*total = 0;
		close(fd);
		return 0;
	}
	*total = atotal;
	lseek(fd, (atotal - 1) * sizeof (fh), SEEK_SET);
	if (read(fd, &fh, sizeof (fh)) > 0) {
		if (fh.edittime == 0)
			*lastpost = fh.filetime;
		else
			*lastpost = fh.edittime;
	}
	close(fd);
	return 0;
}

struct boardaux *
getboardaux(int bnum)		//bnum starts from 0, i.e., bnum for the first board is 0
{
      static struct mmapfile mf = { ptr:NULL };
	if (!mf.ptr) {
		if (mmapfile(BOARDAUX, &mf) < 0)
			return NULL;
	}
	if (bnum < 0 || bnum >= MAXBOARD)
		return NULL;
	return &((struct boardaux *) mf.ptr)[bnum];
}

int
addtopfile(int bnum, struct fileheader *fh)	//bnum starts from 0
{
	struct boardaux baux;
	int fd;
	fh->accessed |= FH_ISTOP;
	bzero(&baux, sizeof (struct boardaux));
	if ((fd = open(BOARDAUX, O_RDWR | O_CREAT, 0660)) < 0)
		return -1;
	lseek(fd, bnum * sizeof (struct boardaux), SEEK_SET);
	read(fd, &baux, sizeof (struct boardaux));
	if (baux.ntopfile >= min(5, MAXTOPFILE))
		return -1;
	baux.topfile[baux.ntopfile] = *fh;
	baux.ntopfile++;
	lseek(fd, bnum * sizeof (struct boardaux), SEEK_SET);
	write(fd, &baux, sizeof (struct boardaux));
	close(fd);
	return 0;
}

int
deltopfile(int bnum, int num)	//bnum starts from 0, num starts from 0
{
	struct boardaux baux;
	int fd, i;
	if (bnum < 0 || num < 0)
		return -1;
	bzero(&baux, sizeof (struct boardaux));
	if ((fd = open(BOARDAUX, O_RDWR | O_CREAT, 0660)) < 0)
		return -1;
	lseek(fd, bnum * sizeof (struct boardaux), SEEK_SET);
	read(fd, &baux, sizeof (struct boardaux));
	if (baux.ntopfile <= num)
		return -1;
	for (i = num; i < baux.ntopfile - 1; i++)
		baux.topfile[i] = baux.topfile[i + 1];
	baux.ntopfile--;
	lseek(fd, bnum * sizeof (struct boardaux), SEEK_SET);
	write(fd, &baux, sizeof (struct boardaux));
	close(fd);
	return 0;
}

int
updateintro(int bnum, char *filename)	//¸üÐÂ WWW °æÃæ¼ò½é£¬bnum starts from 0
{
	struct boardaux baux;
	int fd, i;
	char buf[200];
	if (bnum < 0 || bnum >= MAXBOARD)
		return -1;
	buf[0] = 0;
	fd = open(filename, O_RDONLY);
	if (fd >= 0) {
		i = read(fd, buf, sizeof (buf) - 1);
		buf[i] = 0;
		close(fd);
	}
	bzero(&baux, sizeof (struct boardaux));
	if ((fd = open(BOARDAUX, O_RDWR | O_CREAT, 0660)) < 0)
		return -1;
	lseek(fd, bnum * sizeof (struct boardaux), SEEK_SET);
	read(fd, &baux, sizeof (struct boardaux));
	strsncpy(baux.intro, buf, sizeof (baux.intro));
	lseek(fd, bnum * sizeof (struct boardaux), SEEK_SET);
	write(fd, &baux, sizeof (struct boardaux));
	close(fd);
	return 0;

}
