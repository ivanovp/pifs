#!/bin/sh
SOURCES="../../../source/core/pifs_entry.c \
../../../source/core/pifs.c \
../../../source/core/pifs_file.c \
../../../source/core/pifs_fsbm.c \
../../../source/core/pifs_delta.c \
../../../source/core/pifs_wear.c \
../../../source/core/pifs_helper.c \
../../../source/core/pifs_merge.c \
../../../source/core/pifs_map.c \
../../../source/core/pifs_dir.c \
../../../source/flash_drv/stm32f4xx_spi_dma/flash.c \
../../../source/term/term.c \
../../../source/term/parser.c \
../../../source/test/pifs_test.c \
../../../source/test/buffer.c \
../../../source/test/flash_test.c"

for i in $SOURCES; do
    echo $i
    ln -s $i .
done
