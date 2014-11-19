/* worker.c */
int telnet_init(void);
int quit(void);
int _inkey(void);
int inkey(void);
char *map_char(int n);
int map_init(void);
int map_show(void);
int map_show_pos(int y, int x);
int check_if_win(void);
int find_y_x(int *y, int *x);
int map_move(int y0, int x0, int y1, int x1);
int main(void);
