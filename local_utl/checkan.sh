#!/bin/sh
#����ű�ͨ���ȽϾ��������ݵİ��Ĵ�С�;���������İ��Ĵ�С �ҳ����ɵĿ��ܺ���
#�϶������ľ�����Ŀ¼
anpath=/home/bbs/ftphome/root/pub/X
backuppath=/root/bbsbak/backup
dn=`date +%a`
for i in $anpath/*.tgz;do
	pksize=`ls -l $i|awk '{print $5}'`
	board=`basename $i|cut -d. -f1`
	bksize=`ls -l $backuppath/$dn/ytht.0An.$board.$dn.tgz|awk '{print $5}'`
	hide=`expr $bksize - $pksize`
	if [ $hide -gt 10000000 ] ; then
		echo $board $pksize $bksize
	fi
done
