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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define DiceLed0_Pin GPIO_PIN_0
#define DiceLed0_GPIO_Port GPIOC
#define DiceLed1_Pin GPIO_PIN_1
#define DiceLed1_GPIO_Port GPIOC
#define DiceLed2_Pin GPIO_PIN_2
#define DiceLed2_GPIO_Port GPIOC
#define DiceLed3_Pin GPIO_PIN_3
#define DiceLed3_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define DiceLed4_Pin GPIO_PIN_4
#define DiceLed4_GPIO_Port GPIOC
#define DiceLed5_Pin GPIO_PIN_5
#define DiceLed5_GPIO_Port GPIOC
#define DiceRed6_Pin GPIO_PIN_1
#define DiceRed6_GPIO_Port GPIOB
#define DiceRed2_Pin GPIO_PIN_10
#define DiceRed2_GPIO_Port GPIOB
#define DiceRed3_Pin GPIO_PIN_13
#define DiceRed3_GPIO_Port GPIOB
#define DiceRed4_Pin GPIO_PIN_14
#define DiceRed4_GPIO_Port GPIOB
#define DiceRed5_Pin GPIO_PIN_15
#define DiceRed5_GPIO_Port GPIOB
#define DiceLed6_Pin GPIO_PIN_6
#define DiceLed6_GPIO_Port GPIOC
#define DiceLed7_Pin GPIO_PIN_7
#define DiceLed7_GPIO_Port GPIOC
#define DiceButton_Pin GPIO_PIN_8
#define DiceButton_GPIO_Port GPIOC
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define DiceRed1_Pin GPIO_PIN_4
#define DiceRed1_GPIO_Port GPIOB
#define DiceRed0_Pin GPIO_PIN_5
#define DiceRed0_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
