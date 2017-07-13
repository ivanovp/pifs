TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./seek.c \
./Src/buffer.c \
./Src/flash.c \
./Src/flash_test.c \
./Src/pifs.c \
./Src/pifs_delta.c \
./Src/pifs_entry.c \
./Src/pifs_fsbm.c \
./Src/pifs_helper.c \
./Src/pifs_map.c \
./Src/pifs_merge.c \
./Src/pifs_test.c

HEADERS += ./Inc/api_pifs.h \
./Inc/buffer.h \
./Inc/common.h \
./Inc/flash.h \
./Inc/flash_config.h \
./Inc/pifs.h \
./Inc/pifs_config.h \
./Inc/pifs_debug.h \
./Inc/pifs_delta.h \
./Inc/pifs_entry.h \
./Inc/pifs_fsbm.h \
./Inc/pifs_helper.h \
./Inc/pifs_map.h \
./Inc/pifs_merge.h

INCLUDEPATH += .
INCLUDEPATH += ./Inc
INCLUDEPATH += ./Src

