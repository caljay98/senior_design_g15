/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f7xx_hal.h"

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
#define OUT1_Pin GPIO_PIN_0
#define OUT1_GPIO_Port GPIOA
#define OUT2_Pin GPIO_PIN_1
#define OUT2_GPIO_Port GPIOA
#define OUT3_Pin GPIO_PIN_2
#define OUT3_GPIO_Port GPIOA
#define SHDN_Pin GPIO_PIN_5
#define SHDN_GPIO_Port GPIOA
#define RST_Pin GPIO_PIN_0
#define RST_GPIO_Port GPIOB
#define RVS_Pin GPIO_PIN_1
#define RVS_GPIO_Port GPIOB
#define NEO_LED_Pin GPIO_PIN_9
#define NEO_LED_GPIO_Port GPIOE
#define NEG_EN_Pin GPIO_PIN_10
#define NEG_EN_GPIO_Port GPIOB
#define POS_EN_Pin GPIO_PIN_11
#define POS_EN_GPIO_Port GPIOB
#define CNVST_Pin GPIO_PIN_12
#define CNVST_GPIO_Port GPIOB
#define ENC_1_Pin GPIO_PIN_12
#define ENC_1_GPIO_Port GPIOD
#define ENC_1_EXTI_IRQn EXTI15_10_IRQn
#define ENC_2_Pin GPIO_PIN_13
#define ENC_2_GPIO_Port GPIOD
#define ENC_2_EXTI_IRQn EXTI15_10_IRQn
#define USB_OVERCURR_Pin GPIO_PIN_5
#define USB_OVERCURR_GPIO_Port GPIOG
#define USB_PWR_ON_Pin GPIO_PIN_6
#define USB_PWR_ON_GPIO_Port GPIOG
#define USB_VBUS_Pin GPIO_PIN_9
#define USB_VBUS_GPIO_Port GPIOA
#define OTG_FS_ID_Pin GPIO_PIN_10
#define OTG_FS_ID_GPIO_Port GPIOA
#define RELAY_1_Pin GPIO_PIN_0
#define RELAY_1_GPIO_Port GPIOD
#define RELAY_2_Pin GPIO_PIN_1
#define RELAY_2_GPIO_Port GPIOD
#define BACK_BUT_Pin GPIO_PIN_3
#define BACK_BUT_GPIO_Port GPIOD
#define ENABLE_BUT_Pin GPIO_PIN_4
#define ENABLE_BUT_GPIO_Port GPIOD
#define MODE_BUT_Pin GPIO_PIN_5
#define MODE_BUT_GPIO_Port GPIOD
#define SELECT_BUT_Pin GPIO_PIN_6
#define SELECT_BUT_GPIO_Port GPIOD
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
