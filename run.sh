#!/bin/bash
#islocal=`ifconfig|grep "192.168.31."|wc -l`
#if (( $islocal != 0 ));
#then
	cd /opt/adservice
#fi
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/local/lib64:/usr/lib:/usr/lib64

./nginx -p $(pwd) -c conf/nginx.conf
while true;
do
	sleep 10
done
