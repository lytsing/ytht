#!/bin/sh
#this script will do some heavy work such as backup on night
rm -f /root/.gnupg/.#lk*
export DELAYT=400
su bbs -c "sh -x /home/bbs/bin/bbsheavywork.sh"
/root/bin/backup.sh &> /root/bbsbak/bbslog/tarlog
date
sleep $DELAYT
/root/bin/backsource.sh &> /dev/null
