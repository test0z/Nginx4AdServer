#!/bin/bash

getNestedNamespaceNum(){
	declare OLD_IFS
	declare num
	OLD_IFS=$IFS
	IFS="."
	num=0
	for i in $1;
	do
		let "num=num+1";
	done;
	IFS=$OLD_IFS;
	echo $num;
}

AVDL_SRC=$1
NAMESPACE=$2
OUTPUT_DIR=$3
NESTED_NAMESPACE=$(getNestedNamespaceNum $NAMESPACE);
OUTPUT=${AVDL_SRC/avdl/h}
NAMESPACE=${NAMESPACE//./#::#}
paddingstring=
for ((i=1;i<$NESTED_NAMESPACE;i++));
do
	paddingstring+=}
done
java -jar avro-tools-1.8.0.jar idl $AVDL_SRC __tmp.avpr
cat __tmp.avpr |awk -F'#' 'BEGIN{start=0;end=0;}{if(start==1&&end==0){if(!match($1,/"messages"/))printf("%s\n",$1);else end=1;};if(start==0&&match($1,/types.*:(.*)/,arr)){start=1;printf("%s\n",arr[1]);};}' |sed -e '${s/,//g}' > __tmp.json
avrogencpp -i __tmp.json -o $OUTPUT -n $NAMESPACE
SEDOPTION="1,30s/#::#/{\nnamespace /g;s/#::#/::/g;s/namespace avro/${paddingstring}\nnamespace avro/g"
echo $SEDOPTION
sed -i "$SEDOPTION" $OUTPUT

TARGET_AVSC=${AVDL_SRC/avdl/avsc}
TARGET_AVSC=${TARGET_AVSC##*/}
JAVA_PROJ_DIR=../mttyJava/src/main/avro
#mv __tmp.avpr $OUTPUT_DIR/
#mv __tmp.json $OUTPUT_DIR/
rm __tmp.avpr
rm __tmp.json
#mv  __tmp.json $JAVA_PROJ_DIR/$TARGET_AVSC 1>/dev/null 2>&1
mv $OUTPUT $OUTPUT_DIR/
