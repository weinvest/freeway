#!/bin/bash
export LD_LIBRARY_PATH=$(pwd):/usr/local/lib:$BOOST_LIB_PATH:lib:$PATH
rm *.log
$1
