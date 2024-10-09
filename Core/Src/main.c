/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "usart.h"
#include "gpio.h"


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "asmart_comm_handler.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
uint8_t response_code = 0;
uint8_t payload_recv[100];
uint8_t command_flag = 0;
uint8_t notif_flag = 0;
aSmart_Comm_Handler_t comm_handler;

const uint8_t command_payload[4] = {0xaa,0xdd,0xcc,0xbb};
  

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void response_handler(uint8_t message_type, uint8_t command_type, uint16_t sequence_number, uint8_t* payload, uint16_t length);

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
  MX_LPUART2_UART_Init();
  /* USER CODE BEGIN 2 */
	
	MX_LPUART2_UART_Init();
	

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	asmart_comm_init(&comm_handler, response_handler);
	
	
  while (1)
  {
		if(command_flag){
			asmart_comm_send_command(&comm_handler, COMMAND_TYPE_BEGIN_TRANSACTION,(uint8_t*)command_payload, 4);
			command_flag = 0;
		}
		if(notif_flag){
			asmart_comm_send_notification(&comm_handler, COMMAND_TYPE_BEGIN_TRANSACTION,(uint8_t*)command_payload, 4);
		}
		asmart_comm_handler(&comm_handler);
		HAL_Delay(50);
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void response_handler(uint8_t message_type, uint8_t command_type, uint16_t sequence_number, uint8_t* payload, uint16_t length) {
    if (payload != NULL && length > 0) {
        // Process the message based on the message type and command type
        switch (message_type) {
            case MSG_TYPE_RESPONSE:
                // Handle responses
                switch (command_type) {
                    case COMMAND_TYPE_BEGIN_TRANSACTION:
												for(uint8_t i=0;i<length ;i++){
													payload_recv[i]= payload[i];
												}
                        // Handle begin transaction response
                        //printf("Received Response for Begin Transaction (Seq: %d): %.*s\n", sequence_number, length, payload);
                        break;
                    case COMMAND_TYPE_END_TRANSACTION:
                        // Handle end transaction response
                        //printf("Received Response for End Transaction (Seq: %d): %.*s\n", sequence_number, length, payload);
                        break;
                    // Add cases for other command types
                    default:
                        //printf("Received Response for Unknown Command (Type: 0x%02X, Seq: %d): %.*s\n", command_type, sequence_number, length, payload);
                        break;
                }
                break;
            case MSG_TYPE_COMMAND:
                // Handle incoming commands
                //printf("Received Command (Type: 0x%02X, Seq: %d): %.*s\n", command_type, sequence_number, length, payload);
                // Process the command and send a response
                // Example: Send a response back
                //HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_15);
                asmart_comm_send_response(&comm_handler, sequence_number, command_type, (uint8_t*)command_payload, 4);
								for(uint8_t i=0;i<length ;i++){
									payload_recv[i]= payload[i];
								}
                break;
            case MSG_TYPE_NOTIFICATION:
								for(uint8_t i=0;i<length ;i++){
										payload_recv[i]= payload[i];
									}
                // Handle notifications
                //printf("Received Notification (Type: 0x%02X): %.*s\n", command_type, length, payload);
                break;
            case MSG_TYPE_ERROR:
                // Handle errors
                if (sequence_number != 0) {
                    // Error in response to a command
                    //printf("Received Error (Code: 0x%02X, Seq: %d): %.*s\n", command_type, sequence_number, length, payload);
                } else {
                    // Error notification
                    //printf("Received Error Notification (Code: 0x%02X): %.*s\n", command_type, length, payload);
                }
                break;
            default:
                //printf("Unknown Message Type (Type: 0x%02X, Seq: %d): %.*s\n", message_type, sequence_number, length, payload);
                break;
        }
    } 
		else {
			HAL_GPIO_TogglePin(GPIOA,GPIO_PIN_15);
        // Handle timeout or error
        //printf("Timeout or Error Occurred (Message Type: 0x%02X, Command Type: 0x%02X, Seq: %d)\n", message_type, command_type, sequence_number);
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
