#!/bin/sh
echo "��ѯʱ��Ϊ:" > /home/bbs/bbstmpfs/tmp/sysinfo.$1
date >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
/usr/bin/uptime >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "*****  ������ʹ��״�� (�� KB Ϊ��λ) *****" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
free -t >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
/usr/bin/ipcs -mu >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
/usr/bin/ipcs >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "*****  Ӳ��ʹ��״�� (�� KB Ϊ��λ)  *****" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
df   >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "*****  �ż�����״��  *****" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
/usr/sbin/mailstats >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "*****  Kernel Interface Statistics  *****" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
netstat -i >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "*****  Process ��Ѷһ��  *****" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
ps -aux >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
echo "" >> /home/bbs/bbstmpfs/tmp/sysinfo.$1
