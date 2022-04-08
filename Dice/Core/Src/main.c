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
 UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int buttonPressed(){
	return 0 == (DiceButton_GPIO_Port->IDR & DiceButton_Pin);
}

int blueButtonPressed(){
	return 0 == (B1_GPIO_Port->IDR & B1_Pin);
}

void setGreenLed(int set){
	volatile uint32_t * ODR = &(LD2_GPIO_Port->ODR);
	*ODR = ((*ODR) & !LD2_Pin) //keep old values on rest of register
			| (set?LD2_Pin:0);//set pin if blue is true
}

void put_on_sseg(unsigned value){
	char * gpioP = (char*)&(DiceLed0_GPIO_Port->ODR);
	const char dieBitSet[] = {
		0b00000000,
		0b01000100,
		0b00111110,
		0b01101110,
		0b01001101,
		0b01101011,
		0b01111011,
		0b01000110,
		0b01111111,
		0b01001111,
		0b11110000
	};
	if(value > 9 ){
		value = 10;
	}
	*gpioP = dieBitSet[value];
}

void put_on_dots(unsigned value){
	switch(value){
	case 1:{
		HAL_GPIO_WritePin(DiceRed0_GPIO_Port, DiceRed0_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed1_GPIO_Port, DiceRed1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed2_GPIO_Port, DiceRed2_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed3_GPIO_Port, DiceRed3_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed4_GPIO_Port, DiceRed4_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed5_GPIO_Port, DiceRed5_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed6_GPIO_Port, DiceRed6_Pin, GPIO_PIN_RESET);
	}break;
	case 2:{
		HAL_GPIO_WritePin(DiceRed0_GPIO_Port, DiceRed0_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed1_GPIO_Port, DiceRed1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed2_GPIO_Port, DiceRed2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed3_GPIO_Port, DiceRed3_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed4_GPIO_Port, DiceRed4_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed5_GPIO_Port, DiceRed5_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed6_GPIO_Port, DiceRed6_Pin, GPIO_PIN_RESET);
	}break;
	case 3:{
		HAL_GPIO_WritePin(DiceRed0_GPIO_Port, DiceRed0_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed1_GPIO_Port, DiceRed1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed2_GPIO_Port, DiceRed2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed3_GPIO_Port, DiceRed3_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed4_GPIO_Port, DiceRed4_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed5_GPIO_Port, DiceRed5_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed6_GPIO_Port, DiceRed6_Pin, GPIO_PIN_RESET);
	}break;
	case 4:{
		HAL_GPIO_WritePin(DiceRed0_GPIO_Port, DiceRed0_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed1_GPIO_Port, DiceRed1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed2_GPIO_Port, DiceRed2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed3_GPIO_Port, DiceRed3_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed4_GPIO_Port, DiceRed4_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed5_GPIO_Port, DiceRed5_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed6_GPIO_Port, DiceRed6_Pin, GPIO_PIN_SET);
	}break;
	case 5:{
		HAL_GPIO_WritePin(DiceRed0_GPIO_Port, DiceRed0_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed1_GPIO_Port, DiceRed1_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed2_GPIO_Port, DiceRed2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed3_GPIO_Port, DiceRed3_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed4_GPIO_Port, DiceRed4_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed5_GPIO_Port, DiceRed5_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed6_GPIO_Port, DiceRed6_Pin, GPIO_PIN_SET);
	}break;
	case 6:{
		HAL_GPIO_WritePin(DiceRed0_GPIO_Port, DiceRed0_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed1_GPIO_Port, DiceRed1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed2_GPIO_Port, DiceRed2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed3_GPIO_Port, DiceRed3_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(DiceRed4_GPIO_Port, DiceRed4_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed5_GPIO_Port, DiceRed5_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed6_GPIO_Port, DiceRed6_Pin, GPIO_PIN_SET);
	}break;
	default:{
		HAL_GPIO_WritePin(DiceRed0_GPIO_Port, DiceRed0_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed1_GPIO_Port, DiceRed1_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed2_GPIO_Port, DiceRed2_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed3_GPIO_Port, DiceRed3_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed4_GPIO_Port, DiceRed4_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed5_GPIO_Port, DiceRed5_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(DiceRed6_GPIO_Port, DiceRed6_Pin, GPIO_PIN_SET);
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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */


  char str[81] = { '\0' };
  uint16_t str_len = 0;
  str_len = sprintf(str, "Starting up!\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
  if(blueButtonPressed()){
	  while(!buttonPressed()){
		  int blue = blueButtonPressed();
		  str_len = sprintf(str, "Blue state %u\r\n", blue);
		  HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
		  setGreenLed(blue);
	  }
  }
  while( 0 && !buttonPressed()){
	  int blue = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
	  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, blue);

  }
  {
	  int beef = 0xDEADBEEF;
	  char * bp = (char*)&beef;
	  str_len = sprintf(str, "Set value %x, %xhh %xhh %xhh %xhh\r\n",
			  beef, *(bp), *(bp+1), *(bp+2), *(bp+3));
	  HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int dieValue = 1;
  put_on_sseg(10);
  put_on_dots(10);
  HAL_Delay(1000);
  while (1)
  {
	  if(buttonPressed()){
		  put_on_sseg(dieValue);
		  put_on_dots(dieValue);
		  str_len = sprintf(str, "Set value %i, %xhh\r\n", dieValue, *(char*)&(DiceLed0_GPIO_Port->ODR));
		  HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

		  if(dieValue++ == 6){//reset after max value
		     dieValue = 1;
		  }
	  }

	  HAL_Delay(1);
	  /*
	  HAL_GPIO_WritePin(DiceLed0_GPIO_Port, 1<<pinIndex, GPIO_PIN_RESET);
	  if(1 << pinIndex++ == DiceLed6_Pin){//reset back to the start if we have reached the last pin
		  pinIndex = 0;
	  }
	  HAL_GPIO_WritePin(DiceLed0_GPIO_Port, 1<<pinIndex, GPIO_PIN_SET);
	  char * gpioP = (char*)&(DiceLed0_GPIO_Port->ODR);
	  str_len = sprintf(str, "Set pin %i, %xhh %xhh %xhh %xhh\r\n", pinIndex,
			  *gpioP, *(gpioP+1), *(gpioP+2), *(gpioP+3));
	  HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);

	  HAL_Delay(500);
	  //*/
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
  HAL_GPIO_WritePin(GPIOC, DiceLed0_Pin|DiceLed1_Pin|DiceLed2_Pin|DiceLed3_Pin
                          |DiceLed4_Pin|DiceLed5_Pin|DiceLed6_Pin|DiceLed7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DiceRed6_Pin|DiceRed2_Pin|DiceRed3_Pin|DiceRed4_Pin
                          |DiceRed5_Pin|DiceRed1_Pin|DiceRed0_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DiceLed0_Pin DiceLed1_Pin DiceLed2_Pin DiceLed3_Pin
                           DiceLed4_Pin DiceLed5_Pin DiceLed6_Pin DiceLed7_Pin */
  GPIO_InitStruct.Pin = DiceLed0_Pin|DiceLed1_Pin|DiceLed2_Pin|DiceLed3_Pin
                          |DiceLed4_Pin|DiceLed5_Pin|DiceLed6_Pin|DiceLed7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : DiceRed6_Pin DiceRed2_Pin DiceRed3_Pin DiceRed4_Pin
                           DiceRed5_Pin DiceRed1_Pin DiceRed0_Pin */
  GPIO_InitStruct.Pin = DiceRed6_Pin|DiceRed2_Pin|DiceRed3_Pin|DiceRed4_Pin
                          |DiceRed5_Pin|DiceRed1_Pin|DiceRed0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : DiceButton_Pin */
  GPIO_InitStruct.Pin = DiceButton_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DiceButton_GPIO_Port, &GPIO_InitStruct);

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
