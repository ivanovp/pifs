TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./STM32F407VETx_FLASH.ld \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c \
./Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
./Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
./Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
./Middlewares/Third_Party/FreeRTOS/Source/list.c \
./Middlewares/Third_Party/FreeRTOS/Source/queue.c \
./Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
./Middlewares/Third_Party/FreeRTOS/Source/timers.c \
./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c \
./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c \
./Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_3.c \
./Src/buffer.c \
./Src/flash.c \
./Src/flash_test.c \
./Src/freertos.c \
./Src/main.c \
./Src/parser.c \
./Src/pifs.c \
./Src/pifs_delta.c \
./Src/pifs_dir.c \
./Src/pifs_entry.c \
./Src/pifs_file.c \
./Src/pifs_fsbm.c \
./Src/pifs_helper.c \
./Src/pifs_map.c \
./Src/pifs_merge.c \
./Src/pifs_test.c \
./Src/pifs_wear.c \
./Src/stm32f4xx_hal_msp.c \
./Src/stm32f4xx_hal_timebase_TIM.c \
./Src/stm32f4xx_it.c \
./Src/system_stm32f4xx.c \
./Src/term.c \
./Src/uart.c \
./startup/startup_stm32f407xx.s

HEADERS += ./Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f407xx.h \
./Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h \
./Drivers/CMSIS/Device/ST/STM32F4xx/Include/system_stm32f4xx.h \
./Drivers/CMSIS/Include/arm_common_tables.h \
./Drivers/CMSIS/Include/arm_const_structs.h \
./Drivers/CMSIS/Include/arm_math.h \
./Drivers/CMSIS/Include/cmsis_armcc.h \
./Drivers/CMSIS/Include/cmsis_armcc_V6.h \
./Drivers/CMSIS/Include/cmsis_gcc.h \
./Drivers/CMSIS/Include/core_cm0.h \
./Drivers/CMSIS/Include/core_cm0plus.h \
./Drivers/CMSIS/Include/core_cm3.h \
./Drivers/CMSIS/Include/core_cm4.h \
./Drivers/CMSIS/Include/core_cm7.h \
./Drivers/CMSIS/Include/core_cmFunc.h \
./Drivers/CMSIS/Include/core_cmInstr.h \
./Drivers/CMSIS/Include/core_cmSimd.h \
./Drivers/CMSIS/Include/core_sc000.h \
./Drivers/CMSIS/Include/core_sc300.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_cortex.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_def.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_dma.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_dma_ex.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_flash.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_flash_ex.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_flash_ramfunc.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_gpio.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_gpio_ex.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_pwr.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_pwr_ex.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_rcc.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_rcc_ex.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_spi.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_tim.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_tim_ex.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_uart.h \
./Drivers/STM32F4xx_HAL_Driver/Inc/Legacy/stm32_hal_legacy.h \
./Inc/FreeRTOSConfig.h \
./Inc/api_pifs.h \
./Inc/buffer.h \
./Inc/common.h \
./Inc/flash.h \
./Inc/flash_config.h \
./Inc/flash_test.h \
./Inc/main.h \
./Inc/parser.h \
./Inc/pifs.h \
./Inc/pifs_config.h \
./Inc/pifs_debug.h \
./Inc/pifs_delta.h \
./Inc/pifs_dir.h \
./Inc/pifs_dummy.h \
./Inc/pifs_entry.h \
./Inc/pifs_file.h \
./Inc/pifs_fsbm.h \
./Inc/pifs_helper.h \
./Inc/pifs_map.h \
./Inc/pifs_merge.h \
./Inc/pifs_test.h \
./Inc/pifs_wear.h \
./Inc/stm32f4xx_hal_conf.h \
./Inc/stm32f4xx_it.h \
./Inc/term.h \
./Inc/uart.h \
./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/FreeRTOS.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/FreeRTOSConfig_template.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/StackMacros.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/croutine.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/deprecated_definitions.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/event_groups.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/list.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/mpu_prototypes.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/mpu_wrappers.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/portable.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/projdefs.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/queue.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/semphr.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/task.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/timers.h \
./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/portmacro.h

INCLUDEPATH += .
INCLUDEPATH += ./Debug
INCLUDEPATH += ./Drivers
INCLUDEPATH += ./Inc
INCLUDEPATH += ./Middlewares
INCLUDEPATH += ./Src
INCLUDEPATH += ./startup
INCLUDEPATH += ./Debug/Drivers
INCLUDEPATH += ./Debug/Middlewares
INCLUDEPATH += ./Debug/Src
INCLUDEPATH += ./Debug/startup
INCLUDEPATH += ./Debug/Drivers/STM32F4xx_HAL_Driver
INCLUDEPATH += ./Debug/Drivers/STM32F4xx_HAL_Driver/Src
INCLUDEPATH += ./Debug/Middlewares/Third_Party
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/portable
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
INCLUDEPATH += ./Drivers/CMSIS
INCLUDEPATH += ./Drivers/STM32F4xx_HAL_Driver
INCLUDEPATH += ./Drivers/CMSIS/Device
INCLUDEPATH += ./Drivers/CMSIS/Include
INCLUDEPATH += ./Drivers/CMSIS/Device/ST
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F4xx
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F4xx/Include
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F4xx/Source
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc
INCLUDEPATH += ./Drivers/STM32F4xx_HAL_Driver/Inc
INCLUDEPATH += ./Drivers/STM32F4xx_HAL_Driver/Src
INCLUDEPATH += ./Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
INCLUDEPATH += ./Middlewares/Third_Party
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/include
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/portable
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F

