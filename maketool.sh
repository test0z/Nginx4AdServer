#!/bin/bash

toolsrc=$(pwd)/src/adservice/adservice/tool/
mkdir -p adservice.tool
mkdir -p adservice.tool/build
cd adservice.tool/build && cmake $toolsrc && make -j2
