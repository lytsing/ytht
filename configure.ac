dnl ====================================================
dnl  configure.ac for Firebird BBS v3.0K
dnl
dnl ====================================================

AC_INIT
AC_CONFIG_SRCDIR(["include/bbs.h"])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE(ythtbbs, 0.1)
AC_CONFIG_HEADERS([include/sys_config.h])
dnl ------------------------------------------
dnl ---- Checking C compiler              ----
dnl ------------------------------------------
echo " ........................... "
echo " ... testing C Compiler  ... "
echo " ........................... "
AC_PROG_CC
AC_PROG_INSTALL
AC_LANG([C])
AC_PROG_AWK
AC_PROG_LEX
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_YACC
AC_PROG_RANLIB
CFLAGS=""
OPTI_CFLAGS="-O"

dnl ------------------------------------------
dnl ---- Checking system type             ----
dnl ------------------------------------------

echo " ........................... "
echo " ... testing system type ... "
echo " ........................... "


case ${target} in
    *sun-sunos*)   
        systype=SunOS
        OS_DEF=""
#       CC="cc"
#       OPTI_CFLAGS="-O2"
        ;;      

    *sun-solaris*) 
        systype=Solaris
        OS_DEF="-DSOLARIS -DSYSV"
#       CC="gcc"
#       CFLAGS="-traditional -O2"
        LIBS="-lsocket -lnsl -L/usr/ucblib -lucb"
        ;;      

    *-freebsd*|*-openbsd*|*-netbsd*|*-bsdi*)    
        systype=NetBSD_OR_FreeBSD
        OS_DEF="-DBSD44"
#       CC="cc"
#       CFLAGS=""
        LIBS=""
        ;;     

    *hp-hpux*)     
        systype=HP_UX
        OS_DEF="-DHP_UX -DSYSV"
#       CC="gcc"
#       CFLAGS="-traditional -O2"
        LIBS=""
        ;;     

    *ibm-aix*)     
        systype=IBM_AIX
        OS_DEF="-DAIX -DSYSV"
#       CC="cc"
#       CFLAGS="-O"
        LIBS="-lbsd"
        ;;     

    *sgi-irix*)    
        systype=SGI_IRIX
        OS_DEF="-DIRIX -DSYSV"
#       CC="cc"
#       CFLAGS="-O"
        LIBS=""
        ;;     

    *dec-osf*)     
        systype=DEC_OSF
        OS_DEF=""
#       CC="gcc"
#       CFLAGS="-O2"
        LIBS="-lbsd"
        ;;      

    *-cygwin*)                       
        systype=CYGWIN
        OS_DEF="-DCYGWIN"
        EXEC=".exe"
        CC="gcc"
        OPTI_CFLAGS="-O2"
        LIBS="-lcygipc"
	;;

    *-linux*)       
        systype=LINUX
        OS_DEF="-DLINUX"
        OPTI_CFLAGS="-O2"
       ;;

    *)             
        systype=UNKNOWN
        LIBS="-lbsd"
        ;;     
esac

AC_DEFUN([TRY_COMPILE_OPTIONS],[
		TEMP_CFLAGS=${CFLAGS}
        	CFLAGS="${CFLAGS} -$1"
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[],[echo "-$1 is not supported by this gcc version!";if test "ytht$2" != ytht; then CFLAGS="${TEMP_CFLAGS} $2";echo "use $2 instead!";else CFLAGS="${TEMP_CFLAGS}";fi])
		])

