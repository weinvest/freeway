#!/bin/bash
export LD_LIBRARY_PATH=$(pwd):/usr/local/lib:$BOOST_LIB_PATH:lib:$PATH

count=1
while [ $count -le 100 ]; do
    rm *.log
    $1
    count=$((count + 1))
done
