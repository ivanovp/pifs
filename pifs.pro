TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += demo/maple_mini_pifs/STM32F103CBTx_FLASH.ld \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_iwdg.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_spi.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_spi_ex.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/list.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/queue.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/timers.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_3.c \
demo/maple_mini_pifs/Src/backtrace.c \
demo/maple_mini_pifs/Src/freertos.c \
demo/maple_mini_pifs/Src/isr_handlers.c \
demo/maple_mini_pifs/Src/isr_handlers_asm.s \
demo/maple_mini_pifs/Src/main.c \
demo/maple_mini_pifs/Src/stm32f1xx_hal_msp.c \
demo/maple_mini_pifs/Src/stm32f1xx_hal_timebase_TIM.c \
demo/maple_mini_pifs/Src/stm32f1xx_it.c \
demo/maple_mini_pifs/Src/system_stm32f1xx.c \
demo/maple_mini_pifs/Src/uart.c \
demo/maple_mini_pifs/startup/startup_stm32f103xb.s \
source/core/pifs.c \
source/core/pifs_crc8.c \
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

HEADERS += demo/maple_mini_pifs/Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h \
demo/maple_mini_pifs/Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f1xx.h \
demo/maple_mini_pifs/Drivers/CMSIS/Device/ST/STM32F1xx/Include/system_stm32f1xx.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/arm_common_tables.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/arm_const_structs.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/arm_math.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/cmsis_armcc.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/cmsis_armcc_V6.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/cmsis_gcc.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_cm0.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_cm0plus.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_cm3.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_cm4.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_cm7.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_cmFunc.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_cmInstr.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_cmSimd.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_sc000.h \
demo/maple_mini_pifs/Drivers/CMSIS/Include/core_sc300.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_cortex.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_def.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_dma.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_dma_ex.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_flash.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_flash_ex.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_gpio.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_gpio_ex.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_iwdg.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_pwr.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_rcc.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_rcc_ex.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_spi.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_tim.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_tim_ex.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_uart.h \
demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy/stm32_hal_legacy.h \
demo/maple_mini_pifs/Inc/FreeRTOSConfig.h \
demo/maple_mini_pifs/Inc/backtrace.h \
demo/maple_mini_pifs/Inc/common.h \
demo/maple_mini_pifs/Inc/flash_config.h \
demo/maple_mini_pifs/Inc/main.h \
demo/maple_mini_pifs/Inc/pifs_config.h \
demo/maple_mini_pifs/Inc/pifs_os.h \
demo/maple_mini_pifs/Inc/stm32f1xx_hal_conf.h \
demo/maple_mini_pifs/Inc/stm32f1xx_it.h \
demo/maple_mini_pifs/Inc/uart.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/FreeRTOS.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/FreeRTOSConfig_template.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/StackMacros.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/croutine.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/deprecated_definitions.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/event_groups.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/list.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/mpu_prototypes.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/mpu_wrappers.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/portable.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/projdefs.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/queue.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/semphr.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/task.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include/timers.h \
demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/portmacro.h \
source/core/api_pifs.h \
source/core/common.h \
source/core/flash.h \
source/core/pifs.h \
source/core/pifs_config_template.h \
source/core/pifs_crc8.h \
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

INCLUDEPATH += demo/maple_mini_pifs
INCLUDEPATH += demo/maple_mini_pifs/.settings
INCLUDEPATH += demo/maple_mini_pifs/Drivers
INCLUDEPATH += demo/maple_mini_pifs/Inc
INCLUDEPATH += demo/maple_mini_pifs/Middlewares
INCLUDEPATH += demo/maple_mini_pifs/Src
INCLUDEPATH += demo/maple_mini_pifs/startup
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS
INCLUDEPATH += demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS/Device
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS/Include
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS/Device/ST
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS/Device/ST/STM32F1xx
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS/Device/ST/STM32F1xx/Include
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS/Device/ST/STM32F1xx/Source
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates
INCLUDEPATH += demo/maple_mini_pifs/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc
INCLUDEPATH += demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc
INCLUDEPATH += demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Src
INCLUDEPATH += demo/maple_mini_pifs/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/include
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/portable
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang
INCLUDEPATH += demo/maple_mini_pifs/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3
INCLUDEPATH += source
INCLUDEPATH += source/core
INCLUDEPATH += source/flash_drv
INCLUDEPATH += source/term
INCLUDEPATH += source/test
INCLUDEPATH += source/flash_drv/nor_emu
INCLUDEPATH += source/flash_drv/stm32_spi_dma

