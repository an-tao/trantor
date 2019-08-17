#!/bin/bash
cd build/trantor/unittests/

./date_unittest && ./inetaddress_unittest && ./msgbuffer_unittest

if [ $? -ne 0 ];then
    echo "Error in testing"
    exit -1
fi

echo "Everything is ok!"
exit 0