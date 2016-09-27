#!/bin/bash

./nginx -p $(pwd) -c conf/nginx.conf
while true;
do
	sleep 1
	curpid=`cat logs/nginx.pid`
	existed=`ps -ef|grep "nginx"|grep "$curpid"|grep -v grep`
	if [[ $existed == 0 ]];
	then
		./nginx -p $(pwd) -c conf/nginx.conf
		sleep 10
	fi
done
