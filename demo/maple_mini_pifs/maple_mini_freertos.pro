TEMPLATE = app
CONFIG += console
CONFIG -= qt

include(other.pro)
SOURCES += ./STM32F103CBTx_FLASH.ld \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_iwdg.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c \
./Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c \
./Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
./Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
./Middlewares/Third_Party/FreeRTOS/Source/list.c \
./Middlewares/Third_Party/FreeRTOS/Source/queue.c \
./Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
./Middlewares/Third_Party/FreeRTOS/Source/timers.c \
./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c \
./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/port.c \
./Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c \
./Src/defaulttask.c \
./Src/freertos.c \
./Src/isr_handlers.c \
./Src/isr_handlers_asm.s \
./Src/main.c \
./Src/seven_seg.c \
./Src/stm32f1xx_hal_msp.c \
./Src/stm32f1xx_hal_timebase_TIM.c \
./Src/stm32f1xx_it.c \
./Src/system_stm32f1xx.c \
./Src/uart.c \
./startup/startup_stm32f103xb.s

HEADERS += ./Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xb.h \
./Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f1xx.h \
./Drivers/CMSIS/Device/ST/STM32F1xx/Include/system_stm32f1xx.h \
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
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_cortex.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_def.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_dma.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_dma_ex.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_flash.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_flash_ex.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_gpio.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_gpio_ex.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_iwdg.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_pwr.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_rcc.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_rcc_ex.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_tim.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_tim_ex.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_uart.h \
./Drivers/STM32F1xx_HAL_Driver/Inc/Legacy/stm32_hal_legacy.h \
./Inc/FreeRTOSConfig.h \
./Inc/common.h \
./Inc/main.h \
./Inc/seven_seg.h \
./Inc/stm32f1xx_hal_conf.h \
./Inc/stm32f1xx_it.h \
./Inc/uart.h \
./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/FreeRTOS.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/FreeRTOSConfig_template.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/StackMacros.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/croutine.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/deprecated_definitions.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/event_groups.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/list.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/mpu_wrappers.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/portable.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/projdefs.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/queue.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/semphr.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/task.h \
./Middlewares/Third_Party/FreeRTOS/Source/include/timers.h \
./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3/portmacro.h

INCLUDEPATH += .
INCLUDEPATH += ./.settings
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
INCLUDEPATH += ./Debug/Drivers/STM32F1xx_HAL_Driver
INCLUDEPATH += ./Debug/Drivers/STM32F1xx_HAL_Driver/Src
INCLUDEPATH += ./Debug/Middlewares/Third_Party
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/portable
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang
INCLUDEPATH += ./Debug/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3
INCLUDEPATH += ./Drivers/CMSIS
INCLUDEPATH += ./Drivers/STM32F1xx_HAL_Driver
INCLUDEPATH += ./Drivers/CMSIS/Device
INCLUDEPATH += ./Drivers/CMSIS/Include
INCLUDEPATH += ./Drivers/CMSIS/Device/ST
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F1xx
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F1xx/Include
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F1xx/Source
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates
INCLUDEPATH += ./Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc
INCLUDEPATH += ./Drivers/STM32F1xx_HAL_Driver/Inc
INCLUDEPATH += ./Drivers/STM32F1xx_HAL_Driver/Src
INCLUDEPATH += ./Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
INCLUDEPATH += ./Middlewares/Third_Party
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/include
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/portable
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang
INCLUDEPATH += ./Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3

