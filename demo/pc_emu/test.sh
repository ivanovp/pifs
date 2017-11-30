#!/bin/bash
LOG=test.log
rm flash.bin
rm flash.stt
rm $LOG
make clean
make
# Create a file
./pifs tb staticwear.tst >>$LOG
#for i in `seq --format="%02.0f" 100`; do
for i in `seq --format="%02.0f" 20`; do
    echo "##############################################################################"|tee -a $LOG
    echo "### TEST $i"|tee -a $LOG
    echo "##############################################################################"|tee -a $LOG
#   ./pifs tp|tee -a $LOG
    ./pifs tp>>$LOG
    # Get return code of command before pipe
    RC=${PIPESTATUS[0]}
    #RC=$?
    if [[ $RC != 0 ]]; then
        tail -50 $LOG
        echo "Error $RC occured during test $i, exiting..."
        exit $RC;
    fi
#    hexdump -Cn 32768 flash.bin >$i
done
./pifs bi>>$LOG

