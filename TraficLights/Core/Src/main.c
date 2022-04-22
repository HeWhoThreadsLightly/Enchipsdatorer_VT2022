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
#include "abuzz.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
typedef enum{
	ev_none,
	ev_button_push,
	ev_state_timeout,
	ev_reset,
	ev_error
} event;

const char* eventName[] = {
	"ev_none         ",
	"ev_button_push  ",
	"ev_state_timeout",
	"ev_reset        ",
	"ev_error        "
};

typedef enum{
	s_reset,
	s_yellow_car_start,
	s_green_car_go,
	s_button_pressed_car_go,
	s_yellow_car_stop,
	s_red_car_stoped,
	s_red_pedestrian_go,
	s_red_pedestrian_clear,
	s_error
} state;

const char* stateName[] = {
	"s_reset",
	"s_yellow_car_start",
	"s_green_car_go",
	"s_button_pressed_car_go",
	"s_yellow_car_stop",
	"s_red_car_stoped",
	"s_red_pedestrian_go",
	"s_red_pedestrian_clear",
	"s_error"
};


char lightsDecode[] = {
//    IABCDE
	0b010010, // s_reset
	0b001010, // s_yellow_car_start
	0b000110, // s_green_car_go,
	0b100110, // s_button_pressed_car_go,
	0b101010, // s_yellow_car_stop,
	0b110010, // s_red_car_stoped,
	0b110001, // s_red_pedestrian_go,
	0b010010, // s_red_pedestrian_clear
	0b111111, // s_error
};
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
 TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#define EVQ_SIZE 10
event evq[ EVQ_SIZE ];

int evq_count = 0;
int evq_front_ix = 0;
int evq_rear_ix = 0;

void evq_push_back(event e){
	// if queue is full, ignore e
	if ( evq_count < EVQ_SIZE )
	{
		evq[evq_rear_ix] = e;
		evq_rear_ix++;
		evq_rear_ix %= EVQ_SIZE;
		evq_count++;
	}
}

event evq_pop_front(){
	event e = ev_none;
	if ( evq_count > 0 )
	{
		e = evq[evq_front_ix];
		// replace data in cell to detect stupidity:
		evq[evq_front_ix] = ev_error;
		evq_front_ix++;
		evq_front_ix %= EVQ_SIZE;
		evq_count--;
	}
	return e;
}

int blueButtonPressed(){
	return 0 == (B1_GPIO_Port->IDR & B1_Pin);
}

void setLights(state s){
	char lights = lightsDecode[s];
	const char mask = TrafficLightA_Pin | TrafficLightB_Pin | TrafficLightC_Pin | TrafficLightD_Pin | TrafficLightE_Pin | TrafficLightIndicator_Pin;
	char * tmp = (char*)&(TrafficLightA_GPIO_Port->ODR);//using 8 bit assesses
	*tmp = (*tmp & ~mask) | lights;
}

int ticks_left = 0;

void printState(event ev, state s){
	static event l_ev = ev_error;
	static state l_s = s_error;
	if(ev == l_ev && s == l_s)return;//discard repeated inputs
	l_ev = ev;
	l_s = s;

	char str[100] = {'\0'};
	uint16_t str_len = 0;
	str_len = sprintf(str, "EV: %s \t state: %s\r\n", eventName[ev], stateName[s]);
	HAL_UART_Transmit(&huart2, (uint8_t*) str, str_len, HAL_MAX_DELAY);
}

void stateMachine(event ev)
{
	static state s = s_reset;
	printState(ev, s);
	switch(s)
	{
	case s_reset:
	{
		if(ev == ev_reset){
			ticks_left = 2500;
			ev = ev_none;
			s = s_reset;
			setLights(s);
		}
		if(ev == ev_state_timeout){
			ticks_left = 2500;
			ev = ev_none;
			s = s_yellow_car_start;
			setLights(s);
		}
		break;
	}
	case s_yellow_car_start:{
		if(ev == ev_state_timeout){
			ticks_left = 2500;
			ev = ev_none;
			s = s_green_car_go;
			setLights(s);
		}
		break;
	}
	case s_green_car_go:{
		if(ev == ev_button_push){
			ticks_left = 2500;
			ev = ev_none;
			s = s_button_pressed_car_go;
			setLights(s);
			abuzz_start();
			abuzz_p_long();
		}
		break;
	}
	case s_button_pressed_car_go:{
		if(ev == ev_state_timeout){
			ticks_left = 2500;
			ev = ev_none;
			s = s_yellow_car_stop;
			setLights(s);
		}
		break;
	}
	case s_yellow_car_stop:{
		if(ev == ev_state_timeout){
			ticks_left = 2500;
			ev = ev_none;
			s = s_red_car_stoped;
			setLights(s);
		}
		break;
	}
	case s_red_car_stoped:{
		if(ev == ev_state_timeout){
			ticks_left = 5000;
			ev = ev_none;
			s = s_red_pedestrian_go;
			setLights(s);
			abuzz_p_short();
		}
		break;
	}
	case s_red_pedestrian_go:{
		if(ev == ev_state_timeout){
			ticks_left = 2500;
			ev = ev_none;
			s = s_red_pedestrian_clear;
			setLights(s);
			abuzz_stop();
		}
		break;
	}
	case s_red_pedestrian_clear:{
		if(ev == ev_state_timeout){
			ticks_left = 2500;
			ev = ev_none;
			s = s_yellow_car_start;
			setLights(s);
		}
		break;
	}
	default:{
		s = s_error;
		setLights(s);
		break;
		}
	}
}


void my_EXTI15_10_IRQHandler(){
	evq_push_back(ev_button_push);

}

void my_SysTick_Handler()
{
	if(ticks_left != 0)
	{
		ticks_left--;
		if(ticks_left == 0)
		{
			evq_push_back(ev_state_timeout);
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
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  stateMachine(ev_reset);

  while (1)
  {
	  stateMachine(evq_pop_front());
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
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
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
  HAL_GPIO_WritePin(GPIOC, TrafficLightA_Pin|TrafficLightB_Pin|TrafficLightC_Pin|TrafficLightD_Pin
                          |TrafficLightE_Pin|TrafficLightIndicator_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TrafficLightA_Pin TrafficLightB_Pin TrafficLightC_Pin TrafficLightD_Pin
                           TrafficLightE_Pin TrafficLightIndicator_Pin */
  GPIO_InitStruct.Pin = TrafficLightA_Pin|TrafficLightB_Pin|TrafficLightC_Pin|TrafficLightD_Pin
                          |TrafficLightE_Pin|TrafficLightIndicator_Pin;
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

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
