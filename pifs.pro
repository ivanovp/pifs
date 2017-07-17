TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./Src/buffer.c \
./Src/flash.c \
./Src/flash_test.c \
./Src/main.c \
./Src/parser.c \
./Src/pifs.c \
./Src/pifs_delta.c \
./Src/pifs_dir.c \
./Src/pifs_entry.c \
./Src/pifs_fsbm.c \
./Src/pifs_helper.c \
./Src/pifs_map.c \
./Src/pifs_merge.c \
./Src/pifs_test.c \
./Src/term.c \
./test/append.c \
./test/listdir.c \
./test/seek.c

HEADERS += ./Inc/api_pifs.h \
./Inc/buffer.h \
./Inc/common.h \
./Inc/flash.h \
./Inc/flash_config.h \
./Inc/parser.h \
./Inc/pifs.h \
./Inc/pifs_config.h \
./Inc/pifs_debug.h \
./Inc/pifs_delta.h \
./Inc/pifs_dummy.h \
./Inc/pifs_entry.h \
./Inc/pifs_fsbm.h \
./Inc/pifs_helper.h \
./Inc/pifs_map.h \
./Inc/pifs_merge.h \
./Inc/term.h

INCLUDEPATH += .
INCLUDEPATH += ./Inc
INCLUDEPATH += ./Src
INCLUDEPATH += ./test

