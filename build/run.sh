#!/bin/bash
export LD_LIBRARY_PATH=.:/usr/local/lib:$BOOST_LIB_PATH:lib:$PATH
#export TCMALLOC_PAGE_FENCE=1
count=1
while [ $count -le 1000 ]; do
    rm *.log
    echo "++++++++++++RUNING $count TIMES++++++++++++"
#    $1  | grep ERROR
#    if [[ $? -ne 1 ]]; then
    $1
    if [[ $? -ne 0 ]]; then
        break
    fi
    count=$((count + 1))
done
