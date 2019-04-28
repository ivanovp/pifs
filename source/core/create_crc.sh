#!/bin/sh
# CRC8H2F: CRC8 0x2F polynomial
# CRC-8: 0xEA polynomial, good for data word up to 85 bytes (HD=3)
# 0xE7 polynomial, good for up to 247 bytes (HD=3)
PYCRC=../../../pycrc/pycrc.py
CRC_MODEL="--width=8 --poly=0xe7 --reflect-in=0 --reflect-out=0 --xor-out=0 --xor-in=0"
$PYCRC $CRC_MODEL --algorithm table-driven --generate c -o pifs_crc8.c
$PYCRC $CRC_MODEL --algorithm table-driven --generate h -o pifs_crc8.h

