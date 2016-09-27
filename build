#!/bin/bash

make -j2
if [ $? = 0 ];
then
	cp objs/nginx ./	
else
	echo "build nginx failed"
fi
