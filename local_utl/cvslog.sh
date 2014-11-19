#!/bin/bash
cd /home/bbs/bbs4lepton
cvs -q update -dP
yesterday=`date -d "7 day ago 00:00" -R`
today=`date -d "00:00" -R`
cvsdate=-d\'${yesterday}\<${today}\'
nicedate=`date -d yesterday +"%d %b %Y %Z (%z)"`
/usr/local/bin/cvs2cl.pl -f /home/bbs/bbs4lepton/cvslog.txt -l "${cvsdate}"
mail bbsbug -s "一周 CVS 更新记录"< /home/bbs/bbs4lepton/cvslog.txt