if test -e "/proc/cpuinfo" ; then
	intel="i386: 30 32 34 \n\
i486: 40 41 42 43 44 45 47 48 \n\
pentium: 51 52 \n\
pentium-mmx: 54 \n\
pentiumpro: 61 \n\
pentium2: 63 65 66 \n\
pentium3: 67 68 610 611 \n\
pentium4: 150 151 152 "
	amd="i586: 50 51 52 53 \n\
k6: 56 57 \n\
k6-2: 58 \n\
k6-3: 59 \n\
athlon: 61 62 63 64 66 67 68 610 155" 
        CPU_TYPE="known"
        CPU_FAMILY=`cat /proc/cpuinfo | grep "cpu family"| cut -d ':' -f 2|cut -b 2- | head -n 1`
        CPU_MODEL=`cat /proc/cpuinfo |grep -v "model name"| grep model|cut -d ':' -f 2|cut -b 2- | head -n 1`
        CPU_NAME=`cat /proc/cpuinfo | grep "model name"|cut -d ':' -f 2|head -n 1`
        CPU_VENDOR=`cat /proc/cpuinfo | grep "vendor_id"|cut -d ':' -f 2|head -n 1`
	CPU_FAMILY=${CPU_FAMILY}
	CPU_MODEL=${CPU_MODEL}
	CPU_NAME=${CPU_NAME}
	CPU_VENDOR=${CPU_VENDOR}
        if test $CPU_VENDOR = "GenuineIntel" ; then
                CPU_OPTIONS=`echo -e $intel|grep ${CPU_FAMILY}${CPU_MODEL}|cut -d : -f 1`
        fi
        if test $CPU_VENDOR = "AuthenticAMD" ; then
                CPU_OPTIONS=`echo -e $amd|grep ${CPU_FAMILY}${CPU_MODEL}|cut -d : -f 1` 
        fi
        CPU_OPTIONS="$CPU_OPTIONS"
	if test ytht$CPU_OPTIONS = ytht; then
		echo "Unkown CPU!!! Please mail bbsbug@ytht.net your content of /proc/cpuinfo"
	fi
	if cat /proc/cpuinfo|grep 'flags'|grep ' mmx'; then
		TRY_COMPILE_OPTIONS(mmmx)
	fi
	if cat /proc/cpuinfo|grep 'flags'|grep ' sse'; then
		TRY_COMPILE_OPTIONS(msse)
	fi
fi

if test ytht$CPU_OPTIONS = ytht; then
	CPU_OPTIONS="unknown"
fi
echo "Detected CPU: $CPU_OPTIONS"
TFL=$CFLAGS
TRY_COMPILE_OPTIONS("mtune=i686")
if test "ytht$TFL" = "ytht$CFLAGS"; then
	BACKUPCPU=""
	BACKUPARCH=""
else
	BACKUPCPU="-mtune=i686"
	BACKUPARCH="-march=i686"
fi
CFLAGS=$TFL

TRY_COMPILE_OPTIONS("mtune=${CPU_OPTIONS}",${BACKUPCPU})
TRY_COMPILE_OPTIONS("march=${CPU_OPTIONS}",${BACKUPARCH})
TRY_COMPILE_OPTIONS(pipe)

dnl ------------------------------------------
dnl ---- set RPC library option for bbsrf ----
dnl ------------------------------------------
dnl if test ${systype} = "LINUX"  ; then
dnl    RPCSVC=""
dnl else
dnl    RPCSVC="-lrpcsvc"
dnl fi


default_directory="/usr /usr/local"

dnl ------------------------------------------
dnl ---- check library and function ----------
dnl ------------------------------------------

dnl ------------------------------------------
dnl - check for mysql , copy from snort 1.81 -
dnl ------------------------------------------

AC_DEFUN([FAIL_MESSAGE],[
   echo
   echo
   echo "**********************************************"
   echo "  ERROR: unable to find" $1
   echo "  checked in the following places"
   for i in `echo $2`; do
     echo "        $i"
   done
   echo "**********************************************"
   echo
   exit
])

UTIL=
AC_CHECK_FUNCS(forkpty, , [
	AC_CHECK_LIB(util, forkpty, UTIL="-lutil" , 
	AC_MSG_ERROR(You don't have forkpty))])
AC_SUBST(UTIL)

AC_CHECK_FUNCS(mallopt)
AC_CHECK_FUNCS(flock)
AC_CHECK_FUNCS(strcasestr)
AC_CHECK_FUNCS(strnlen)
AC_CHECK_FUNCS(basename)
AC_CHECK_FUNCS(nftw)
AC_CHECK_FUNCS(versionsort)
AC_CHECK_FUNCS(memmem)
AC_CHECK_FUNCS(setproctitle)
AC_CHECK_FUNCS(open_memstream)

AC_ARG_WITH(mysql, [  --with-mysql=DIR        support for mysql])
AC_ARG_WITH(site,  [  --with-site=SITE_NAME    select site configuration], defaults=yes, defaults=no)
if test "no$with_site" = "no" ; then
	with_site="sample"
fi
if test ! -f site/bbsconfig.$with_site; then 
        FAIL_MESSAGE("no such site configuration!", "site/bbsconfig.$with_site")
fi

if test "$with_mysql" != "no"; then
  if test "$with_mysql" = "yes"; then
    mysql_directory="$"; mysql_fail="yes"
  elif test "$with_mysql" = ""; then
    mysql_directory="$default_directory"; mysql_fail="no"
  else
    mysql_directory="$with_mysql"; mysql_fail="yes"
  fi

  AC_MSG_CHECKING("for mysql")

  for i in $mysql_directory; do
    if test -r $i/include/mysql/mysql.h; then
      MYSQL_DIR=$i
      MYSQL_INC_DIR=$i/include/mysql
    elif test -r $i/include/mysql.h; then
      MYSQL_DIR=$i
      MYSQL_INC_DIR=$i/include
    fi
  done

  if test -z "$MYSQL_DIR"; then
    if test "$mysql_fail" != "no"; then
      tmp=""
      for i in $mysql_directory; do
        tmp="$tmp $i/include $i/include/mysql" 
      done
      FAIL_MESSAGE("mysql headers (mysql.h)", $tmp)
    else
      AC_MSG_RESULT(no)
    fi
  else

    for i in lib lib/mysql; do
      str="$MYSQL_DIR/$i/libmysqlclient.so"
      for j in `echo $str`; do
        if test -r $j; then
          MYSQL_LIB_DIR="$MYSQL_DIR/$i"
          break 2
        fi
      done
    done

    if test -z "$MYSQL_LIB_DIR"; then
      if test "$mysql_fail" != "no"; then
        FAIL_MESSAGE("mysqlclient library", 
                     "$MYSQL_DIR/lib $MYSQL_DIR/lib/mysql")
      else
        AC_MSG_RESULT(no)
      fi
    else
      AC_MSG_RESULT(yes)
      CPPFLAGS="${CPPFLAGS} -I${MYSQL_INC_DIR} -DENABLE_MYSQL"
      LIBSQL="-L${MYSQL_LIB_DIR} -lmysqlclient"
    fi
  fi
fi

AC_CHECK_HEADER(pty.h,[
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <pty.h>]], [[int m,s;
 char * line;
 openpty(&m,&s,line,0,0);]])],[CFLAGS="$CFLAGS -DCONF_HAVE_OPENPTY"],[])],[])

