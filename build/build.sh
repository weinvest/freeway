#!/bin/bash
rm -rf `ls |egrep -v '(build.sh|run.sh)'`
export PATH=/Applications/CLion.app/Contents/bin/cmake/bin/:$PATH
cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_INSTALL_PREFIX=. ..
make install
