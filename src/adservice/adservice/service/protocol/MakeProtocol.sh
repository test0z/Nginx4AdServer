#!/bin/bash

verbose=0
export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:/usr/lib:/usr/lib64:$LD_LIBRARY_PATH

if [ $# -eq 1 ];
then
	if [ $1 = 'clean' ];
	then
		rm -rf */*.pb.*
		exit 0
	elif [ $1 = '--verbose' ];
	then
		verbose=1	
	fi
fi

for src_dir in `find * -maxdepth 0 -type d`
do
#	echo $src_dir
	if [ -e "$src_dir"/proto ];
	then
		dst_dir="$src_dir"
		error_msg=$(protoc -I="$src_dir"/proto/ --cpp_out="$dst_dir" "$src_dir"/proto/*.proto 2>&1)
		if [[ $? != 0 && ${verbose} == 1 ]];
		then
			echo "error occured processing ${src_dir}:${error_msg}"
		fi
				
	fi
	if [ -e "$src_dir"/avro ];
	then
		namespace="protocol.${src_dir}"
		dst_dir="$src_dir"
		for avdl_file in `ls $src_dir/avro/*.avdl` ;
		do
			error_msg=$(./avdl2cpp.sh $avdl_file $namespace $dst_dir)
			if [[ $? != 0 && ${verbose} == 1 ]];
			then
				echo "error occured processing ${src_dir}:${error_msg}"
			fi
		done
	fi
done

for ccfile in `ls */*.cc`
do
	prefix=`echo ${ccfile}|grep -oP "(.+)(?=.cc)"`
	mv $ccfile ${prefix}.cpp
done
