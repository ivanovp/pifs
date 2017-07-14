#!/bin/sh
LOG=test.log
rm flash.bin
rm $LOG
make
for i in `seq --format="%02.0f" 16`; do
    echo "##############################################################################"|tee -a $LOG
    echo "### TEST $i"|tee -a $LOG
    echo "##############################################################################"|tee -a $LOG
    ./pifs|tee -a $LOG
#    hexdump -Cn 32768 flash.bin >$i
done
