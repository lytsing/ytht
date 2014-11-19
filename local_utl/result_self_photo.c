#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

static char self_an_vote_path[] =
    "0Announce/groups/GROUP_T/self_photo/M1085243614";

static int self_an_vote_type[5] = {
	1085028064,
	1085028072,
	1085028080,
	1085035260,
	1085035280
};

static char *class[5] = {
	"单人男",
	"单人女",
	"情侣",
	"合照",
	"children"
};

static char *self_an_vote_ctype[8] = {
	"最佳时尚GG",
	"最佳时尚MM",
	"最佳情侣",
	"最佳团体",
	"最佳可爱baby",
	"最佳动作",
	"最佳表情",
	"最佳服饰"
};

void
get_score(char *path)
{
	DIR *d;
	int fd;
	FILE *fp;
	struct dirent *t;
	char buf[256];
	int count = 0, sum = 0, ts;
	d = opendir(path);
	if (NULL == d) {
		printf("%s error!\n", path);
		return;
	}
	while ((t = readdir(d))) {
		if (t->d_name[0] == '.')
			continue;
		snprintf(buf, sizeof (buf), "%s/%s", path, t->d_name);
		fd = open(buf, O_RDONLY);
		if (fd < 0)
			continue;
		read(fd, buf, sizeof (buf));
		close(fd);
		ts = atoi(buf);
		if (!ts)
			continue;
		count++;
		sum += ts;
	}
	closedir(d);
	sprintf(buf, "../%s/M%d/M%s", self_an_vote_path,
		self_an_vote_type[atoi(path)], path + 4);
	fp=fopen(buf,"r");
	if(fp){
		fgets(buf,sizeof(buf),fp);
		fgets(buf,sizeof(buf),fp);
		strtok(buf,"\n");
		fclose(fp);
	}

	printf("%s %s %s %s %d %d %f\n", class[atoi(path)],
	       self_an_vote_ctype[atoi(path + 2)], path, buf, count, sum,
	       sum * 1.0 / count);

}

int
main(int argc, char *argv[])
{
	DIR *d;
	struct dirent *t;
	d = opendir(".");
	if (NULL == d) {
		printf("error!\n");
		return -1;
	}
	while ((t = readdir(d))) {
		if (t->d_name[0] == '.')
			continue;
		get_score(t->d_name);
	}
	closedir(d);
	return 0;
}
