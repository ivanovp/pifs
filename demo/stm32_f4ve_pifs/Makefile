APP_NAME = stm32_f4ve_pifs

ENABLE_CROSS_COMPILE = 1

# Tool chain settings
ifeq ($(ENABLE_CROSS_COMPILE),1)
#CROSS = $(HOME)/Ac6/SystemWorkbench/plugins/fr.ac6.mcu.externaltools.arm-none.linux64_1.13.1.201705091103/tools/compiler/bin/arm-none-eabi-
CROSS = arm-none-eabi-
CPP = $(CROSS)g++
CC  = $(CROSS)gcc
LD  = $(CROSS)g++
OBJCOPY = $(CROSS)objcopy
endif

INCLUDE =
INCLUDE = -I../../source/core
INCLUDE += -I../../source/flash_drv/stm32_spi_dma
INCLUDE += -I../../source/term
INCLUDE += -I../../source/test
INCLUDE += -I./Drivers/CMSIS/Device/ST/STM32F4xx/Include
INCLUDE += -I./Drivers/CMSIS/Include
INCLUDE += -I./Drivers/STM32F4xx_HAL_Driver/Inc
INCLUDE += -I./Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
INCLUDE += -I./Inc
INCLUDE += -I./Middlewares/Third_Party/FatFs/src
INCLUDE += -I./Middlewares/Third_Party/FatFs/src/drivers
INCLUDE += -I./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
INCLUDE += -I./Middlewares/Third_Party/FreeRTOS/Source/include
INCLUDE += -I./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F

# Options for compiler and linker as well
COMMON_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

# Compiler options
CFLAGS = -c -O0 -ggdb3 $(INCLUDE)
CFLAGS += -fmessage-length=0 -ffunction-sections
CFLAGS += -Wall -Wextra -Wno-format #-pedantic -std=c99
CFLAGS += -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE
CFLAGS += -D__weak="__attribute__((weak))"
CFLAGS += -D__packed="__attribute__((__packed__))"
CFLAGS += -DUSE_HAL_DRIVER
CFLAGS += -DSTM32F407xx
CFLAGS += -DSTM32F4xx
CFLAGS += -DDEBUG=1
CFLAGS += -DFLASH_ENABLE_DMA=1
CFLAGS += -DENABLE_DHT=1
CFLAGS += -DENABLE_DATETIME=1
CFLAGS += $(COMMON_FLAGS)
CPPFLAGS = $(CFLAGS)
CFLAGS_A = $(CFLAGS) -D_ASSEMBLER_ -x assembler-with-cpp

# Linker options
LDFLAGS = $(COMMON_FLAGS)
LDFLAGS += -specs=nosys.specs -specs=nano.specs -TSTM32F407VETx_FLASH.ld -Wl,-Map=output.map -Wl,--gc-sections -lm 

# Set source files
SRC_CPP =
SRC_C =
SRC_C += ../../source/core/pifs.c
SRC_C += ../../source/core/pifs_delta.c
SRC_C += ../../source/core/pifs_dir.c
SRC_C += ../../source/core/pifs_entry.c
SRC_C += ../../source/core/pifs_file.c
SRC_C += ../../source/core/pifs_fsbm.c
SRC_C += ../../source/core/pifs_helper.c
SRC_C += ../../source/core/pifs_map.c
SRC_C += ../../source/core/pifs_merge.c
SRC_C += ../../source/core/pifs_wear.c
SRC_C += ../../source/flash_drv/stm32_spi_dma/flash.c
SRC_C += ../../source/term/parser.c
SRC_C += ../../source/term/term.c
SRC_C += ../../source/test/buffer.c
SRC_C += ../../source/test/flash_test.c
SRC_C += ../../source/test/pifs_test.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_iwdg.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rtc_ex.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
SRC_C += ./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c
SRC_C += ./Middlewares/Third_Party/FatFs/src/diskio.c
SRC_C += ./Middlewares/Third_Party/FatFs/src/drivers/sd_diskio.c
SRC_C += ./Middlewares/Third_Party/FatFs/src/ff.c
SRC_C += ./Middlewares/Third_Party/FatFs/src/ff_gen_drv.c
SRC_C += ./Middlewares/Third_Party/FatFs/src/option/syscall.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/croutine.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/list.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_3.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/queue.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/tasks.c
SRC_C += ./Middlewares/Third_Party/FreeRTOS/Source/timers.c
SRC_C += ./Src/bsp_driver_sd.c
SRC_C += ./Src/dht.c
SRC_C += ./Src/fatfs.c
SRC_C += ./Src/freertos.c
SRC_C += ./Src/isr_handlers.c
SRC_C += ./Src/main.c
SRC_C += ./Src/stm32f4xx_hal_msp.c
SRC_C += ./Src/stm32f4xx_hal_timebase_TIM.c
SRC_C += ./Src/stm32f4xx_it.c
SRC_C += ./Src/system_stm32f4xx.c
SRC_C += ./Src/uart.c
SRC_S = ./startup/startup_stm32f407xx.s
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJ_S   = $(patsubst %.s, %.o, $(SRC_S))
OBJ     = $(OBJ_CPP) $(OBJ_C) $(OBJ_S)
DEP     = $(patsubst %.o, %.d, $(OBJ))

# Compile rules

.PHONY : all

all : $(APP_NAME).elf

$(APP_NAME).elf : $(OBJ)
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
	ctags -R .

