#!/bin/sh
rm flash.bin
make
./pifs
hexdump -Cn 32768 flash.bin >1
echo "##############################################################################"
./pifs
hexdump -Cn 32768 flash.bin >2

