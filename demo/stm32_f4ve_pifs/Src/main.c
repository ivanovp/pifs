/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "fatfs.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "common.h"
#include "uart.h"
#include "term.h"
#include "dht.h"
#include "buffer.h"

#define DISABLE_WATCHDOG_MAGIC  0xDEADBEEF
#define ENABLE_TERMINAL         1
#define ENABLE_FLASH_TEST       0
#define ENABLE_PIFS_TEST        0
#define ENABLE_ERASE_ALL        0
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

RTC_HandleTypeDef hrtc;

SD_HandleTypeDef hsd;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;

UART_HandleTypeDef huart1;

NAND_HandleTypeDef hnand1;

osThreadId defaultTaskHandle;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

osThreadId watchdogTaskHandle;
osThreadId measureTaskHandle;
osMutexDef(printf_mutex);
osMutexId printf_mutex = NULL;
osMutexDef(stdin_mutex);
osMutexId stdin_mutex = NULL;
uint32_t disableWatchdog = 0;
FATFS sd_fat_fs;
char disk_path[4];
DIR dir;
FILINFO filinfo;

extern void defaultTask(void const * argument);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_IWDG_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_RTC_Init(void);
static void MX_FSMC_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

void watchdogTask(void const * arg);
void measureTask(void const * arg);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
static inline void LED_on(int number)
{
    if (number == 2)
    {
        HAL_GPIO_WritePin(D2_GPIO_Port, D2_Pin, GPIO_PIN_RESET);
    }
    else if (number == 3)
    {
        HAL_GPIO_WritePin(D3_GPIO_Port, D3_Pin, GPIO_PIN_RESET);
    }
}

static inline void LED_off(int number)
{
    if (number == 2)
    {
        HAL_GPIO_WritePin(D2_GPIO_Port, D2_Pin, GPIO_PIN_SET);
    }
    else if (number == 3)
    {
        HAL_GPIO_WritePin(D3_GPIO_Port, D3_Pin, GPIO_PIN_SET);
    }
}

static inline void LED_toggle(int number)
{
    if (number == 2)
    {
        HAL_GPIO_TogglePin(D2_GPIO_Port, D2_Pin);
    }
    else if (number == 3)
    {
        HAL_GPIO_TogglePin(D3_GPIO_Port, D3_Pin);
    }
}

uint8_t nand_buf[512];

/**
 * @brief ftest Testing NAND flash wired to TFT connector.
 */
void ftest(void)
{
    static uint32_t cntr = 0;
    NAND_IDTypeDef nand_id;
    HAL_StatusTypeDef stat;
    NAND_AddressTypeDef address;

#if 0
    printf("Reading NAND ID... ");
    stat = HAL_NAND_Read_ID(&hnand1, &nand_id);
    if (stat == HAL_OK)
    {
        printf("Ok.\r\n");
        printf("NAND ID: %02X %02X %02X %02X\r\n",
               nand_id.Maker_Id,
               nand_id.Device_Id,
               nand_id.Third_Id,
               nand_id.Fourth_Id);
    }
    else
    {
        printf("Error: %i\r\n", stat);
    }
#endif
    memset(nand_buf, 0xAA, sizeof(nand_buf));
    address.Block = cntr;
    address.Page = cntr++;
    address.Plane = 0;
    printf("Reading %i pages at BA%i/PA%i/PL%i...",
           sizeof(nand_buf) / hnand1.Config.PageSize,
           address.Block, address.Page, address.Plane);
    stat = HAL_NAND_Read_Page_8b(&hnand1, &address, &nand_buf, sizeof(nand_buf) / hnand1.Config.PageSize);
    if (stat == HAL_OK)
    {
        printf("Ok.\r\n");
        print_buffer(nand_buf, sizeof(nand_buf), 0);
    }
    else
    {
        printf("Error: %i\r\n", stat);
    }
}

