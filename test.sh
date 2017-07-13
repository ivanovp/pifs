#!/bin/sh
LOG=test.log
rm flash.bin
rm $LOG
make
for i in `seq 4`; do
    ./pifs|tee -a $LOG
    hexdump -Cn 32768 flash.bin >$i
    echo "##############################################################################"|tee -a $LOG
done
