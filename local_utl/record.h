/* record.c */
long get_num_records(char *filename, int size);
void toobigmesg(void);
int get_record(char *filename, void *rptr, int size, int id);
int get_records(char *filename, void *rptr, int size, int id, int number);
int substitute_record(char *filename, void *rptr, int size, int id);
int update_file(char *dirname, int size, int ent, int (*filecheck)(void), void (*fileupdate)(void));
