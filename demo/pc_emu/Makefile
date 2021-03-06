APP_NAME = pifs

ENABLE_CROSS_COMPILE = 0
DEBUG = 1

# Tool chain settings
ifeq ($(ENABLE_CROSS_COMPILE),1)
CROSS = arm-none-eabi-
CPP = $(CROSS)g++
CC  = $(CROSS)gcc
LD  = $(CROSS)g++
OBJCOPY = $(CROSS)objcopy
endif

INCLUDE = -I.
INCLUDE += -I../../source/core
INCLUDE += -I../../source/flash_drv/nor_emu
INCLUDE += -I../../source/term
INCLUDE += -I../../source/test

# Options for compiler and linker as well
COMMON_FLAGS =

# Compiler options
CFLAGS = -c -O0 -ggdb3 $(INCLUDE)
CFLAGS += -Wall -Wextra -Wno-format #-pedantic -std=c99
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
ifeq ($(DEBUG), 1)
CFLAGS += -DENABLE_SW_TRAP=1
CFLAGS += -DDEBUG=1
endif
CFLAGS += $(COMMON_FLAGS)
CPPFLAGS = $(CFLAGS)
CFLAGS_A = $(CFLAGS) -D_ASSEMBLER_ -x assembler-with-cpp

# Linker options
LDFLAGS = $(COMMON_FLAGS)
LDFLAGS += -lpthread -lm

# Set source files
SRC_CPP =
SRC_C =
SRC_C += ../../demo/pc_emu/main.c
SRC_C += ../../source/core/pifs.c
SRC_C += ../../source/core/pifs_crc8.c
SRC_C += ../../source/core/pifs_delta.c
SRC_C += ../../source/core/pifs_dir.c
SRC_C += ../../source/core/pifs_entry.c
SRC_C += ../../source/core/pifs_file.c
SRC_C += ../../source/core/pifs_fsbm.c
SRC_C += ../../source/core/pifs_helper.c
SRC_C += ../../source/core/pifs_map.c
SRC_C += ../../source/core/pifs_merge.c
SRC_C += ../../source/core/pifs_wear.c
SRC_C += ../../source/flash_drv/nor_emu/flash.c
SRC_C += ../../source/term/parser.c
SRC_C += ../../source/term/term.c
SRC_C += ../../source/test/buffer.c
SRC_C += ../../source/test/flash_test.c
SRC_C += ../../source/test/pifs_test.c
SRC_S = 
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJ_S   = $(patsubst %.s, %.o, $(SRC_S))
OBJ     = $(OBJ_CPP) $(OBJ_C) $(OBJ_S)
DEP     = $(patsubst %.o, %.d, $(OBJ))

# Compile rules

.PHONY : all

all : $(APP_NAME)

$(APP_NAME) : $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS) 

$(APP_NAME).bin: $(APP_NAME)
	$(OBJCOPY) -O binary $< $@

$(OBJ_CPP) : %.o : %.cpp 
	$(CPP) $(CPPFLAGS) -o $@ $<
	@$(CPP) -MM $(CPPFLAGS) -MT $@ $*.cpp > $*.d

$(OBJ_C) : %.o : %.c 
	$(CC) $(CFLAGS) -o $@ $<
	@$(CC) -MM $(CFLAGS) -MT $@ $*.c > $*.d

$(OBJ_S) : %.o : %.s 
	$(CC) $(CFLAGS_A) -o $@ $<
	@$(CC) -MM $(CFLAGS_A) -MT $@ $*.s > $*.d

# Include dependencies
-include $(DEP)

# Clean rules

.PHONY : clean

clean :
	rm -f $(OBJ) $(DEP) $(APP_NAME)

.PHONY: tags
tags:
	ctags -R . ../../source

