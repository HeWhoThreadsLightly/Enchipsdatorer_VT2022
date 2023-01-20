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
#include "graphicsLib.h"
#include "codepage-437-bmp.h"
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
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim5;
DMA_HandleTypeDef hdma_tim1_up;

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
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char str[81] = { '\0' };
uint16_t str_len = 0;

void timerReset(){
char str[] = "Timer reset\n\r";
	HAL_UART_Transmit(&huart2, (uint8_t*) str, sizeof(str), HAL_MAX_DELAY);
}

int lastLine = -100;
uint32_t profileCount = 0;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim == &htim5){
		str_len = sprintf(str, "Profile %8lu ticks clock\t line %i\r\n", profileCount, lineCount);
		HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
		profileCount = 0;
	}
	return;
	if(htim == &htim5){
		timerReset();
	}else if(htim == &htim1){
		timerReset();
	}
}

const char cornerTopLeft = 201;
const char cornerTopRight = 187;
const char cornerBottomLeft = 200;
const char cornerBottomRight = 188;
const char cornerVertical = 186;
const char cornerHorizontal = 205;

Color getRainbowColor(int h, int v){
	Color c = {(h+v) %0x1000};
	return c;
}

void makeRainbow(){

}

void clearScreen(){

}

void makeBorders(){
	int colums = horiRes / codepage_437.sprite_hori - 1;
	int rows = vertRes / codepage_437.sprite_vert - 1;
	Color background = {0xFFF};
	Color forground = {0};
	renderCharOnGrid(cornerTopLeft, 0, 0, background, forground, &codepage_437);
	renderCharOnGrid(cornerTopRight, colums, 0, background, forground, &codepage_437);
	renderCharOnGrid(cornerBottomLeft, 0, rows, background, forground, &codepage_437);
	renderCharOnGrid(cornerBottomRight, colums, rows, background, forground, &codepage_437);
	for(int i = 1; i < colums; i++){
		renderCharOnGrid(cornerHorizontal, i, 0, background, forground, &codepage_437);
		renderCharOnGrid(cornerHorizontal, i, rows, background, forground, &codepage_437);
	}
	for(int i = 1; i < rows; i++){
		renderCharOnGrid(cornerVertical, 0, i, background, forground, &codepage_437);
		renderCharOnGrid(cornerVertical, colums, i, background, forground, &codepage_437);
	}
	makeRainbow();
}

void runTTY(){
	int init = 0;

	Color background = {0xFFF};
	Color forground = {0};

	int colums = horiRes / codepage_437.sprite_hori - 1;
	int rows = vertRes / codepage_437.sprite_vert - 1;
	int h = 1, v = 1;

	char toPrint;

	char rainbowModeTrigger[] = "rainbow";
	char *rainbowActivationTracker = rainbowModeTrigger;

	while(1){
		HAL_StatusTypeDef resStatus = HAL_UART_Receive(&huart2, (uint8_t*)&toPrint, 1, HAL_MAX_DELAY);
		uint16_t charCode = toPrint;
		str_len = sprintf(str, "\r\nRecived byte %c code %u HAL %u\r\n", toPrint, charCode, (uint16_t)resStatus);
		HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

		if(init == 0){//clear the screen and draw the window border
			init = 1;
			clearScreen();
			makeBorders();
		}


		if(toPrint == 13){//newline
			v++;
			h = 1;
		}else if(toPrint == 127){//backspace
			h--;
			if(h == 0){//beginning of line
				h = colums - 1;
				v--;
			}
			if(v == 0){//beginning of screen
				v = rows - 1;
			}
			renderCharOnGrid(' ', h, v, background, forground, &codepage_437);
		}else{//print char
			renderCharOnGrid(toPrint, h, v, background, forground, &codepage_437);
			h++;
		}
		if(h >= colums){//line wrap
			v++;
			h = 1;
		}
		if(v >= rows){//screen wrap
			v = 1;
		}

		if(toPrint == *rainbowActivationTracker){//replace all black text with rainbow text
			rainbowActivationTracker++;
			str_len = sprintf(str, "\r\nNext activation char %c step %i\r\n", *rainbowActivationTracker, (rainbowActivationTracker - rainbowModeTrigger));
			HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
			if(*rainbowActivationTracker == '\0'){
				rainbowActivationTracker = rainbowModeTrigger;
				makeRainbow();
			}
		}else{
			rainbowActivationTracker = rainbowModeTrigger;
		}
	}
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
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

	char str[81] = { '\0' };
	uint16_t str_len = 0;
	str_len = sprintf(str, "Starting up!\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	vgaSetup(&htim1, &hdma_tim1_up, &hdma_memtomem_dma2_stream0, vgaPin13_Vsync_GPIO_Port, vgaPin13_Vsync_Pin);
	registerHUARTvga(&huart2);

	vgaStart();//start VGA driver
	HAL_Delay(500);//delay rendering for monitor sync

	Color textColor = ColorBlack;
	int y = 10;
	int x = 10;
	y+=codepage_437.sprite_vert;
	//renderString(str, h, v, background, forground)
	renderString("Hi", x, y, ColorTransparant, textColor, &codepage_437);

	y+=codepage_437.sprite_vert;
	renderString("Hello world!", x, y, ColorTransparant, textColor, &codepage_437);
	y+=codepage_437.sprite_vert;
	renderString("Press <any> key for TTY", x, y, ColorTransparant, textColor, &codepage_437);
	y+=codepage_437.sprite_vert;
	renderChar(177, x, y, ColorTransparant, textColor, &codepage_437);

	y+=codepage_437.sprite_vert;//test patterns
	renderChar(201, x, y, ColorTransparant, textColor, &codepage_437);
	renderChar(187, x + codepage_437.sprite_hori, y, ColorTransparant, textColor, &codepage_437);
	renderChar(186, x + 2*codepage_437.sprite_hori, y, ColorTransparant, textColor, &codepage_437);
	y+=codepage_437.sprite_vert;
	renderChar(200, x, y, ColorTransparant, textColor, &codepage_437);
	renderChar(188, x + codepage_437.sprite_hori, y, ColorTransparant, textColor, &codepage_437);

	str_len = sprintf(str, "\r\nDone\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	//makeRainbow();
	//runTTY();
	while (1)
	{
		profileCount++;

		//HAL_Delay(1);
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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 151;
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
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = vgaUpscale-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 3-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 2;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

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

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 7550-1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 10000-1;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

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
  /* DMA2_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);

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

  /*Configure GPIO pin : vgaPin13_Vsync_Pin */
  GPIO_InitStruct.Pin = vgaPin13_Vsync_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(vgaPin13_Vsync_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

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
