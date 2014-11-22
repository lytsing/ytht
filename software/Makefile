include Makefile.Base
apache_v:=1.3.31
apache_ssl_v:=1.49
fastcgi_v:=2.2.12
fcgi_v:=2.2.2
newtv:=0.50.33
newtplevel:=1
ghthashv=0.5.4
apache2_v:=2.0.43
fastcgicvs_v:=SNAP-0212222223

apache:=apache_$(apache_v)
apache_s:=$(apache).tar.gz
apache_ssl_s:=apache_$(apache_v)+ssl_$(apache_ssl_v).tar.gz
apache2:=httpd-$(apache2_v)
apache2_s:=$(apache2).tar.gz
fastcgi:=mod_fastcgi-$(fastcgi_v)
fastcgi_s:=$(fastcgi).tar.gz
fastcgicvs:=mod_fastcgi-$(fastcgicvs_v)
fastcgicvs_s:=$(fastcgicvs).tar.gz
fcgi:=fcgi-$(fcgi_v)
fcgi_s:=$(fcgi).tar.gz
VPATH:=$(apache)/src
CPU:=$(shell cat ../Makefile.Base|grep mcpu|sed -e 's/.*\-mcpu=\(.*\) -march.*/\1/')
SMAGIC:=$(shell cat ../include/bbsconfig.h|grep SMAGIC|head -n1|awk '{print $$3}')
FIRST_SMAGIC:=$(shell echo $(SMAGIC)|sed -e 's/\(.\).*/\1/')
SSL:=0
HERE:=$(shell pwd)
DIST:=$(shell cat /etc/issue|head -n1|cut -d' ' -f1)

.PHONY:installhttpd installfcgidev

mod_fastrw.h: ../include/bbsconfig.h
	echo "#define SMAGIC \"$(SMAGIC)\"" > mod_fastrw.h
	echo "#define FIRST_SMAGIC '$(FIRST_SMAGIC)'" >> mod_fastrw.h

ifeq ($(SSL), 1)
httpsd:$(apache_s) $(apache_ssl_s) $(fastcgi_s) mod_redurl.c mod_conn.c apache_modules/mod_fastrw.c mod_fastrw.h
else
httpd:$(apache_s) $(fastcgi_s) mod_redurl.c mod_conn.c apache_modules/mod_fastrw.c mod_fastrw.h
endif
	rm -fr $(apache);tar xzvf $(apache_s);tar xzvf $(fastcgi_s);cd $(fastcgi); cd ..;mv $(fastcgi) $(apache)/src/modules/fastcgi 
ifeq ($(SSL), 1)
	cd $(apache); tar xzvf ../$(apache_ssl_s);echo Y|./FixPatch;
endif
	cd $(apache); patch -p2 < ../501_debian_config_layout;patch -p1 < ../apachelimit.patch;OPTIM="-O2 -funroll-loops -ffast-math -malign-double -mcpu=$(CPU) -march=$(CPU) -fomit-frame-pointer -fno-exceptions -I $(HERE)" ./configure \
	--disable-module=autoindex \
	--disable-module=auth \
	--disable-module=imap \
	--disable-module=include \
	--disable-module=userdir \
	--add-module=../mod_redurl.c --add-module=../mod_conn.c --add-module=../apache_modules/mod_fastrw.c\
	--activate-module=src/modules/fastcgi/libfastcgi.a \
	--with-layout=$(DIST);make
ifeq ($(SSL),1)
installhttpd:httpsd
else
installhttpd:httpd
endif
	-mv /usr/sbin/httpd /usr/sbin/httpd.old
ifeq ($(SSL),1)
	mv $(apache)/src/httpsd /usr/sbin/httpd
	mv $(apache)/src/modules/ssl/gcache /usr/sbin
else
	mv $(apache)/src/httpd /usr/sbin/httpd
endif
#	cat ssl.conf >> /etc/httpd/conf/httpd.conf
installfcgidev:$(fcgi_s)
	rm -fr $(fcgi);tar xzvf $<;cd $(fcgi);./configure --prefix=/usr;make;make install
installnewt:newt-$(newtv)-$(newtplevel).src.rpm
	rpm2cpio $^|cpio -i
	tar xzvf newt-$(newtv).tar.gz
	patch -p0< newt-ytht.patch
	tar -cvpzf newt-$(newtv).tar.gz newt-$(newtv)
	rpmbuild -tb newt-$(newtv).tar.gz
	rpm -Uvh /usr/src/redhat/RPMS/i386/newt-*.rpm
#Download libghthash from http://www.ipd.bth.se/ska/sim_home/libghthash.html
libghthash-$(ghthashv).tar.gz:
	wget http://www.ipd.bth.se/ska/sim_home/filer/$@
installghthash:libghthash-$(ghthashv).tar.gz
	tar xzvf $^;cd $(^:.tar.gz=);./configure --prefix=/usr;$(MAKE);$(MAKE) install

installhttpd2:$(apache2_s) $(fastcgicvs_s)
	rm -fr $(apache2);tar xzvf $(apache2_s)
	cd $(apache2);NOTEST_CFLAGS="-O2 -funroll-loops -ffast-math -malign-double -mcpu=$(CPU) -march=$(CPU) -fomit-frame-pointer -fno-exceptions" ./configure \
	--disable-autoindex --disable-imap --disable-include \
	--disable-asis --disable-actions --disable-status \
	--disable-userdir --disable-negotiation \
	--enable-rewrite --with-mpm=worker;$(MAKE);$(MAKE) install
	rm -fr $(fastcgicvs);tar xzvf $(fastcgicvs_s)
	cd $(fastcgicvs);cp Makefile.AP2 Makefile;$(MAKE);$(MAKE) install
#	cp httpd2.conf /usr/local/apache2/conf/httpd.conf
