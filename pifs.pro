TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += demo/pc_emu/main.c \
source/core/pifs.c \
source/core/pifs_delta.c \
source/core/pifs_dir.c \
source/core/pifs_entry.c \
source/core/pifs_file.c \
source/core/pifs_fsbm.c \
source/core/pifs_helper.c \
source/core/pifs_map.c \
source/core/pifs_merge.c \
source/core/pifs_wear.c \
source/flash_drv/nor_emu/flash.c \
source/flash_drv/stm32_spi_dma/flash.c \
source/term/parser.c \
source/term/term.c \
source/test/buffer.c \
source/test/flash_test.c \
source/test/pifs_test.c

HEADERS += demo/pc_emu/flash_config.h \
demo/pc_emu/pifs_config.h \
demo/pc_emu/pifs_os.h \
source/core/api_pifs.h \
source/core/common.h \
source/core/flash.h \
source/core/pifs.h \
source/core/pifs_config_template.h \
source/core/pifs_debug.h \
source/core/pifs_delta.h \
source/core/pifs_dir.h \
source/core/pifs_dummy.h \
source/core/pifs_entry.h \
source/core/pifs_file.h \
source/core/pifs_fsbm.h \
source/core/pifs_helper.h \
source/core/pifs_map.h \
source/core/pifs_merge.h \
source/core/pifs_wear.h \
source/term/parser.h \
source/term/term.h \
source/test/buffer.h \
source/test/flash_test.h \
source/test/pifs_test.h

INCLUDEPATH += demo/pc_emu
INCLUDEPATH += source
INCLUDEPATH += source/core
INCLUDEPATH += source/flash_drv
INCLUDEPATH += source/term
INCLUDEPATH += source/test
INCLUDEPATH += source/flash_drv/nor_emu
INCLUDEPATH += source/flash_drv/stm32_spi_dma

