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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
uint32_t stall_timer = 0;
uint32_t fault_recovery_timer = 0;

// Button debounce state
bool start_button_was_pressed = false;
bool reset_button_was_pressed = false;

typedef enum {
	STATE_IDLE,
	STATE_RUNNING,
	STATE_FAULT
} SystemState;

SystemState current_state = STATE_IDLE;

typedef enum {
    FAULT_NONE,
    FAULT_OVERCURRENT,
    FAULT_OVERTEMP,
    FAULT_OVERSPEED,
    FAULT_STALL
} FaultReason;

FaultReason current_fault = FAULT_NONE;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
  * @brief  Reconfigures ADC1 for a single channel and performs one blocking conversion.
  *         Used instead of relying on scan mode cycling all 4 ranks automatically,
  *         to sidestep possible simulator/scan-mode issues in Wokwi.
  * @param  channel: ADC_CHANNEL_x to read
  * @retval Raw 12-bit ADC value (0-4095)
  */
uint32_t Read_ADC_Channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

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
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // Read all 4 channels individually (single-conversion mode per channel)
    uint32_t adc_val[4];
    adc_val[0] = Read_ADC_Channel(ADC_CHANNEL_0);   // PA0
    adc_val[1] = Read_ADC_Channel(ADC_CHANNEL_1);   // PA1
    adc_val[2] = Read_ADC_Channel(ADC_CHANNEL_2);   // PA2
    adc_val[3] = Read_ADC_Channel(ADC_CHANNEL_3);   // PA3

    float current_amp    = (adc_val[3] / 4095.0f) * 3.0f;    // PA3 -> current, 0-3A
    float temp_c         = (adc_val[2] / 4095.0f) * 100.0f;  // PA2 -> temp, 0-100°C
    float speed_rpm      = (adc_val[1] / 4095.0f) * 4000.0f;  // PA1 = pot3 -> speed, scale 0-4000 RPM
    float speed_setpoint = (adc_val[0] / 4095.0f) * 4000.0f;  // PA0 = pot4 -> target RPM
    // Button: Start/Stop toggle (debounced)
    bool start_button_now = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET);
    if (start_button_now && !start_button_was_pressed) {
        // Rising edge detected (button just pressed)
        if (current_state == STATE_IDLE) {
            current_state = STATE_RUNNING;
        } else if (current_state == STATE_RUNNING) {
            current_state = STATE_IDLE;
        }
    }
    start_button_was_pressed = start_button_now;

    // Button: Reset (debounced, only works from FAULT if conditions cleared)
    bool reset_button_now = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10) == GPIO_PIN_RESET);
    if (reset_button_now && !reset_button_was_pressed) {
        if (current_state == STATE_FAULT && current_fault == FAULT_NONE) {
            // Only allow reset if fault has actually cleared via hysteresis
            current_state = STATE_IDLE;
            stall_timer = 0;
        }
    }
    reset_button_was_pressed = reset_button_now;
    // Fault check: overcurrent + overtemp + overspeed (only matters while RUNNING)
    // Fault check with hysteresis: only trip on crossing ABOVE threshold, clear on dropping BELOW lower threshold
    if (current_state == STATE_RUNNING) {
        // Overcurrent
        if (current_fault != FAULT_OVERCURRENT && current_amp > 2.5f) {
            current_state = STATE_FAULT;
            current_fault = FAULT_OVERCURRENT;
        }
        // Overtemp
        if (current_fault != FAULT_OVERTEMP && temp_c > 80.0f) {
            current_state = STATE_FAULT;
            current_fault = FAULT_OVERTEMP;
        }
        // Overspeed
        if (current_fault != FAULT_OVERSPEED && speed_rpm > 3500.0f) {
            current_state = STATE_FAULT;
            current_fault = FAULT_OVERSPEED;
        }
    }

    // Fault clear: only when in FAULT and value drops below clear threshold
    if (current_state == STATE_FAULT) {
        bool current_ok = (current_amp < 2.2f);
        bool temp_ok     = (temp_c < 75.0f);
        bool speed_ok     = (speed_rpm < 3300.0f);

        if (current_fault == FAULT_OVERCURRENT && current_ok) {
            current_fault = FAULT_NONE;
        }
        if (current_fault == FAULT_OVERTEMP && temp_ok) {
            current_fault = FAULT_NONE;
        }
        if (current_fault == FAULT_OVERSPEED && speed_ok) {
            current_fault = FAULT_NONE;
        }
    }
    // Auto-recovery: if in FAULT and all conditions have cleared, count up to 10 seconds
    if (current_state == STATE_FAULT) {
        bool current_ok = (current_amp < 2.2f);
        bool temp_ok     = (temp_c < 75.0f);
        bool speed_ok     = (speed_rpm < 3300.0f);

        if (current_ok && temp_ok && speed_ok) {
            fault_recovery_timer += 500;  // increment by loop period (500ms)
            if (fault_recovery_timer >= 10000) {  // 10 seconds
                current_state = STATE_IDLE;
                stall_timer = 0;
                fault_recovery_timer = 0;
            }
        } else {
            fault_recovery_timer = 0;  // reset timer if any condition is still faulted
        }
    }

    // Closed-loop control: runs every loop, regardless of state
    uint32_t pulse_value = 0;  // default: motor off (covers IDLE and FAULT)

    if (current_state == STATE_RUNNING) {
        float pulse_f = (speed_setpoint / 4000.0f) * 999.0f;
        if (pulse_f < 0) pulse_f = 0;
        if (pulse_f > 999) pulse_f = 999;
        pulse_value = (uint32_t)pulse_f;
    }
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse_value);

    // Stall check: only meaningful while RUNNING
    if (current_state == STATE_RUNNING) {
        float pwm_percent = (pulse_value / 999.0f) * 100.0f;
        if (pwm_percent > 20.0f && speed_rpm < 100.0f) {
            stall_timer += 500;
            if (stall_timer >= 2000) {
                current_state = STATE_FAULT;
            }
        } else {
            stall_timer = 0;
        }
    }

    // Fault LED: runs every loop, reflects current state
    if (current_state == STATE_FAULT) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    }
    char msg[150];
    const char* state_name;
    const char* fault_name;

    switch (current_state) {
        case STATE_IDLE:    state_name = "IDLE";    break;
        case STATE_RUNNING: state_name = "RUNNING"; break;
        case STATE_FAULT:   state_name = "FAULT";   break;
        default:             state_name = "UNKNOWN"; break;
    }

    switch (current_fault) {
        case FAULT_NONE:          fault_name = "none";         break;
        case FAULT_OVERCURRENT:   fault_name = "overcurrent";  break;
        case FAULT_OVERTEMP:      fault_name = "overtemp";     break;
        case FAULT_OVERSPEED:     fault_name = "overspeed";    break;
        case FAULT_STALL:         fault_name = "stall";        break;
        default:                  fault_name = "unknown";      break;
    }

    sprintf(msg, "State: %s [%s] | Current: %.2fA | Temp: %.1fC | Speed: %.0fRPM\r\n",
            state_name, fault_name, current_amp, temp_c, speed_rpm);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

    HAL_Delay(500);
    /* USER CODE END 3 */
  }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
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

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  *   Only channel 0 is set up here at init time; Read_ADC_Channel()
  *   reconfigures the active channel on every call before each conversion.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 63;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB1 PB10 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /*Configure GPIO pins : PA0 PA1 PA2 PA3 as analog inputs for ADC1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
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
  /* User can add his own implementation to report the HAL error return state */
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
