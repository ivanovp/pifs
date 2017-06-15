TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./buffer.c \
./flash.c \
./flash_test.c \
./pifs.c \
./pifs_test.c

HEADERS += ./api_pifs.h \
./buffer.h \
./common.h \
./flash.h \
./flash_config.h \
./pifs.h \
./pifs_config.h \
./pifs_debug.h

INCLUDEPATH += .

