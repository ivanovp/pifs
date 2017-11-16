#!/bin/sh
PYCRC=../../../pycrc/pycrc.py
CRC_MODEL="--width=8 --poly=0x2f --reflect-in=0 --reflect-out=0 --xor-out=0 --xor-in=0"
$PYCRC $CRC_MODEL --algorithm table-driven --generate c -o pifs_crc8.c
$PYCRC $CRC_MODEL --algorithm table-driven --generate h -o pifs_crc8.h

