#ifndef __BROADCAST_H
#define __BROADCAST_H
struct broad_item {
	char inlist;
	char broaded;
};
int shouldbroadcast(int userindex);
#define BROADCAST_LIST ".broad_list"
#define BROADCAST_FILE "etc/broad_announce"
#endif
