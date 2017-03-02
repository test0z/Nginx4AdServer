#!/bin/bash

#source service_list.sh

for server in e10 e11 e12 e13 ;
do
	dataDir=/tmp/recoverdata/$server
	mkdir -p $dataDir
	copydirfrom $server /opt/adservice/logs/201702* $dataDir/
	echo "copydata from $server ok"
	for d in `ls -d $dataDir/*`;
	do
		rm -rf ./*.bin
		echo "syncchronizing data $d ..."
		locallog_collector $d sync
	done	
	rm -rf $dataDir
done
