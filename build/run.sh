#!/bin/bash
export LD_LIBRARY_PATH=$(pwd):/usr/local/lib:$BOOST_LIB_PATH:lib:$PATH
#export TCMALLOC_PAGE_FENCE=1
count=1
while [ $count -le 100 ]; do
    rm *.log
    echo "---runing---$count times---"
    $1
    if [[ $? -ne 0 ]]; then
        break
    fi
    count=$((count + 1))
done
