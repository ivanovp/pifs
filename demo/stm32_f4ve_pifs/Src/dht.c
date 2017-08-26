/**
 * @file        dht.c
 * @brief       DHT11/DHT22 humidity sensor's implementation
 * @author      Copyright (C) Peter Ivanov, 2017
 *
 * Created      2017-08-25 17:48:53
 * Last modify: 2017-08-25 18:10:42 ivanovp {Time-stamp}
 * Licence:     GPL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_gpio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>

#include "main.h"
#include "common.h"
#include "time.h"
#include "uart.h"
#include "dht.h"

/**
 * This shall match DHT22_Pin!
 * If DHT22_Pin is GPIO_PIN_5 then it should be 5!
 */
#define DHT22_Pin_Num   0

/// Configure data as output
#define CONF_DATA_OUT()     do { \
        DHT22_GPIO_Port->MODER |= GPIO_MODE_OUTPUT_PP << (DHT22_Pin_Num * 2); \
    } while (0);
/// Configure data as input
#define CONF_DATA_IN()      do { \
        DHT22_GPIO_Port->MODER &= ~(GPIO_MODE_OUTPUT_PP << (DHT22_Pin_Num * 2)); \
    } while (0);
#define DATA_IS_HIGH()      (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == GPIO_PIN_SET)
#define DATA_IS_LOW()       (HAL_GPIO_ReadPin(DHT22_GPIO_Port, DHT22_Pin) == GPIO_PIN_RESET)
#define SET_DATA_HIGH()     do { \
      HAL_GPIO_WritePin(DHT22_GPIO_Port, DHT22_Pin, GPIO_PIN_SET); \
    } while (0);
#define SET_DATA_LOW()      do { \
      HAL_GPIO_WritePin(DHT22_GPIO_Port, DHT22_Pin, GPIO_PIN_RESET); \
    } while (0);

#define MAX_DATA_READ_ERROR_COUNT   (8)

uint8_t DHT_dataBuf[5] = { 0 };
bool_t DHT_dataBufIsValid = FALSE;
uint8_t DHT_dataReadErrorCntr = 0;

#pragma GCC push_options
#pragma GCC optimize ("O3")
static void delay_us(uint32_t us)
{
    volatile uint32_t cycles;

    /* Calculate how many cycles necessary according to MCU's clock */
    cycles = (SystemCoreClock / (uint32_t)1000000) * us;

    /* Enable trace and debug blocks */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Zero counter */
    DWT->CYCCNT = 0;
    /* Enable cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    do
    {
    } while (DWT->CYCCNT < cycles);
}
#pragma GCC pop_options

bool_t DHT_init (void)
{
    bool_t ok = FALSE;

    if ((1u << DHT22_Pin_Num) == DHT22_Pin)
    {
        CONF_DATA_OUT(); // DATA is output
        SET_DATA_HIGH();

        DHT_dataReadErrorCntr = 0;

        // first data will be erroneous
        //DHT_read ();

        ok = TRUE;
    }
    else
    {
        /* ERROR: Configure DHT22_Pin_Num! */
    }

    return ok;
}

/**
 * Before using DHT_getTempCelsius() or DHT_getHumPercent() the data
 * should be read using this function.
 * @see DHT_getTempCelsius() DHT_getHumPercent()
 */
