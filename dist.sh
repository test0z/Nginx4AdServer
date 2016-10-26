#!/bin/bash

mkdir -p adservice.dist
cp -R conf adservice.dist
rm -rf logs/*.log
cp -R logs adservice.dist
cp -R res  adservice.dist
cp -R run.sh adservice.dist
cp objs/nginx adservice.dist/
tar -cvzf adservice.tgz adservice.dist
