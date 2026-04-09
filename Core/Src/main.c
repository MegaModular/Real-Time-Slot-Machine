/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f429i_discovery_sdram.h"
#include "stm32f429i_discovery_lcd.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

CRC_HandleTypeDef hcrc;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

RNG_HandleTypeDef hrng;

SPI_HandleTypeDef hspi5;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;

SDRAM_HandleTypeDef hsdram1;

/* Definitions for updateLCDTask */
osThreadId_t updateLCDTaskHandle;
const osThreadAttr_t updateLCDTask_attributes = {
  .name = "updateLCDTask",
  .stack_size = 4096 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for leverTask */
osThreadId_t leverTaskHandle;
const osThreadAttr_t leverTask_attributes = {
  .name = "leverTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for LEDDisplayTask */
osThreadId_t LEDDisplayTaskHandle;
const osThreadAttr_t LEDDisplayTask_attributes = {
  .name = "LEDDisplayTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for winCalcTask */
osThreadId_t winCalcTaskHandle;
const osThreadAttr_t winCalcTask_attributes = {
  .name = "winCalcTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for handleBetTask */
osThreadId_t handleBetTaskHandle;
const osThreadAttr_t handleBetTask_attributes = {
  .name = "handleBetTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for ButtonSemaphore */
osSemaphoreId_t ButtonSemaphoreHandle;
const osSemaphoreAttr_t ButtonSemaphore_attributes = {
  .name = "ButtonSemaphore"
};
/* USER CODE BEGIN PV */
//States and logic trackers
int canIncreaseBet = 0;
int isAnimationPlaying = 0;
double currentBet = 1;
int canCalcWin = 1;
int didWin = 0;
int isResultDisplayed = 0;
int requestLCDUpdate = 1;

//Variables & logic
const double NUM_LINES = 15;
int ledWins[15];
double balance = 1000;
int wins = -1;

//Buffers for the output.
char outStr1[64];
char outStr2[64];
char outStr3[64];
char outStr4[64];

//Expressed as win% / 100
int winChanceLED = 50;




/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C3_Init(void);
static void MX_LTDC_Init(void);
static void MX_SPI5_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_RNG_Init(void);
void startLCDUpdateTask(void *argument);
void leverTaskk(void *argument);
void LEDDisplayTaskk(void *argument);
void winCalcTaskk(void *argument);
void handleBetTaskk(void *argument);

/* USER CODE BEGIN PFP */
void MX_FREERTOS_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CRC_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */
  /* 1. Initialize the LCD and SDRAM hardware via BSP */
  if (BSP_LCD_Init() != LCD_OK) {
      // If it stays here, there is a hardware initialization issue
      Error_Handler();
  }

  /* 2. Initialize Layer 1 (The foreground) */
  // 0xD0000000 is the start of SDRAM
  BSP_LCD_LayerDefaultInit(1, 0xD0000000);
  BSP_LCD_SelectLayer(1);
  BSP_LCD_DisplayOn();

  /* 3. Clean the slate */
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE); // Set background for text
  BSP_LCD_SetFont(&Font24);              // Use a large, readable font


  uint32_t raw_seed;
  if (HAL_RNG_GenerateRandomNumber(&hrng, &raw_seed) == HAL_OK) {
      srand(raw_seed); // Seed the standard C generator with a hardware random number
  }


  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of ButtonSemaphore */
  ButtonSemaphoreHandle = osSemaphoreNew(1, 1, &ButtonSemaphore_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of updateLCDTask */
  updateLCDTaskHandle = osThreadNew(startLCDUpdateTask, NULL, &updateLCDTask_attributes);

  /* creation of leverTask */
  leverTaskHandle = osThreadNew(leverTaskk, NULL, &leverTask_attributes);

  /* creation of LEDDisplayTask */
  LEDDisplayTaskHandle = osThreadNew(LEDDisplayTaskk, NULL, &LEDDisplayTask_attributes);

  /* creation of winCalcTask */
  winCalcTaskHandle = osThreadNew(winCalcTaskk, NULL, &winCalcTask_attributes);

  /* creation of handleBetTask */
  handleBetTaskHandle = osThreadNew(handleBetTaskk, NULL, &handleBetTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

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

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};
  LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 7;
  hltdc.Init.VerticalSync = 3;
  hltdc.Init.AccumulatedHBP = 14;
  hltdc.Init.AccumulatedVBP = 5;
  hltdc.Init.AccumulatedActiveW = 654;
  hltdc.Init.AccumulatedActiveH = 485;
  hltdc.Init.TotalWidth = 660;
  hltdc.Init.TotalHeigh = 487;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 0;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 0;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg.Alpha = 0;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0;
  pLayerCfg.ImageWidth = 0;
  pLayerCfg.ImageHeight = 0;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg1.WindowX0 = 0;
  pLayerCfg1.WindowX1 = 0;
  pLayerCfg1.WindowY0 = 0;
  pLayerCfg1.WindowY1 = 0;
  pLayerCfg1.PixelFormat = LTDC_PIXEL_FORMAT_ARGB8888;
  pLayerCfg1.Alpha = 0;
  pLayerCfg1.Alpha0 = 0;
  pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg1.FBStartAdress = 0;
  pLayerCfg1.ImageWidth = 0;
  pLayerCfg1.ImageHeight = 0;
  pLayerCfg1.Backcolor.Blue = 0;
  pLayerCfg1.Backcolor.Green = 0;
  pLayerCfg1.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 84-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
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
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 3;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13|NCS_MEMS_SPI_Pin|CSX_Pin|GPIO_PIN_3
                          |OTG_FS_PSO_Pin|GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ACP_RST_GPIO_Port, ACP_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, RDX_Pin|WRX_DCX_Pin|GPIO_PIN_2|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2|GPIO_PIN_3|LD3_Pin|LD4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE2 PE3 PE4 PE5
                           PE6 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5
                          |GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PC13 NCS_MEMS_SPI_Pin CSX_Pin PC3
                           OTG_FS_PSO_Pin PC8 PC11 PC12 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|NCS_MEMS_SPI_Pin|CSX_Pin|GPIO_PIN_3
                          |OTG_FS_PSO_Pin|GPIO_PIN_8|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PF6 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : B1_Pin MEMS_INT1_Pin MEMS_INT2_Pin TP_INT1_Pin */
  GPIO_InitStruct.Pin = B1_Pin|MEMS_INT1_Pin|MEMS_INT2_Pin|TP_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ACP_RST_Pin */
  GPIO_InitStruct.Pin = ACP_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACP_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OC_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BOOT1_Pin PB3 PB4 */
  GPIO_InitStruct.Pin = BOOT1_Pin|GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : TE_Pin */
  GPIO_InitStruct.Pin = TE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RDX_Pin WRX_DCX_Pin PD2 PD4 */
  GPIO_InitStruct.Pin = RDX_Pin|WRX_DCX_Pin|GPIO_PIN_2|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : PG2 PG3 LD3_Pin LD4_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|LD3_Pin|LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_startLCDUpdateTask */
/**
  * @brief  Function implementing the updateLCDTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_startLCDUpdateTask */
void startLCDUpdateTask(void *argument)
{
  /* init code for USB_HOST */
  MX_USB_HOST_Init();
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	  /* 4. Draw Line 1 */




	  if (requestLCDUpdate == 1) {
		  BSP_LCD_Clear(LCD_COLOR_WHITE);

		  requestLCDUpdate = 0;
		  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
		  BSP_LCD_SetFont(&Font16);
		  snprintf(outStr1, sizeof(outStr1), "Balance : %.0f", balance);
		  BSP_LCD_DisplayStringAt(0, 100, (uint8_t*)outStr1, CENTER_MODE);
		  /* 5. Draw Line 2 */

		  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
		  BSP_LCD_SetFont(&Font16);
		  snprintf(outStr2, sizeof(outStr2), "Current Bet : %.0f", currentBet);
		  BSP_LCD_DisplayStringAt(0, 140, (uint8_t*)outStr2, CENTER_MODE);

		  //Only once slots have been spun once.
		  if (wins > -1) {
			  BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			  BSP_LCD_SetFont(&Font16);

			  if (wins == 0) {
				  BSP_LCD_SetTextColor(LCD_COLOR_RED);
				  strcpy(outStr3, "You lost </3");
			  }
			  else {
				  snprintf(outStr3, sizeof(outStr3), "You win : %.0f", wins * currentBet);
			  }

			  BSP_LCD_DisplayStringAt(0, 180, (uint8_t*)outStr3, CENTER_MODE);

			  wins = -1;
		  }
	  }

    osDelay(20);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_leverTaskk */
/**
* @brief Function implementing the leverTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_leverTaskk */
void leverTaskk(void *argument)
{
  /* USER CODE BEGIN leverTaskk */
  /* Infinite loop */
  for(;;)
  {
	  if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {

		//Stuff that happens when pressed
		isAnimationPlaying = 1;
		canCalcWin = 1;
		wins = -1;
		canIncreaseBet = 0;

		//Delay
		osDelay(2000);

		//Stuff that happens after pressed.
		isAnimationPlaying = 0;
		requestLCDUpdate = 1;
		canIncreaseBet = 1;
	  }
	  osDelay(20);
  }
  /* USER CODE END leverTaskk */
}

/* USER CODE BEGIN Header_LEDDisplayTaskk */
/**
* @brief Function implementing the LEDDisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LEDDisplayTaskk */
void LEDDisplayTaskk(void *argument)
{
  /* USER CODE BEGIN LEDDisplayTaskk */
  /* Infinite loop */
  for(;;)
  {
	  if (isAnimationPlaying == 1) {

		  //Anim1
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET); //Led 0
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET); //Led 1
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET); //Led 2
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET); //Led 3
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET); //Led 4
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); //Led 5
			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET); //Led 6
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); //Led 7
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_SET); //Led 8
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_RESET); //Led 9
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); //Led 10
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET); //Led 11
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET); //Led 12
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); //Led 13
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET); //Led 14

	  		osDelay(50);
	  		//Anim2
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET); //Led 0
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET); //Led 1
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET); //Led 2
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET); //Led 3
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET); //Led 4
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); //Led 5
			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET); //Led 6
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET); //Led 7
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_RESET); //Led 8
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_SET); //Led 9
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); //Led 10
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET); //Led 11
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET); //Led 12
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); //Led 13
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET); //Led 14


	  		osDelay(50);
	  		//Turn off
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET); //Led 0
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET); //Led 1
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET); //Led 2
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET); //Led 3
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_RESET); //Led 4
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); //Led 5
			HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET); //Led 6
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); //Led 7
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_RESET); //Led 8
			HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_RESET); //Led 9
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); //Led 10
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET); //Led 11
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_RESET); //Led 12
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET); //Led 13
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET); //Led 14
	  	}
	  else {
	  			isResultDisplayed = 1;
	  			canCalcWin = 1;
	  			if (ledWins[0] == 1) {
	  				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET); //Led 0
	  			}
	  			if (ledWins[1] == 1) {
	  				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET); //Led 1
	  			}
	  			if (ledWins[2] == 1) {
	  				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET); //Led 2
	  			}
	  			if (ledWins[3] == 1) {
	  				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_SET); //Led 3
	  			}
	  			if (ledWins[4] == 1) {
	  				HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET); //Led 4
	  			}
	  			if (ledWins[5] == 1) {
	  				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); //Led 5
	  			}
	  			if (ledWins[6] == 1) {
	  				HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET); //Led 6
	  			}
	  			if (ledWins[7] == 1) {
	  				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET); //Led 7
	  			}
	  			if (ledWins[8] == 1) {
	  				HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_SET); //Led 8
	  			}
	  			if (ledWins[9] == 1) {
	  				HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_SET); //Led 9
	  			}
	  			if (ledWins[10] == 1) {
	  				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); //Led 10
	  			}
	  			if (ledWins[11] == 1) {
	  				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET); //Led 11
	  			}
	  			if (ledWins[12] == 1) {
	  				HAL_GPIO_WritePin(GPIOC, GPIO_PIN_12, GPIO_PIN_SET); //Led 12
	  			}
	  			if (ledWins[13] == 1) {
	  				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET); //Led 13
	  			}
	  			if (ledWins[14] == 1) {
	  				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET); //Led 14
	  			}
	  	}

	      osDelay(20);
  }
  /* USER CODE END LEDDisplayTaskk */
}