AC_CHECK_LIB(iconv, libiconv_open, [WWW_LIBS="${WWW_LIBS} -liconv"])

AC_CHECK_LIB(ghthash, ght_create, [LIBS="${LIBS} -lghthash"
  WWW_CPPFLAGS="${WWW_CPPFLAGS} -DENABLE_GHTHASH"],
  AC_MSG_ERROR(Please install ghthash first. (cd software; make installghthash)))

AC_CHECK_LIB(fcgi, FCGI_Accept, [WWW_LIBS="${WWW_LIBS} -lfcgi"
  WWW_CPPFLAGS="${WWW_CPPFLAGS} -DENABLE_FASTCGI"])

AC_EXEEXT

dnl ------------------------------------------
dnl  default argument,copy from ucd-snmp-4.2.2 
dnl ------------------------------------------

AC_ARG_WITH(defaults, 
[
Miscellaneous:
  --with-defaults          Use configuration of ytht bbs for prompted values.],
    [defaults="yes", with_site="ytht"])

AC_DEFUN([ASK4],[
 echo -n "$2 --> [[$3]] "
 if test "x$defaults" = "xno" ; then read $1;else echo;fi
 $1=${$1:-$3}])

dnl ------------------------------------------
dnl    Asking about local BBS configuration 
dnl ------------------------------------------


echo " .................................. "
echo "   Please fill data for your BBS    "
echo " .................................. "

if test $with_site != "ytht"; then
	. site/bbsconfig.$with_site
	if test ! -e site/seclist.txt -a -f site/seclist.txt.$with_site; then
		ln -s seclist.txt.$with_site site/seclist.txt
	fi
else
	. bbsconfig
	rm site/seclist.txt
fi