#if 0
void HAL_NAND_MspInit(NAND_HandleTypeDef *hnand)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* Peripheral clock enable */
    __HAL_RCC_FSMC_CLK_ENABLE();

    /* FSMC GPIO Configuration
      PE7   ------> FSMC_D4
      PE8   ------> FSMC_D5
      PE9   ------> FSMC_D6
      PE10  ------> FSMC_D7
      PD0   ------> FSMC_D2
      PD1   ------> FSMC_D3
      PD4   ------> FSMC_NOE
      PD5   ------> FSMC_NWE
      PD6   ------> FSMC_NWAIT
      PD7   ------> FSMC_NCE2
      PD11  ------> FSMC_CLE
      PD12  ------> FSMC_ALE
      PD14  ------> FSMC_D0
      PD15  ------> FSMC_D1
      */
    GPIO_InitStruct.Pin = GPIO_PIN_0
            | GPIO_PIN_1
            | GPIO_PIN_4
            | GPIO_PIN_5
            | GPIO_PIN_7
            | GPIO_PIN_11
            | GPIO_PIN_12
            | GPIO_PIN_14
            | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FSMC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7
            | GPIO_PIN_8
            | GPIO_PIN_9
            | GPIO_PIN_10;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}
#endif
/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* Enable division by zero fault */
  SCB->CCR |= 0x10;
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_IWDG_Init();
  MX_SDIO_SD_Init();
  MX_RTC_Init();
  MX_FSMC_Init();

  /* USER CODE BEGIN 2 */
  UART_printf_("\r\n\r\nBoard initialized\r\n");
#if DEBUG == 1
  UART_printf_("DEBUG\r\n");
#else
  UART_printf_("RELEASE\r\n");