/* USER CODE BEGIN Header_winCalcTaskk */
/**
* @brief Function implementing the winCalcTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_winCalcTaskk */
void winCalcTaskk(void *argument)
{
  /* USER CODE BEGIN winCalcTaskk */
  /* Infinite loop */
  for(;;)
  {
	      /*  0  3  6  9   12
	       *  1  4  7  10  13
	       *  2  5  8  11  14
	  	 */
	  	  //CalculateWin if not already calculated and animation is playing.
	  	  if (canCalcWin == 1 && isAnimationPlaying == 1) {
	  		  canCalcWin = 0;
	  		  canIncreaseBet = 0;
	  		  isResultDisplayed = 0;

	  		  balance -= currentBet;
	  		  requestLCDUpdate = 1;


	  		  osDelay(200);

	  		  //Populate leds
	  		  for(int i = 0; i < 15; i++) {
	  			 int randomNumber = rand() % 100;
	  			 //ledWins[i] = 0 ? randomNumber < winChanceLED : 1;
	  			 if (randomNumber < winChanceLED) {
	  				 ledWins[i] = 1;
	  			 }
	  			 else {
	  				 ledWins[i] = 0;
	  			 }
	  		  }
	  		  wins = 0;
	  		  	  /*
	  		  	   *  x  x  x  x  x
	  		  	   *  0  0  0  0  0
	  		  	   *  0  0  0  0  0
	  		  	  */
	  		  	  if ((ledWins[0] == 1 && ledWins[3] == 1 && ledWins[6] == 1 && ledWins[9] == 1 && ledWins[12] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  0  0  0  0
	  		  	   *  x  x  x  x  x
	  		  	   *  0  0  0  0  0
	  		  	  */
	  		  	  if ((ledWins[1] == 1 && ledWins[4] == 1 && ledWins[7] == 1 && ledWins[10] == 1 && ledWins[13] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  0  0  0  0
	  		  	   *  0  0  0  0  0
	  		  	   *  x  x  x  x  x
	  		  	  */
	  		  	  if ((ledWins[2] == 1 && ledWins[5] == 1 && ledWins[8] == 1 && ledWins[11] == 1 && ledWins[14] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  x  0  x  0  x
	  		  	   *  0  0  0  0  0
	  		  	   *  0  x  0  x  0
	  		  	  */
	  		  	  if ((ledWins[0] == 1 && ledWins[5] == 1 && ledWins[6] == 1 && ledWins[11] == 1 && ledWins[12] == 1)) {
	  		  		  wins++;
	  		  	  }

	  		  	  /*
	  		  	   *  0  x  0  x  0
	  		  	   *  0  0  0  0  0
	  		  	   *  x  0  x  0  x
	  		  	  */
	  		  	  if ((ledWins[2] == 1 && ledWins[3] == 1 && ledWins[8] == 1 && ledWins[9] == 1 && ledWins[14] == 1)) {
	  		  		  wins++;
	  		  	  }

	  		  	  /*
	  		  	   *  0  x  x  x  0
	  		  	   *  0  0  0  0  0
	  		  	   *  x  0  0  0  x
	  		  	  */
	  		  	  if ((ledWins[2] == 1 && ledWins[3] == 1 && ledWins[6] == 1 && ledWins[9] == 1 && ledWins[14] == 1)) {
	  		  		  wins++;
	  		  	  }

	  		  	  /*
	  		  	   *  x  0  0  0  x
	  		  	   *  0  0  0  0  0
	  		  	   *  0  x  x  x  0
	  		  	  */
	  		  	  if ((ledWins[0] == 1 && ledWins[5] == 1 && ledWins[8] == 1 && ledWins[11] == 1 && ledWins[12] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  x  0  0  0  0
	  		  	   *  0  x  x  x  0
	  		  	   *  0  0  0  0  x
	  		  	  */
	  		  	  if ((ledWins[0] == 1 && ledWins[4] == 1 && ledWins[7] == 1 && ledWins[10] == 1 && ledWins[14] == 1)) {
	  		  		  wins++;
	  		  	  }

	  		  	  /*
	  		  	   *  0  0  0  0  x
	  		  	   *  0  x  x  x  0
	  		  	   *  x  0  0  0  0
	  		  	  */
	  		  	  if ((ledWins[2] == 1 && ledWins[4] == 1 && ledWins[7] == 1 && ledWins[10] == 1 && ledWins[12] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  0  0  0  0
	  		  	   *  x  0  0  0  x
	  		  	   *  0  x  x  x  0
	  		  	  */
	  		  	  if ((ledWins[1] == 1 && ledWins[5] == 1 && ledWins[8] == 1 && ledWins[11] == 1 && ledWins[13] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  x  0  0  0  x
	  		  	   *  0  x  x  x  0
	  		  	   *  0  0  0  0  0
	  		  	  */
	  		  	  if ((ledWins[0] == 1 && ledWins[4] == 1 && ledWins[7] == 1 && ledWins[10] == 1 && ledWins[12] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  0  0  0  0
	  		  	   *  0  x  x  x  0
	  		  	   *  x  0  0  0  x
	  		  	  */
	  		  	  if ((ledWins[2] == 1 && ledWins[4] == 1 && ledWins[7] == 1 && ledWins[10] == 1 && ledWins[14] == 1)) {
	  		  		  wins++;
	  		  	  }

	  		  	  /*
	  		  	   *  0  x  x  x  0
	  		  	   *  x  0  0  0  x
	  		  	   *  0  0  0  0  0
	  		  	  */
	  		  	  if ((ledWins[1] == 1 && ledWins[3] == 1 && ledWins[6] == 1 && ledWins[9] == 1 && ledWins[13] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  0  0  x  x
	  		  	   *  0  0  x  0  0
	  		  	   *  x  x  0  0  0
	  		  	  */
	  		  	  if ((ledWins[2] == 1 && ledWins[5] == 1 && ledWins[7] == 1 && ledWins[9] == 1 && ledWins[12] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  x  x  0  0  0
	  		  	   *  0  0  x  0  0
	  		  	   *  0  0  0  x  x
	  		  	  */
	  		  	  if ((ledWins[0] == 1 && ledWins[3] == 1 && ledWins[7] == 1 && ledWins[11] == 1 && ledWins[14] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  x  0  0  0
	  		  	   *  x  0  x  0  x
	  		  	   *  0  0  0  x  0
	  		  	  */
	  		  	  if ((ledWins[1] == 1 && ledWins[3] == 1 && ledWins[7] == 1 && ledWins[11] == 1 && ledWins[13] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  0  0  x  0
	  		  	   *  x  0  x  0  x
	  		  	   *  0  x  0  0  0
	  		  	  */
	  		  	  if ((ledWins[1] == 1 && ledWins[5] == 1 && ledWins[7] == 1 && ledWins[9] == 1 && ledWins[13] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  0  0  0  0
	  		  	   *  x  0  x  0  x
	  		  	   *  0  x  0  x  0
	  		  	  */
	  		  	  if ((ledWins[1] == 1 && ledWins[5] == 1 && ledWins[7] == 1 && ledWins[11] == 1 && ledWins[13] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  x  0  x  0
	  		  	   *  x  0  x  0  x
	  		  	   *  0  0  0  0  0
	  		  	  */
	  		  	  if ((ledWins[1] == 1 && ledWins[3] == 1 && ledWins[7] == 1 && ledWins[9] == 1 && ledWins[13] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  x  0  x  0  x
	  		  	   *  0  x  0  x  0
	  		  	   *  0  0  0  0  0
	  		  	  */
	  		  	  if ((ledWins[0] == 1 && ledWins[4] == 1 && ledWins[6] == 1 && ledWins[10] == 1 && ledWins[12] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  0  0  0  0  0
	  		  	   *  0  x  0  x  0
	  		  	   *  x  0  x  0  x
	  		  	  */
	  		  	  if ((ledWins[2] == 1 && ledWins[4] == 1 && ledWins[8] == 1 && ledWins[10] == 1 && ledWins[14] == 1)) {
	  		  		  wins++;
	  		  	  }

	  		  	  /*
	  		  	   *  0  x  0  x  0
	  		  	   *  0  0  x  0  0
	  		  	   *  x  0  0  0  x
	  		  	  */
	  		  	  if ((ledWins[2] == 1 && ledWins[3] == 1 && ledWins[7] == 1 && ledWins[9] == 1 && ledWins[14] == 1)) {
	  		  		  wins++;
	  		  	  }
	  		  	  /*
	  		  	   *  x  0  0  0  x
	  		  	   *  0  0  x  0  0
	  		  	   *  0  x  0  x  0
	  		  	  */
	  		  	  if ((ledWins[0] == 1 && ledWins[5] == 1 && ledWins[7] == 1 && ledWins[11] == 1 && ledWins[12] == 1)) {
	  		  		  wins++;
	  		  	  }

	  		  	  if (wins > 0) {
	  		  		  balance += currentBet * wins;
	  		  	  }

	  	  }


	      osDelay(20);
  }
  /* USER CODE END winCalcTaskk */
}

/* USER CODE BEGIN Header_handleBetTaskk */
/**
* @brief Function implementing the handleBetTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_handleBetTaskk */
void handleBetTaskk(void *argument)
{
  /* USER CODE BEGIN handleBetTaskk */
  /* Infinite loop */
  for(;;)
  {
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_RESET && canIncreaseBet == 1) {
		currentBet += 1;
		requestLCDUpdate = 1;
		osDelay(1000);
	}
	else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_RESET && currentBet > 1 && canIncreaseBet == 1) {
		currentBet -= 1;
		requestLCDUpdate = 1;
		osDelay(1000);
	}
    osDelay(20);
  }
  /* USER CODE END handleBetTaskk */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM14 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM14) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
