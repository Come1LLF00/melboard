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
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "buzzer.h"
#include "mb.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef void (buzzer_cb)(struct fifo_queue*, enum request_type);
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define POLLING_RECEIVE_TIMEOUT_PER_CHAR (10)

#define RQT_THRESHOLD (11)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static buzzer_cb* buzzer_callbacks[] = {
		[RQT_DO]             = play_note,
		[RQT_RE]             = play_note,
		[RQT_MI]             = play_note,
		[RQT_FA]             = play_note,
		[RQT_SOL]            = play_note,
		[RQT_LA]             = play_note,
		[RQT_TI]             = play_note,
		[RQT_RAISE_OCTAVE]   = raise_octave,
		[RQT_LOWER_OCTAVE]   = lower_octave,
		[RQT_RAISE_DURATION] = raise_duration,
		[RQT_LOWER_DURATION] = lower_duration
};

static enum request_type rqt_map[] = {
		[KB_BTN_1] = RQT_DO,
		[KB_BTN_2] = RQT_RE,
		[KB_BTN_3] = RQT_MI,
		[KB_BTN_4] = RQT_FA,
		[KB_BTN_5] = RQT_SOL,
		[KB_BTN_6] = RQT_LA,
		[KB_BTN_7] = RQT_TI,
		[KB_BTN_A] = RQT_RAISE_OCTAVE,
		[KB_BTN_a] = RQT_LOWER_OCTAVE,
		[KB_BTN_P] = RQT_RAISE_DURATION,
		[KB_BTN_M] = RQT_LOWER_DURATION
};

static struct fifo_queue requests_queue;

static struct fifo_queue to_user_queue;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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
  MX_TIM1_Init();
  MX_USART6_UART_Init();
  MX_TIM6_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

//  char symbol = 0;
//  HAL_StatusTypeDef rx_status = HAL_OK;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // receive command
	  const uint8_t pressed_btn = poll_btn();
	  if (pressed_btn != KB_BTN_NONE) {
	  // rx_status = HAL_UART_Receive(&huart6, (uint8_t*) &symbol, 1, POLLING_RECEIVE_TIMEOUT_PER_CHAR);
	  // if (rx_status == HAL_OK) {

		  if ((pressed_btn >= KB_BTN_1 && pressed_btn <= KB_BTN_7)
				  || (pressed_btn == KB_BTN_P)
				  || (pressed_btn == KB_BTN_M)
				  || (pressed_btn == KB_BTN_A)
				  || (pressed_btn == KB_BTN_a)
				  || (pressed_btn == KB_BTN_ENTER)) {

			  if (pressed_btn == KB_BTN_ENTER) {
				  for (uint8_t t_n = RQT_DO; t_n <= RQT_TI; ++t_n)
					  queue_write(&requests_queue, &t_n, 1);

			  } else queue_write(&requests_queue, &rqt_map[(size_t) pressed_btn], 1);
		  } else {
			  uint8_t invalid_request = pressed_btn + RQT_THRESHOLD;
			  queue_write(&requests_queue, &invalid_request, 1);
		  }
	  }

	  // transmit result
	  if (!queue_is_empty(&to_user_queue)) {
		  char response[256];
		  const size_t length = queue_read(&to_user_queue, (uint8_t*) response, sizeof(response));
		  HAL_UART_Transmit(&huart6, (uint8_t*) response, length, length * POLLING_RECEIVE_TIMEOUT_PER_CHAR);
	  }

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {

	if(htim->Instance == TIM6) {

		// pass time to buzzer until its done
		if (!is_buzzer_done())
			pass_time(1);
		else mute_buzzer();

		if (queue_is_empty(&requests_queue)) return;

		if (!is_buzzer_done()) {
			const uint8_t top = queue_top(&requests_queue);
			if (top >= RQT_DO && top <= RQT_TI) return;
		}

		// handle request
		uint8_t request = 0;
		queue_read(&requests_queue, &request, 1);
		if (request >= RQT_THRESHOLD) {
			char response[1024];
			snprintf(response, sizeof(response), "неверный символ %u\r\n", request - RQT_THRESHOLD);
			const size_t length = strlen(response);
			queue_write(&to_user_queue, (uint8_t*) response, length);
		} else buzzer_callbacks[request](&to_user_queue, request);
	}

}
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
