#ifndef SOCKET_H
#define SOCKET_H

extern int filenum, killconns;
extern client_t *clients;
extern pid_t mypid;
extern time_t curtime;

extern client_t *add_client __P ((int));
extern void remove_client __P ((client_t *));
extern int handle_socket __P ((client_t *));
extern void do_kill_conns __P ((void));
extern int select_loop __P ((int));

#endif /* SOCKET_H */
