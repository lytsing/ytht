#!/bin/bash
cd /home/bbs/bbs4lepton
cvs -q update -dP
yesterday=`date -d "7 day ago 00:00" -R`
today=`date -d "00:00" -R`
cvsdate=-d\'${yesterday}\<${today}\'
nicedate=`date -d yesterday +"%d %b %Y %Z (%z)"`
/usr/local/bin/cvs2cl.pl -f /home/bbs/bbs4lepton/cvslog.txt -l "${cvsdate}"
mail bbsbug -s "һ�� CVS ���¼�¼"< /home/bbs/bbs4lepton/cvslog.txt
