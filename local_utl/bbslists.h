/* bbslists.c */
void bbslists_newaccount(int day, char *time, char *user, char *other);
void bbslists_enter(int day, char *time, char *user, char *other);
void bbslists_drop(int day, char *time, char *user, char *other);
void bbslists_exitbbs(int day, char *time, char *user, char *other);
void bbslists_use(int day, char *time, char *user, char *other);
void draw_account(void);
void draw_newacct(void);
void draw_usage(void);
void bbslists_exit(void);
void bbslists_init(void);
