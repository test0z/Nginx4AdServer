#!/bin/bash

mkdir -p adservice
cp -R conf adservice
rm -rf logs/*.log
cp -R logs adservice
cp -R res  adservice
cp -R run.sh adservice
cp objs/nginx adservice/
tar -cvzf adservice.tgz adservice
