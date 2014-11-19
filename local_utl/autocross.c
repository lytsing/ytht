#include "ythtlib.h"
#include "bbs.h"

#define AUTOCROSS_CONF_FILENAME MY_BBS_HOME "/etc/autocross"
#define AUTOCROSS_RULE_MAX 1024

struct autocross_rule {
	char board_from[32];
	char board_to[32];
	char sign;
};

struct autocross_rule rules[AUTOCROSS_RULE_MAX];
int nrules = 0;

char POST_NAME[32], POST_PASS[32];

void
read_post()
{
	FILE *fp;
	fp = fopen(MY_BBS_HOME "/etc/bmstatpost", "r");
	if (fp == NULL) {
		errlog("Can't open poster info file!");
		exit(-1);
	}
	fgets(POST_NAME, sizeof(POST_NAME), fp);
	fgets(POST_PASS, sizeof(POST_PASS), fp);
	fclose(fp);
}

int
read_conf_file()
{
	FILE *fp;
	char temp[256];
	char *token;
	int state = 0;
	fp = fopen(AUTOCROSS_CONF_FILENAME, "r");
	if (!fp)
		return -1;	// fail
	while (fgets(temp, 256, fp)) {
		// each rule in a line: board_from[\t]board_to[\t]sign[\n]
		// no duplicate board_from is allowed, but no check here, duplicate keys will cause no action
		// strtok(temp, "\n");
		/* NOTE: On the first call to strtok, the function skips leading delimiters */
		if (temp[0] == '\n')
			continue;
		token = strtok(temp, "\t");
		while (token) {
			if (state == 0) {
				strncpy(rules[nrules].board_from, token, 32);
				state ++;
//printf("%d: boardfrom: %s\n", nrules, token);
			} else if (state == 1) {
				strncpy(rules[nrules].board_to, token, 32);
				state ++;
//printf("%d: boardto: %s\n", nrules, token);
			} else if (state == 2) {
				rules[nrules].sign = *token;
//printf("%d: sign: %c\n", nrules, *token);
				nrules ++;
				if (nrules == AUTOCROSS_RULE_MAX)
					break;	// cut off the overflow ones
				state = 0;
			}
			token = strtok(NULL, "\t\n");
		}
		if (state != 0) {
			errlog("autocross: bad configure file format");
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

int
check_board(int idx)
{
	char path[256];
	FILE *fp, *fp_p;
	struct fileheader *fh;
	struct mmapfile mf = { ptr:NULL };
	int retv, total, i;
	int filetime;
	char temp[256];
	char title[256];
//printf("processing %s ...\n", rules[idx].board_from);
	sprintf(path, MY_BBS_HOME "/boards/%s/.autocross", rules[idx].board_from);
	if ((fp = fopen(path, "r")) == NULL) {
		filetime = 0;
	} else {
		fscanf(fp, "%d", &filetime);
		fclose(fp);
	}
	sprintf(path, MY_BBS_HOME "/boards/%s/.DIR", rules[idx].board_from);

	MMAP_TRY {
		if (mmapfile(path, &mf) == -1)
			MMAP_RETURN(-1);
		total = mf.size / sizeof (struct fileheader) - 1;
		retv = Search_Bin((struct fileheader *)mf.ptr, filetime, 0, total);
		if (retv < 0)
			retv = -retv - 1;
		else
			retv ++;
		if (retv > total)
			MMAP_RETURN(0);	// nothing new
		for (i = retv; i <= total; i ++) {
//printf(".\n");
			fh = (struct fileheader *) (mf.ptr + i * sizeof (struct fileheader));
			if (((fh->accessed & FH_MARKED) && (rules[idx].sign == 'm')) ||
				((fh->accessed & FH_DIGEST) && (rules[idx].sign == 'g')) ||
				((fh->accessed & FH_MARKED) && (fh->accessed & FH_DIGEST) && (rules[idx].sign == 'b'))) {
				// if not topic, ignore
				if (fh->filetime != fh->thread)
					continue;
				// need cross
				sprintf(title, "%s [%s]", fh->title, rules[idx].board_from);
				sprintf(path, MY_BBS_HOME "/boards/%s/%s", rules[idx].board_from, fh2fname(fh));
				fp = fopen(path, "r");
				if (!fp) {
					errlog("autocross: unable to read source file %s", path);
					continue;	// fail, ignore
				}
				fp_p = popen("/usr/bin/mail bbs", "w");
				if (!fp_p) {
					errlog("autocross: fail opening pipe: /usr/bin/mail bbs, for write");
					fclose(fp);
					MMAP_RETURN(-1);	// fail, abort
				}
				fprintf(fp_p, "#name: %s\n", POST_NAME);
				fprintf(fp_p, "#pass: %s\n", POST_PASS);
				fprintf(fp_p, "#board: %s\n", rules[idx].board_to);
				fprintf(fp_p, "#title: %s\n", title);
				fprintf(fp_p, "#localpost:\n\n");
				while (fgets(temp, 256, fp))
					fprintf(fp_p, "%s", temp);
				pclose(fp_p);
				fclose(fp);
				sleep(2);
//printf("!\n");
			}
		}
		sprintf(path, MY_BBS_HOME "/boards/%s/.autocross", rules[idx].board_from);
		fp = fopen(path, "w");
		if (fp) {
			fprintf(fp, "%d\n", fh->filetime);
			fclose(fp);
		}
	}

	MMAP_CATCH {
		MMAP_RETURN(-1);
	}

	MMAP_END mmapfile(NULL, &mf);

//printf("\n");
	return 0;
}

int
clearstamp(int idx)
{
	char path[256];
	FILE *fp = NULL;
	struct fileheader *fh;
	int total;
	struct mmapfile mf = { ptr:NULL };
	sprintf(path, MY_BBS_HOME "/boards/%s/.DIR", rules[idx].board_from);

	MMAP_TRY {
		if (mmapfile(path, &mf) == -1)
			MMAP_RETURN(-1);
		total = mf.size / sizeof (struct fileheader) - 1;
		if (total < 0)
			MMAP_RETURN(0);
		fh = (struct fileheader *) (mf.ptr + total * sizeof (struct fileheader));
		sprintf(path, MY_BBS_HOME "/boards/%s/.autocross", rules[idx].board_from);
		if ((fp = fopen(path, "w")) == NULL) {
			MMAP_RETURN(-1);
		} else {
			fprintf(fp, "%d\n", fh->filetime);
			fclose(fp);
		}
	}
	
	MMAP_CATCH {
		if (fp)
			fclose(fp);
		MMAP_RETURN(-1);
	}

	MMAP_END mmapfile(NULL, &mf);
		
	return 0;
}

int
main(int argc, char *argv[])
{
	int i, clear = 0;
	if (argc > 1) {
		// argument
		if (strcmp(argv[1], "clear") != 0) {
			printf("usage: autocross [clear]\n");
			return 0;
		}
		// clear all time stamp first
		clear = 1;
	}
	read_post();
	if (read_conf_file() == -1) {
		errlog("autocross: error reading configure file");
		return -1;
	}
	if (clear) {
		for (i = 0; i < nrules; i ++)
			clearstamp(i);
		return 0;
	}
	for (i = 0; i < nrules; i ++)
		check_board(i);
	return 0;
}