bool_t DHT_read (void)
{
    uint16_t i = 0, j = 0;
    uint16_t bitWidth = 0;
    uint16_t timeoutCntr = 0;
    uint8_t data[5] = { 0 }; // 40 bits
    bool_t ok = TRUE;

    taskENTER_CRITICAL();

    CONF_DATA_OUT();
    SET_DATA_LOW();
    HAL_Delay(20); // minimum 18 ms
    SET_DATA_HIGH();
    CONF_DATA_IN();
    
    // Waiting for the DHT to pull-down the data line
#if DHT_TYPE == 22
    /* 30 us is needed for DHT22 */
    delay_us(30);
#else
    /* 12 us is needed for DHT11 */
    delay_us(12);
#endif

    timeoutCntr = 9; // maximum 80 us
    while (DATA_IS_LOW() && timeoutCntr--)
    {
        delay_us(10);
    }
    if (!timeoutCntr)
    {
        // Timeout occurred
        DHT_dataReadErrorCntr++;
        ok = FALSE;
    }
    if (ok)
    {
        timeoutCntr = 12; // maximum 80 us
        while (DATA_IS_HIGH() && timeoutCntr--)
        {
            delay_us(10);
        }
        if (!timeoutCntr)
        {
            // Timeout occurred
            DHT_dataReadErrorCntr++;
            ok = FALSE;
        }
    }
    for (i = 0; i < 5 && ok; i++)
    {
        for (j = 0; j < 8 && ok; j++)
        {
            timeoutCntr = 10; // maximum 50 us
            while (DATA_IS_LOW() && timeoutCntr--)
            {
                delay_us(10);
            }
            if (!timeoutCntr)
            {
                // Timeout occurred
                DHT_dataReadErrorCntr++;
                ok = FALSE;
            }
            bitWidth = 0;
            timeoutCntr = 9; // maximum 80 us
            while (DATA_IS_HIGH() && timeoutCntr--)
            {
                delay_us(10);
                bitWidth++;
            }
            if (!timeoutCntr)
            {
                // Timeout occurred
                DHT_dataReadErrorCntr++;
                ok = FALSE;
            }
            data[i] <<= 1;
            // If pulse width is greater than 40 us (typically 80 us), we received ONE.
            // Otherwise we received ZERO.
            if (bitWidth >= 4)
            {
                // Bit ONE received
                data[i] |= 1;
            }
        }
    }
    if (ok)
    {
        if (data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0 && data[4] == 0)
        {
            // no data was received
            DHT_dataReadErrorCntr++;
            ok = FALSE;
        }
    }
    if (ok)
    {
        if (((data[0] + data[1] + data[2] + data[3]) & 0xFF) == data[4])
        {
            // Checksum is OK
            DHT_dataBufIsValid = TRUE;
            DHT_dataReadErrorCntr = 0;
            // Copy buffer
            for (i = 0; i < 5; i++)
            {
                DHT_dataBuf[i] = data[i];
            }
            ok = TRUE;
        }
        else
        {
            DHT_dataReadErrorCntr++;
            ok = FALSE;
        }
    }

    taskEXIT_CRITICAL();

    return ok;
}

/**
 * Read temperature from DHT.
 * Before using this function the data should be read using DHT_read() function.
 * @see DHT_read()
 *
 * @return Temperature in Celsius or DHT_ERROR if sensor cannot be read.
 */
float DHT_getTempCelsius (void)
{
    float temp = DHT_ERROR;

    //DHT_read ();
    if (DHT_dataBufIsValid && DHT_dataReadErrorCntr < MAX_DATA_READ_ERROR_COUNT)
    {
#if (DHT_TYPE == 22)
        temp = ((uint16_t) DHT_dataBuf[2] & 0x7F) << 8;
        temp += DHT_dataBuf[3];
        temp /= 10.0f;
        if (DHT_dataBuf[2] & 0x80)
        {
            temp *= -1;
        }
#elif (DHT_TYPE == 11)
        temp = DHT_dataBuf[2];
#endif // DHT_TYPE
    }
    else
    {
        DHT_dataBufIsValid = FALSE;
    }
    return temp;
}

/**
 * Read realative humidity from DHT.
 * Before using this function the data should be read using DHT_read() function.
 * @see DHT_read()
 *
 * @return Relative humidity in percent or DHT_ERROR if sensor cannot be read.
 */
float DHT_getHumPercent (void)
{
    float humidity = DHT_ERROR;

    //DHT_read ();
    if (DHT_dataBufIsValid && DHT_dataReadErrorCntr < MAX_DATA_READ_ERROR_COUNT)
    {
#if (DHT_TYPE == 22)
        humidity = ((uint16_t) DHT_dataBuf[0]) << 8;
        humidity += DHT_dataBuf[1];
        humidity /= 10.0f;
#elif (DHT_TYPE == 11)
        humidity = DHT_dataBuf[0];
#endif // DHT_TYPE
    }
    else
    {
        DHT_dataBufIsValid = FALSE;
    }
    return humidity;
}

