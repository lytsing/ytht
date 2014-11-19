#ifndef __VOTE_H
#define __VOTE_H

#define VOTE_YN         (1)
#define VOTE_SINGLE     (2)
#define VOTE_MULTI      (3)
#define VOTE_VALUE      (4)
#define VOTE_ASKING     (5)

#define VOTE_FLAG_OPENED 0x1
#define VOTE_FLAG_LIMITED 0x2
#define VOTE_FLAG_LIMITIP 0x4

struct ballot
{
        char    uid[IDLEN];                   /* ͶƱ��       */
        unsigned int voted;                  /* ͶƱ������   */
        char    msg[3][STRLEN];         /* ��������     */
};

struct votelog
{
	char uid[IDLEN+1];
	char ip[16];
	time_t votetime;
	unsigned int voted;
};

struct votebal
{
        char            userid[IDLEN+1];
	char            title[STRLEN];
        char            type;
        char            items[32][38];
        int             maxdays;
        int             maxtkt;
        short int	totalitems;
	short int	flag;
        time_t          opendate;
};

int invalid_voteIP(char *ip);
#endif

