#!/bin/bash
LOG=test.log
rm flash.bin
rm flash.stt
rm $LOG
make clean
#make DEBUG=1
make
# Create a file
./pifs tb staticwear.tst >>$LOG
for i in `seq --format="%02.0f" 986`; do
#for i in `seq --format="%02.0f" 10000`; do
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
    ./pifs ls -b>>$LOG
#    hexdump -Cn 32768 flash.bin >$i
done
# put block information into log
./pifs bi>>$LOG
# put statistics into log
./pifs st>>$LOG

