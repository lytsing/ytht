#!/bin/sh
# Run this to generate all the initial makefiles, etc.

#LIBTOOLIZE=${LIBTOOLIZE:-libtoolize}
#LIBTOOLIZE_FLAGS="--copy --force"
ACLOCAL=${ACLOCAL:-aclocal-1.14}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOMAKE=${AUTOMAKE:-automake-1.14}
AUTOMAKE_FLAGS="--add-missing --copy"
AUTOCONF=${AUTOCONF:-autoconf}

ARGV0=$0



run() {
	echo "$ARGV0: running \`$@'"
	$@
}

checkversion () {
	k=`$@ --version|grep 1.4`;
	if [ "x$k" != "x" ]
	then
		echo =======================================================================
		echo $@ is version 1.4, but a newer version is needed \(1.9 for example\)
		echo If you already installed a new version, you can edit this file manually
		echo For example, \"AUTOMAKE=\$\{AUTOMAKE:-automake-1.9\}\"
		echo =======================================================================
		exit
	fi
}

checkversion $ACLOCAL
checkversion $AUTOMAKE

set -e

#run $LIBTOOLIZE $LIBTOOLIZE_FLAGS
run $ACLOCAL $ACLOCAL_FLAGS
run $AUTOHEADER
run $AUTOMAKE $AUTOMAKE_FLAGS
run $AUTOCONF
echo "Now type './configure ...' and 'make' to compile."

