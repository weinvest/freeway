#!/bin/bash
export PATH=/Applications/CLion.app/Contents/bin/cmake/bin/:$PATH
cmake -DCMAKE_INSTALL_PREFIX=. ..
make install
