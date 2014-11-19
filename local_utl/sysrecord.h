/* sysrecord.c */
char *setbfile(char *buf, char *boardname, char *filename);
char *setbdir(char *buf, char *boardname);
void getcross(char *filepath, char *filepath2, char *nboard, char *posttitle);
int post_cross(char *filename, char *nboard, char *posttitle, char *owner);
int cmpbnames(char *bname, struct boardheader *brec);
int postfile(char *filename, char *owner, char *nboard, char *posttitle);
int securityreport(char *owner, char *str, char *title);
int deliverreport(char *board, char *title, char *str);
