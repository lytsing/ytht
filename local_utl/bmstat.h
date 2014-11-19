/* bmstat.c */
void bm_posted(int day, char *time, char *user, char *other);
void bm_use(int day, char *time, char *user, char *other);
void bm_import(int day, char *time, char *user, char *other);
void bm_move(int day, char *time, char *user, char *other);
void bm_undigest(int day, char *time, char *user, char *other);
void bm_digest(int day, char *time, char *user, char *other);
void bm_mark(int day, char *time, char *user, char *other);
void bm_unmark(int day, char *time, char *user, char *other);
void bm_range(int day, char *time, char *user, char *other);
void bm_del(int day, char *time, char *user, char *other);
void bm_deny(int day, char *time, char *user, char *other);
void bm_same(int day, char *time, char *user, char *other);
void bm_exit(void);
void bm_init(void);
