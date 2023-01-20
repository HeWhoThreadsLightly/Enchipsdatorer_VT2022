/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define vgaUpscale 2
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define vgaPin0_B0_Pin GPIO_PIN_0
#define vgaPin0_B0_GPIO_Port GPIOC
#define vgaPin1_B1_Pin GPIO_PIN_1
#define vgaPin1_B1_GPIO_Port GPIOC
#define vgaPin2_B2_Pin GPIO_PIN_2
#define vgaPin2_B2_GPIO_Port GPIOC
#define vgaPin3_B3_Pin GPIO_PIN_3
#define vgaPin3_B3_GPIO_Port GPIOC
#define vgaPin13_Vsync_Pin GPIO_PIN_1
#define vgaPin13_Vsync_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define vgaPin4_G0_Pin GPIO_PIN_4
#define vgaPin4_G0_GPIO_Port GPIOC
#define vgaPin5_G1_Pin GPIO_PIN_5
#define vgaPin5_G1_GPIO_Port GPIOC
#define vgaPin6_G2_Pin GPIO_PIN_6
#define vgaPin6_G2_GPIO_Port GPIOC
#define vgaPin7_G3_Pin GPIO_PIN_7
#define vgaPin7_G3_GPIO_Port GPIOC
#define vgaPin8_R0_Pin GPIO_PIN_8
#define vgaPin8_R0_GPIO_Port GPIOC
#define vgaPin9_R1_Pin GPIO_PIN_9
#define vgaPin9_R1_GPIO_Port GPIOC
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define vgaPin10_R2_Pin GPIO_PIN_10
#define vgaPin10_R2_GPIO_Port GPIOC
#define vgaPin11_R3_Pin GPIO_PIN_11
#define vgaPin11_R3_GPIO_Port GPIOC
#define vgaPin12_Hsync_Pin GPIO_PIN_12
#define vgaPin12_Hsync_GPIO_Port GPIOC
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_memtomem_dma2_stream0;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
