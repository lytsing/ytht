INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/ythtlib -D_GNU_SOURCE 
DEFS += $(OS_DEF)
AM_CFLAGS = $(OPTI_CFLAGS)

.PHONY: ythtlib
ythtlib:
	$(MAKE) -C ../ythtlib
LDADD = $(top_srcdir)/ythtlib/libytht.a
noinst_LIBRARIES = libythtbbs.a
noinst_PROGRAMS = gensectree
gensectree_SOURCES = gensectree.c 
seclist.local:
	if [ -f ../site/seclist.txt ];then \
		ln -sf ../site/seclist.txt seclist.local; \
	else \
		ln -sf seclist.txt seclist.local; \
	fi
sectree.c: gensectree$(EXEEXT) seclist.local sectree.h
	./gensectree$(EXEEXT) > sectree.c
checkreg.c:
	cp checkreg.c.cvs checkreg.c
BUILT_SOURCES = ythtlib sectree.c checkreg.c
libythtbbs_a_SOURCES = boardrc.c misc.c record.c user.c article.c modetype.c \
	docutil.c msg.c politics.c goodgbid.c binaryattach.c permissions.c \
	bbseva.c attach.c regform.c announce.c board.c sectree.c sectreeop.c \
	bbsinfo.c uinfo.c mail.c broadcast.c vote.c yspam.c regform.c 
EXTRA_libythtbbs_a_SOURCES = sectree.c 
noinst_HEADERS = announce.h article.h attach.h bbseva.h bbsinfo.h \
	binaryattach.h board.h boardrc.h broadcast.h docutil.h goodgbid.h \
	mail.h misc.h modes.h modetype.h msg.h permissions.h politics.h \
	record.h regform.h sectree.h uhashgen.h uinfo.h user.h vote.h yspam.h \
	ythtbbs.h
