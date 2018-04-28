#!/bin/bash
rm -rf `ls |egrep -v '(build.sh|run.sh|waitingAnalysis.py)'`
export PATH=/Applications/CLion.app/Contents/bin/cmake/bin/:$PATH
cmake -DCMAKE_BUILD_TYPE=$1 -DCMAKE_INSTALL_PREFIX=. ..
make install
