SUBDIRS = . ythtlib libythtbbs src local_utl nju09 include
.PHONY: install-exec-local
install-exec-local:
	./Installbbs.sh
update:
	$(MAKE) -C src install
	$(MAKE) -C local_utl install
	$(MAKE) -C nju09 install
	$(MAKE) -C smth_sshbbsd update
EXTRA_DIST = doc site bbshome 

dist-hook:
	for dirname in $(EXTRA_DIST); do \
	  rm -rf `find $(distdir)/$$dirname -name .svn`; \
	done
