TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./Src/buffer.c \
./Src/flash.c \
./Src/flash_test.c \
./Src/pifs.c \
./Src/pifs_test.c

HEADERS += ./Inc/api_pifs.h \
./Inc/buffer.h \
./Inc/common.h \
./Inc/flash.h \
./Inc/flash_config.h \
./Inc/pifs.h \
./Inc/pifs_config.h \
./Inc/pifs_debug.h

