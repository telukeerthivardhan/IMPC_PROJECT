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
  ******************************************************************************
  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdint.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*
 * Smoothing method:
 *
 * An 8-sample rolling average is used to reduce small ADC fluctuations.
 * The newest ADC value replaces the oldest value in a circular buffer.
 * A running sum is maintained, so the average can be calculated without
 * re-summing all 8 samples every time.
 *
 * Eight samples provide good noise reduction while keeping the LED response
 * smooth and responsive when the potentiometer is turned slowly.
 */
#define SMOOTH_WINDOW       8U

/*
 * Brightness is updated every 10 ms (100 Hz).
 * This keeps the main loop non-blocking and leaves CPU time available
 * for future features such as buttons, UART, or other peripherals.
 */
#define UPDATE_PERIOD_MS    10U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */

/* --- Smoothing variables --- */

static uint16_t smooth_buf[SMOOTH_WINDOW];
static uint8_t  smooth_idx = 0;
static uint32_t smooth_sum = 0;

/* --- Non-blocking update timing --- */

static uint32_t last_update_tick = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);

/* USER CODE BEGIN PFP */

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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */

  /*
   * Start PWM on TIM3 Channel 1.
   *
   * TIM3 is configured with:
   * Prescaler = 71
   * ARR       = 999
   *
   * With a 72 MHz timer clock this produces approximately 1 kHz PWM.
   */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

  /* Start with LED off */
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);

  /*
   * Start ADC in continuous conversion mode.
   *
   * Wokwi's STM32F103C8 simulation does not provide the DMA behavior
   * required by the original implementation, so the ADC value is read
   * directly with a non-blocking poll in the main loop.
   */
  HAL_ADC_Start(&hadc1);

  /*
   * Initialize smoothing buffer to zero.
   *
   * The buffer will fill with real ADC readings during normal operation.
   */
  for (uint8_t i = 0; i < SMOOTH_WINDOW; i++)
  {
      smooth_buf[i] = 0;
  }

  smooth_sum = 0;
  smooth_idx = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /*
     * Non-blocking timing check.
     *
     * The loop does not use HAL_Delay(), so it remains available
     * for future extensions.
     */
    uint32_t now = HAL_GetTick();

    if ((now - last_update_tick) >= UPDATE_PERIOD_MS)
    {
        last_update_tick = now;

        /*
         * Check whether an ADC conversion has completed.
         *
         * Timeout = 0 means the CPU never waits for the ADC.
         * If a conversion is not ready, the code immediately continues.
         */
        if (HAL_ADC_PollForConversion(&hadc1, 0) == HAL_OK)
        {
            /*
             * Read the latest 12-bit ADC value.
             *
             * Expected range:
             * 0    = potentiometer minimum
             * 4095 = potentiometer maximum
             */
            uint16_t raw = (uint16_t)HAL_ADC_GetValue(&hadc1);

            /*
             * Rolling average:
             *
             * 1. Remove the oldest sample from the running sum.
             * 2. Store the new ADC sample.
             * 3. Add the new sample to the running sum.
             * 4. Move to the next position in the circular buffer.
             *
             * This requires only constant-time operations.
             */
            smooth_sum -= smooth_buf[smooth_idx];

            smooth_buf[smooth_idx] = raw;

            smooth_sum += raw;

            smooth_idx++;

            if (smooth_idx >= SMOOTH_WINDOW)
            {
                smooth_idx = 0;
            }

            /*
             * Calculate the average of the last 8 samples.
             */
            uint16_t avg = (uint16_t)(smooth_sum / SMOOTH_WINDOW);

            /*
             * Map:
             *
             * ADC 0    -> PWM 0
             * ADC 4095 -> PWM 999
             *
             * This produces a continuous brightness control.
             */
            uint32_t ccr = ((uint32_t)avg * 999U) / 4095U;

            /*
             * Update TIM3 Channel 1 duty cycle.
             */
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, ccr);
        }
    }

    /*
     * Nothing blocking happens here.
     *
     * The main loop remains available for future functionality such as:
     * buttons, UART, sensors, communication, etc.
     */

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
  ADC_ChannelConfTypeDef sConfig = {0};

  /** Common config
  */
  hadc1.Instance = ADC1;

  /*
   * Single ADC channel.
   */
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;

  /*
   * Continuous conversion keeps ADC conversions running in the background.
   */
  hadc1.Init.ContinuousConvMode = ENABLE;

  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;

  /*
   * Only one ADC channel is configured.
   */
  hadc1.Init.NbrOfConversion = 1;

  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;

  /*
   * 72 MHz / (71 + 1) = 1 MHz timer counter.
   */
  htim3.Init.Prescaler = 71;

  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;

  /*
   * 1 MHz / (999 + 1) = 1 kHz PWM.
   */
  htim3.Init.Period = 999;

  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

  if (HAL_TIMEx_MasterConfigSynchronization(
          &htim3,
          &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(
          &htim3,
          &sConfigOC,
          TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /*
   * DMA interrupt configuration remains generated by CubeMX.
   * The ADC application itself does not use DMA in this Wokwi-compatible
   * implementation.
   */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

  __disable_irq();

  while (1)
  {
  }

  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT

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

  /*
   * User can add implementation to report the file name and line number.
   */

  /* USER CODE END 6 */
}

#endif /* USE_FULL_ASSERT */
