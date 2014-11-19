/*

auth-passwd.c

Author: Tatu Ylonen <ylo@cs.hut.fi>

Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
                   All rights reserved

Created: Sat Mar 18 05:11:38 1995 ylo

Password authentication.  This file contains the functions to check whether
the password is valid for the user.

*/


#include "includes.h"
#include "packet.h"
#include "ssh.h"
#include "servconf.h"
#include "xmalloc.h"
#include "bbs.h"

/* Tries to authenticate the user using password.  Returns true if
   authentication succeeds. */
extern struct userec *currentuser;
int auth_password(const char *server_user, const char *password)
{
    if (password[0] == '\0'||!strcasecmp(server_user,"guest")||!strcasecmp(server_user,"new"))
        return 0;

    if( !access("NOLOGIN",F_OK) && !USERPERM(currentuser, PERM_SPEC))
	return 0;

    if (!checkpasswd(currentuser->passwd, currentuser->salt, password)) {
	time_t t=time(0);
        logattempt(server_user, get_remote_ipaddr(),"SSH", t);
        return 0;
    }
    return 1;
}
