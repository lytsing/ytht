#!/bin/sh
DIR=0Announce/groups/GROUP_T/self_photo/M1085243614
CLASS="1085028064 1085028072 1085028080 1085035260 1085035280"
#next
LIMIT="0:0 5 6 7\n\
1:1 5 6 7\n\
2:2\n\
3:3 5\n\
4:4 5 6"
cd
mkdir self_photo_vote
k=0
for i in $CLASS;do
	for l in $DIR/M$i/M*;do
		item=`basename $l|tr -d M`
		for j in `echo -e $LIMIT|grep "${k}:"|cut -d: -f2`;do
			mkdir self_photo_vote/$k-$j-$item
		done
	done
	k=`expr $k + 1`
done

