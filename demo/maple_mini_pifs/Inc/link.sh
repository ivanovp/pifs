#!/bin/bash
HEADERS="../../../source/core/pifs_merge.h \
../../../source/core/pifs_debug.h \
../../../source/core/pifs.h \
../../../source/core/pifs_dir.h \
../../../source/core/api_pifs.h \
../../../source/core/pifs_file.h \
../../../source/core/flash.h \
../../../source/core/pifs_delta.h \
../../../source/core/pifs_helper.h \
../../../source/core/pifs_wear.h \
../../../source/core/pifs_map.h \
../../../source/core/pifs_entry.h \
../../../source/core/pifs_fsbm.h \
../../../source/core/pifs_config.h \
../../../source/core/pifs_dummy.h \
../../../source/core/common.h \
../../../source/flash_drv/stm32f4xx_spi_dma/flash_config.h \
../../../source/term/term.h \
../../../source/term/parser.h \
../../../source/test/buffer.h \
../../../source/test/flash_test.h \
../../../source/test/pifs_test.h"

#ln -s $i .
for i in $HEADERS; do
    echo $i
    ln -s $i .
done

