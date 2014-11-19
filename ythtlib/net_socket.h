#ifndef __NET_SOCKET_H
#define __NET_SOCKET_H
int bindSocket(char *address, int port, int queueLength);
int connectSocket(char *address, int port);
#endif
