INCLUDES = -I$(top_srcdir)/include -D_GNU_SOURCE
DEFS += $(OS_DEF)
AM_CFLAGS = $(OPTI_CFLAGS) -DMY_BBS_HOME='"$(BBS_HOME)"'

#.c.o:	;  $(CC) -c $*.c $(CFLAGS) $(OS_DEF) -D_GNU_SOURCE

noinst_LIBRARIES = libytht.a

libytht_a_SOURCES = uudecode.c strhash.c common.c numbyte.c strop.c fileop.c \
	strlib.c shmop.c uuencode.c net_socket.c named_socket.c crypt.c \
	limitcpu.c timeop.c smth_filter.c mgrep.c md5.c compatible.c \
	newconvcode.c htmlfunc.c szm.c setproctitle.c base64.c mime.c

noinst_HEADERS = base64.h compatible.h convcode.h crypt.h fileop.h htmlfunc.h \
	limitcpu.h md5.h mime.h named_socket.h net_socket.h setproctitle.h \
	strlib.h szm.h timeop.h ythtlib.h

#trans: trans.c convcode.c
#	$(CC) trans.c -o trans
#convtable.c: trans
#	./trans > convtable.c
	
