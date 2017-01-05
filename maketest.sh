#!/bin/bash

testsrc=$(pwd)/src/adservice/adservice/test/
mkdir -p adservice.test
mkdir -p adservice.test/build
cd adservice.test/build && cmake $testsrc && make -j2
