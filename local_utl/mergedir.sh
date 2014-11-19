#!/bin/sh
if test ! -d boards/$1;then
	echo "can not find board $1"
	exit
fi
dd if=boards/$1/.DIR of=/tmp/$$.DIR bs=128 count=$2
dd if=boards/$1/.DELETED of=/tmp/$$.DELETED bs=128 count=`expr $3 - 1`
dd if=boards/$1/.DELETED of=/tmp/$$.DIR bs=128 skip=`expr $3 - 1` seek=$2 count=`expr $4 - $3 + 1`
dd if=boards/$1/.DELETED of=/tmp/$$.DELETED bs=128 skip=$4 seek=`expr $3 - 1`
dd if=boards/$1/.DIR of=/tmp/$$.DIR bs=128 skip=$2 seek=`expr $2 + $4 - $3 + 1`
cp boards/$1/.DIR boards/$1/.DIR.bak.$$
mv -f /tmp/$$.DIR boards/$1/.DIR
cp boards/$1/.DELETED boards/$1/.DELETED.bak.$$
mv -f /tmp/$$.DELETED boards/$1/.DELETED