ASK4(BBS_USER,USER of BBS,$LAST_BBS_USER)
#ASK4(BBS_UID,UID of BBS,$LAST_BBS_UID)
#ASK4(BBS_GID,GID of BBS,$LAST_BBS_GID)
BBS_UID=`id -u $BBS_USER`
BBS_GID=`id -g $BBS_USER`
if test "no$BBS_UID" = "no"; then
	AC_MSG_ERROR(User '$BBS_USER' doesn't exist.)
fi
ASK4(BBS_HOME,Home directory of BBS,$LAST_BBS_HOME)
ASK4(BBS_ID,The ID of your site,$LAST_BBS_ID)
ASK4(BBS_CNAME,The name of your site,$LAST_BBS_CNAME)
ASK4(BBS_DOMAIN,The domain name of your site,$LAST_BBS_DOMAIN)
ASK4(BBS_IP,The ip address of your site,$LAST_BBS_IP)
ASK4(BBS_LOC,The location of your site,$LAST_BBS_LOC)
ASK4(ADMIN_EMAIL,Email of your bbs administrator,$LAST_ADMIN_EMAIL)
ASK4(MAXUSERS,Maximum number of users,$LAST_MAXUSERS)
ASK4(MAXBOARD,Maximum number of boards,$LAST_MAXBOARD)
ASK4(MAXACTIVE,Maximum number of on-line users,$LAST_MAXACTIVE)
ASK4(MAXACTIVERUN,Runtime Maximum number of on-line users,$LAST_MAXACTIVERUN)
ASK4(BBS_PORT,BBS listen port,$LAST_BBS_PORT)
ASK4(BBS_BIG5_PORT,BBS listen port(BIG5),$LAST_BBS_BIG5_PORT)
if test "$with_mysql" != "no"; then
ASK4(BBS_SQLDB,MySQL DATABASE NAME,$LAST_BBS_SQLDB)
ASK4(BBS_SQLUSER,MySQL USER NAME,$LAST_BBS_SQLUSER)
ASK4(BBS_SQLPASSWD,MySQL USER PASSWORD,$LAST_BBS_SQLPASSWD)
fi
ASK4(CGIPATH,cgi install directory,$LAST_CGIPATH)
ASK4(HTMPATH,html install directory,$LAST_HTMPATH)
ASK4(SMAGIC,the initial mark string for webpage,$LAST_SMAGIC)

echo "........................................"

#CFLAGS="$CFLAGS -D_FILE_OFFSET_BITS=64"
CFLAGS="$CFLAGS -Wall -g"
CTESTFLAGS="-g -pg -static"

dnl ---------------------------------------------------
dnl ---- Substitute system configuration variables ----
dnl ---------------------------------------------------

AC_SUBST(systype)
AC_SUBST(RPCSVC)
AC_SUBST(OS_DEF)
AC_SUBST(CC)
AC_SUBST(CFLAGS)
AC_SUBST(OPTI_CFLAGS)
AC_SUBST(CTESTFLAGS)
AC_SUBST(LIBS)
AC_SUBST(WWW_LIBS)
AC_SUBST(WWW_CPPFLAGS)
AC_SUBST(OS_DEF)
AC_SUBST(BBS_HOME)
AC_SUBST(BBS_USER)
AC_SUBST(BBS_UID)
AC_SUBST(BBS_GID)
AC_SUBST(CSIE_DEF)
AC_SUBST(BBS_CNAME)
AC_SUBST(BBS_ID)
AC_SUBST(BBS_DOMAIN)
AC_SUBST(BBS_IP)
AC_SUBST(BBS_LOC)
AC_SUBST(MAXUSERS)
AC_SUBST(MAXBOARD)
AC_SUBST(MAXACTIVE)
AC_SUBST(MAXACTIVERUN)
AC_SUBST(BBS_PORT)
AC_SUBST(BBS_BIG5_PORT)
AC_SUBST(PTYEXEC)
AC_SUBST(LIBSQL)
AC_SUBST(BBS_SQLDB)
AC_SUBST(BBS_SQLUSER)
AC_SUBST(BBS_SQLPASSWD)
AC_SUBST(CGIPATH)
AC_SUBST(HTMPATH)
AC_SUBST(SMAGIC)
AC_SUBST(ADMIN_EMAIL)
AC_SUBST(EXEEXT)

dnl ------------------------------------------
dnl ---- Output file                      ----
dnl ------------------------------------------

AC_CONFIG_FILES([include/bbsconfig.h Makefile Makefile.Base ythtlib/Makefile \
                 libythtbbs/Makefile src/Makefile local_utl/Makefile \
                 local_utl/usesql.h local_utl/bbsheavywork.sh \
		 local_utl/daily.sh local_utl/MakeIndex3.sh \
		 local_utl/shellscript4bbsuser/yxkdraw.sh nju09/Makefile atthttpd/Makefile \
                 Installbbs.sh mail2bbs/bbsmail_wrap mail2bbs/bbsmail_wrap2 \
                 site/crontab.sample bbshome/etc/bbsstatlog.plt \
		 bbshome/etc/day.plt bbshome/etc/mailcheck \
		 bbshome/ftphome/root/boards/BBSHelp/html/bknhelp.html \
		 bbshome/ftphome/root/boards/BBSHelp/html/bmhelp.html \
		 bbshome/ftphome/root/boards/BBSHelp/html/bwwwhelp.html \
		 bbshome/ftphome/root/boards/BBSHelp/html/ftphelp.html \
		 bbshome/ftphome/root/boards/BBSHelp/html/index.htm \
		 yftpd/ftpmotd include/Makefile tests/Makefile])
AC_CONFIG_COMMANDS([default],[[chmod 700 Installbbs.sh]],[[]])
AC_OUTPUT

cat << EOF

........................................................................
You may review the Makefile now. Then, type 'make' to build your system.
........................................................................

EOF

dnl -------------------- 
dnl END of configure.ac
dnl --------------------