#endif
  UART_printf_("Compiled on " __DATE__ " " __TIME__ "\r\n");

  if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))
  {
      UART_printf_("POR/PDR or BOR reset.\r\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
  {
      UART_printf_("Pin reset.\r\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
  {
      UART_printf_("POR/PDR reset.\r\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
  {
      UART_printf_("Software reset.\r\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
  {
      UART_printf_("Independent Watchdog reset.\r\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
  {
      UART_printf_("Window Watchdog reset.\r\n");
  }
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
  {
      UART_printf_("Low Power reset.\r\n");
  }
  __HAL_RCC_CLEAR_RESET_FLAGS();
#if ENABLE_WATCHDOG
  UART_printf_("Watchdog started\r\n");
  HAL_IWDG_Refresh(&hiwdg);
#else
  UART_printf_("Watchdog disabled\r\n");
#endif
  setbuf(stdout, NULL);
  printf("+++ printf test! +++\r\n");

  printf("Initialize DHT22 sensor... ");
  if (DHT_init())
  {
      printf("Done.\r\n");
  }
  else
  {
      printf("Error!\r\n");
  }

  HAL_NAND_Reset(&hnand1);

  ftest();
  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  printf_mutex = osMutexCreate(osMutex(printf_mutex));
  stdin_mutex = osMutexCreate(osMutex(stdin_mutex));
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadDef(watchdogTask, watchdogTask, osPriorityRealtime, 0, 64);
  watchdogTaskHandle = osThreadCreate(osThread(watchdogTask), NULL);
  //osThreadDef(measureTask, measureTask, osPriorityAboveNormal, 0, 512);
  //measureTaskHandle = osThreadCreate(osThread(measureTask), NULL);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
 

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/* IWDG init function */
static void MX_IWDG_Init(void)
{

  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
  hiwdg.Init.Reload = 800;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* RTC init function */
static void MX_RTC_Init(void)
{

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef sDate;

    /**Initialize RTC Only 
    */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initialize RTC and set the Time and Date 
    */
  if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) != 0x32F2){
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR0,0x32F2);
  }

}

/* SDIO init function */
static void MX_SDIO_SD_Init(void)
{

  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 2;

}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{

  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** 
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void) 
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
     PA11   ------> USB_OTG_FS_DM
     PA12   ------> USB_OTG_FS_DP
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, D2_Pin|D3_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT22_GPIO_Port, DHT22_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : D2_Pin D3_Pin */
  GPIO_InitStruct.Pin = D2_Pin|D3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI_CS_Pin */
  GPIO_InitStruct.Pin = SPI_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : DHT22_Pin */
  GPIO_InitStruct.Pin = DHT22_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT22_GPIO_Port, &GPIO_InitStruct);

}

/* FSMC initialization function */
static void MX_FSMC_Init(void)
{
  FSMC_NAND_PCC_TimingTypeDef ComSpaceTiming;
  FSMC_NAND_PCC_TimingTypeDef AttSpaceTiming;

  /** Perform the NAND1 memory initialization sequence
  */
  hnand1.Instance = FSMC_NAND_DEVICE;
  /* hnand1.Init */
  hnand1.Init.NandBank = FSMC_NAND_BANK2;
  hnand1.Init.Waitfeature = FSMC_NAND_PCC_WAIT_FEATURE_ENABLE;
  hnand1.Init.MemoryDataWidth = FSMC_NAND_PCC_MEM_BUS_WIDTH_8;
  hnand1.Init.EccComputation = FSMC_NAND_ECC_DISABLE;
  hnand1.Init.ECCPageSize = FSMC_NAND_ECC_PAGE_SIZE_512BYTE;
  hnand1.Init.TCLRSetupTime = 0;
  hnand1.Init.TARSetupTime = 0;
  /* hnand1.Config */
  hnand1.Config.PageSize = 512;
  hnand1.Config.SpareAreaSize = 16;
  hnand1.Config.BlockSize = 32;
  hnand1.Config.BlockNbr = 2048;
  hnand1.Config.PlaneNbr = 0;
  hnand1.Config.PlaneSize = 0;
  hnand1.Config.ExtraCommandEnable = DISABLE;
  /* ComSpaceTiming */
  ComSpaceTiming.SetupTime = 0;
  ComSpaceTiming.WaitSetupTime = 2;
  ComSpaceTiming.HoldSetupTime = 2;
  ComSpaceTiming.HiZSetupTime = 4;
  /* AttSpaceTiming */
  AttSpaceTiming.SetupTime = 0;
  AttSpaceTiming.WaitSetupTime = 2;
  AttSpaceTiming.HoldSetupTime = 2;
  AttSpaceTiming.HiZSetupTime = 4;

  if (HAL_NAND_Init(&hnand1, &ComSpaceTiming, &AttSpaceTiming) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USER CODE BEGIN 4 */
/**
 * @brief watchdogTask LED blinking and watchdog reloading task.
 * @param arg
 */
void watchdogTask(void const * arg)
{
	(void) arg;

	for ( ; ; )
	{
#if ENABLE_WATCHDOG
        /*
         * Watchdog's timeout is 800 ms
         * Oscillator clock (LSI) = 32 kHz
         * Prescaler = /32
         * Down counter reload value = 800
         * Timeout = 1 / ( 40000 / 32 ) * 800 = 0.8 seconds
         */
        if (disableWatchdog != DISABLE_WATCHDOG_MAGIC)
        {
            /* Feed the watchdog */
            HAL_IWDG_Refresh(&hiwdg);
        }
#endif
        LED_toggle(2);
        osDelay(500);
    }
}

void save_measurement(void)
{
    typedef struct __attribute__((packed))
    {
        uint8_t  hour : 5;
        uint8_t  min : 6;
        uint8_t  sec : 5;
        int16_t  hum;
        int16_t  temp;
        //float    hum;
        //float    temp;
    } meas_t;
    meas_t            meas;
    float             hum;
    float             temp;
    P_FILE          * file;
    size_t            written_bytes;
    HAL_StatusTypeDef stat;
    RTC_DateTypeDef   date;
    RTC_TimeTypeDef   time;
    char              filename[PIFS_FILENAME_LEN_MAX + 1];

    /* First time shall be read */
    stat = HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    if (stat == HAL_OK)
    {
        UART_printf("Hour: %i, min: %i, sec: %i\r\n", time.Hours, time.Minutes, time.Seconds);
    }
    /* After date shall be read */
    stat = HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
    if (stat == HAL_OK)
    {
        UART_printf("Year: %i, month: %i, day: %i\r\n", date.Year, date.Month, date.Date);
    }

    hum = DHT_getHumPercent();
    temp = DHT_getTempCelsius();
    UART_printf("Humidity:    %i.%i%%\r\n", (int)hum, (int)fabsf ((hum - (int)hum) * 10.0f));
    UART_printf("Temperature: %i.%i C\r\n", (int)temp, (int)fabsf ((temp - (int)temp) * 10.0f));
    snprintf(filename, sizeof(filename), "%02i%02i%02i.dat", date.Year, date.Month, date.Date);

    file = pifs_fopen(filename, "a");
    if (file)
    {
        meas.hour = time.Hours;
        meas.min = time.Minutes;
        meas.sec = time.Seconds >> 1;
        //meas.hum = hum;
        //meas.temp = temp;
        meas.hum = (int16_t)(hum * 10.0f);
        meas.temp = (int16_t)(temp * 10.0f);
        written_bytes = pifs_fwrite(&meas, 1, sizeof(meas), file);
        if (written_bytes == sizeof(meas))
        {
            UART_printf("Data saved!\r\n");
        }
        else
        {
            UART_printf("ERROR: Cannot save data. Code: %i\r\n", pifs_errno);
        }
        pifs_fclose(file);
    }
    else
    {
        UART_printf("ERROR: Cannot open file!\r\n");
    }
}

/**
 * @brief measureTask DHT22 measurement task
 * @param arg
 */
void measureTask(void const * arg)
{
    bool_t   measure_ok = FALSE;

    (void) arg;

    UART_printf("Measurement task started\r\n");

    /* Drop first measurement */
    DHT_read();

    for ( ; ; )
    {
        osDelay(10000); /* Measure in every 10 second */
        //osDelay(60000); /* Measure in every minute */
        measure_ok = FALSE;
        LED_on(3);
        if (DHT_read())
        {
            measure_ok = TRUE;
        }
        else
        {
            /* Try again */
            if (DHT_read())
            {
                measure_ok = TRUE;
            }
        }
        if (measure_ok)
        {
            save_measurement();
        }
        LED_off(3);
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void) xTask;

    _print("\r\nStack overflow in task ");
    _print(pcTaskName);
    _print("\r\n");
    while(1)
    {
    }
}
/* USER CODE END 4 */

/* StartDefaultTask function */
void StartDefaultTask(void const * argument)
{
  /* init code for FATFS */
  MX_FATFS_Init();

  /* USER CODE BEGIN 5 */
  (void) argument;
#if ENABLE_TERMINAL
    pifs_status_t ret;
    FRESULT       fres;

    ret = pifs_init();
    if (ret != PIFS_SUCCESS)
    {
        printf("ERROR: Cannot initialize file system: %i\r\n", ret);
    }

    printf("Mounting SD card... ");
    fres = f_mount(&sd_fat_fs, (TCHAR const*)disk_path, 1);
    if (fres == FR_OK)
    {
        printf("Done.\r\n");
        printf("Listing root directory:\r\n");
        fres = f_findfirst(&dir, &filinfo, "", "*");
        while (fres == FR_OK && filinfo.fname[0])
        {
            printf("%-32s %8i\r\n", filinfo.fname, filinfo.fsize);
            fres = f_findnext(&dir, &filinfo);
        }
        printf("Closing directory... ");
        fres = f_closedir(&dir);
        if (fres == FR_OK)
        {
            printf("Done.\r\n");
        }
        else
        {
            printf("ERROR: Cannot close directory: %i\r\n", fres);
        }
        printf("Un-mounting SD card... ");
        fres = f_mount(NULL, (TCHAR const*)disk_path, 0);
        if (fres == FR_OK)
        {
            printf("Done.\r\n");
        }
        else
        {
            printf("ERROR: Cannot unmount SD card: %i\r\n", fres);
        }
    }
    else
    {
        printf("ERROR: Cannot mount SD card: %i\r\n", fres);
    }

    term_init();
    while (1)
    {
        term_task();
    }
#elif ENABLE_FLASH_TEST
    pifs_flash_init();
    flash_test();
    pifs_flash_delete();
#elif ENABLE_PIFS_TEST
#if ENABLE_ERASE_ALL
    pifs_flash_init();
	UART_printf("Erasing all blocks... ");
	if (flash_erase_all() == 0)
	{
	    UART_printf("Done.\r\n");
	}
	else
	{
	    UART_printf("ERROR!\r\n");
	}
	pifs_flash_delete();
#endif
	pifs_init();
	pifs_test();
    pifs_delete();
#endif
	  /* Infinite loop */
    UART_printf("Halting task\r\n");
	for(;;)
	{
	    osDelay(1);
	}
  /* USER CODE END 5 */ 
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
/* USER CODE BEGIN Callback 0 */

/* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
/* USER CODE BEGIN Callback 1 */

/* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    _print("\r\nFatal error in file ");
    _print(file);
    _print("at line 0x");
    _printHex32(line);
    _print("\r\n");
    /* User can add his own implementation to report the HAL error return state */
    while(1)
    {
    }
  /* USER CODE END Error_Handler_Debug */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
