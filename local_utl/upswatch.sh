#!/bin/sh
#���ü��������Ƿ�ͨ�����ж��Ƿ�ͣ�� ���ͣ�� ���Զ�ֹͣbbs���� �ȴ�����
#���������ص�ַ�ͷ�����Ϣ ������޸����
GATE=162.105.31.1
SERVICE="syslog crond inet named sendmail httpd proftpd mysql atd"
BBSSERV="bbsd bbs yftpd bbsinnd bbspop3d innbbsd"
BBSUSER=bbs
#internal variable
n=60
STOP=0

function stopbbs() {
	#ֹͣ����bbs�������
	for i in $BBSSERV;do
		killall $i #Ϊ�˷�ֹ���� ���kill
		sleep 2
		killall -9 $i
	done
	#ֹͣ����ϵͳ����
	for i in $SERVICE;do
		/etc/rc.d/init.d/$i stop #Ϊ�˷�ֹ���� ���ֹͣ
		sleep 2
		/etc/rc.d/init.d/$i stop
		sleep 2
		/etc/rc.d/init.d/$i stop
	done
	#ֹͣ������bbs�û�������еĳ��� �����ڵ��� �鵥�ʵ�
	for i in `ps -u $BBSUSER|awk '{print $1}'`;do
		kill $i
		sleep 2
		kill -9 $i
	done
	#umount���з��� ͬʱ��/ mountΪֻ�� �ȴ�ͣ��
	umount -a
	sync;sleep 2;sync;sleep 2;sync
	mount / -o remount,ro
}

function startbbs() {
	#mount�ļ�ϵͳ
	mount / -o remount,rw
	sleep 1
	mount -a
	#����ϵͳ���� ע����Ϊǰ��stopbbsʱ��shm��û����� ����httpd��������bbsd��
	for i in $SERVICE;do
		/etc/rc.d/init.d/$i start
		sleep 2
	done
	#������bbs�ķ������ ��Ϊ������Ҫ���û���ͬ ����ֻ��������ֱ��д���ű��ķ�ʽ ˭�ܸĸ�?
	/home/bbs/bin/bbsd
	/home/bbs/bin/bbspop3d
	su bbs -c "/home/bbs/ftphome/yftpd"
	su bbs -c "/home/bbs/bin/bbslogd"
	su bbs -c "/home/bbs/bin/bbsinnd"
	su bbs -c "/home/bbs/innd/innbbsd"
}
#�����������
#==============main function start here=======================================
while true;do
	if ping $GATE -c $n -w $n -n | grep "100% packet loss" &> /dev/null ; then
		if [ $STOP -ne 1 ];then
			STOP=1
			stopbbs
		fi
	else
		if [ $STOP -eq 1 ];then
			STOP=0
			startbbs
		fi
	fi
	sleep 10
done
#==============shell script end===============================================
