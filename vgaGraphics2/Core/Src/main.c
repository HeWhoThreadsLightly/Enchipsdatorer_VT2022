/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
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

#include "vga.h"
#include <stdio.h>
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
 TIM_HandleTypeDef htim5;
DMA_HandleTypeDef hdma_tim5_up;
DMA_HandleTypeDef hdma_tim5_ch2;

UART_HandleTypeDef huart2;

DMA_HandleTypeDef hdma_memtomem_dma2_stream0;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM5_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char str[81] = { '\0' };
uint16_t str_len = 0;

void dumpLine(){

	for(int i = 0; i < 40; i++){
		int tmp = screenBuff[i].value;
		str_len = sprintf(str, "%02x ", tmp);
		HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	}
	str_len = sprintf(str, "\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}

void timerReset(){
	char str[] = "Timer reset\n\r";
	HAL_UART_Transmit(&huart2, (uint8_t*) str, sizeof(str), HAL_MAX_DELAY);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim == &htim5){
		timerReset();
	}
}

int printStatus(HAL_StatusTypeDef status){
	/*
	typedef enum
	{
	  HAL_OK       = 0x00U,
	  HAL_ERROR    = 0x01U,
	  HAL_BUSY     = 0x02U,
	  HAL_TIMEOUT  = 0x03U
	} HAL_StatusTypeDef;//*/
	switch(status){
	case HAL_OK:str_len = sprintf(str, "HAL_OK\r\n");break;
	case HAL_ERROR:str_len = sprintf(str, "HAL_ERROR\r\n");break;
	case HAL_BUSY:str_len = sprintf(str, "HAL_BUSY\r\n");break;
	case HAL_TIMEOUT:str_len = sprintf(str, "HAL_TIMEOUT\r\n");break;
	default:str_len = sprintf(str, "HAL_Unknown\r\n");break;
	}
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	return status != HAL_OK;

}

int printState(HAL_DMA_StateTypeDef state){
	//HAL_DMA_STATE_RESET             = 0x00U,  /*!< DMA not yet initialized or disabled */
	//HAL_DMA_STATE_READY             = 0x01U,  /*!< DMA initialized and ready for use   */
	//HAL_DMA_STATE_BUSY              = 0x02U,  /*!< DMA process is ongoing              */
	//HAL_DMA_STATE_TIMEOUT           = 0x03U,  /*!< DMA timeout state                   */
	//HAL_DMA_STATE_ERROR             = 0x04U,  /*!< DMA error state                     */
	//HAL_DMA_STATE_ABORT             = 0x05U,  /*!< DMA Abort state                     */
	//}HAL_DMA_StateTypeDef;
	switch(state){
	case HAL_DMA_STATE_RESET:str_len = sprintf(str, "DMA not yet initialized or disabled\r\n");break;
	case HAL_DMA_STATE_READY:str_len = sprintf(str, "DMA initialized and ready for use\r\n");break;
	case HAL_DMA_STATE_BUSY:str_len = sprintf(str, "DMA process is ongoing\r\n");break;
	case HAL_DMA_STATE_TIMEOUT:str_len = sprintf(str, "DMA timeout state\r\n");break;
	case HAL_DMA_STATE_ERROR:str_len = sprintf(str, "DMA error state\r\n");break;
	case HAL_DMA_STATE_ABORT:str_len = sprintf(str, "DMA Abort state\r\n");break;
	default:str_len = sprintf(str, "DMA_Unknown\r\n");break;

	}
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
	return state != HAL_DMA_STATE_READY;
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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */

	char str[81] = { '\0' };
	uint16_t str_len = 0;
	str_len = sprintf(str, "Starting up!\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);


	vgaSetup(&htim5, &hdma_tim5_up, &hdma_memtomem_dma2_stream0);
	registerHUART(&huart2);

	for(int i = 0; i < vertRes; i++){//load a test pattern
		for(int j = 0; j < horiRes; j++){
			screenBuff[i*vertRes + j].value = j & 0b111111;
		}
	}

	dumpLine();
	printState(HAL_DMA_GetState(&hdma_memtomem_dma2_stream0));

	str_len = sprintf(str, "\r\n\r\nTesting Memset\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	printStatus(old_memSet(0, (uint32_t*)&screenBuff[0], 2));//testingMemset
	printState(HAL_DMA_GetState(&hdma_memtomem_dma2_stream0));
	dumpLine();
	while(printStatus(HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 100))){HAL_Delay(1000);};

	str_len = sprintf(str, "\r\n\r\nTesting Memcopy\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK) {
		Error_Handler();
	}
	printStatus(old_memCopy((uint32_t*)&screenBuff[8], (uint32_t*)&screenBuff[0], 2));//testingMemcopy
	printState(HAL_DMA_GetState(&hdma_memtomem_dma2_stream0));
	dumpLine();
	while(printStatus(HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 100))){HAL_Delay(1000);};

	str_len = sprintf(str, "\r\n\r\nTesting Memset\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK) {
		Error_Handler();
	}
	printStatus(old_memSet(0xff, (uint32_t*)&screenBuff[0], 2));//testingMemset
	printState(HAL_DMA_GetState(&hdma_memtomem_dma2_stream0));
	dumpLine();
	while(printStatus(HAL_DMA_PollForTransfer(&hdma_memtomem_dma2_stream0, HAL_DMA_FULL_TRANSFER, 100))){HAL_Delay(1000);};

	str_len = sprintf(str, "\r\nDone\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	vgaStart();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		HAL_Delay(100);
		str_len = sprintf(str, "%i\r\n", lineCount);
		HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 84-1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 1000-1;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 100;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  HAL_TIM_MspPostInit(&htim5);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_dma2_stream0
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma2_stream0 on DMA2_Stream0 */
  hdma_memtomem_dma2_stream0.Instance = DMA2_Stream0;
  hdma_memtomem_dma2_stream0.Init.Channel = DMA_CHANNEL_0;
  hdma_memtomem_dma2_stream0.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma2_stream0.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma2_stream0.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma2_stream0.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_memtomem_dma2_stream0.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_memtomem_dma2_stream0.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma2_stream0.Init.Priority = DMA_PRIORITY_MEDIUM;
  hdma_memtomem_dma2_stream0.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_memtomem_dma2_stream0.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_HALFFULL;
  hdma_memtomem_dma2_stream0.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_memtomem_dma2_stream0.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK)
  {
    Error_Handler( );
  }

  /* DMA interrupt init */
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, vgaPin0_B0_Pin|vgaPin1_B1_Pin|vgaPin2_B2_Pin|vgaPin3_B3_Pin
                          |vgaPin4_G0_Pin|vgaPin5_G1_Pin|vgaPin6_G2_Pin|vgaPin7_G3_Pin
                          |vgaPin8_R0_Pin|vgaPin9_R1_Pin|vgaPin10_R2_Pin|vgaPin11_R3_Pin
                          |vgaPin12_Hsync_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, vgaPin13_Vsync_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : vgaPin0_B0_Pin vgaPin1_B1_Pin vgaPin2_B2_Pin vgaPin3_B3_Pin
                           vgaPin4_G0_Pin vgaPin5_G1_Pin vgaPin6_G2_Pin vgaPin7_G3_Pin
                           vgaPin8_R0_Pin vgaPin9_R1_Pin vgaPin10_R2_Pin vgaPin11_R3_Pin
                           vgaPin12_Hsync_Pin */
  GPIO_InitStruct.Pin = vgaPin0_B0_Pin|vgaPin1_B1_Pin|vgaPin2_B2_Pin|vgaPin3_B3_Pin
                          |vgaPin4_G0_Pin|vgaPin5_G1_Pin|vgaPin6_G2_Pin|vgaPin7_G3_Pin
                          |vgaPin8_R0_Pin|vgaPin9_R1_Pin|vgaPin10_R2_Pin|vgaPin11_R3_Pin
                          |vgaPin12_Hsync_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : vgaPin13_Vsync_Pin LD2_Pin */
  GPIO_InitStruct.Pin = vgaPin13_Vsync_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
