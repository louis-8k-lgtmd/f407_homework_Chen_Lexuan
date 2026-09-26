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
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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

/* USER CODE BEGIN PV */
 #define UART_RX_BUFFER_SIZE 64U
static uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE];
static uint8_t uart_pending_command;

#define UART_TX_QUEUE_SIZE 64U
static uint8_t uart_tx_queue[UART_TX_QUEUE_SIZE];
static volatile uint8_t uart_tx_head;
static volatile uint8_t uart_tx_tail;
static uint8_t uart_tx_busy;
static uint8_t uart_tx_dma_length;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void UART_StartNextTransmit(void)
{
  if (uart_tx_busy || uart_tx_tail == uart_tx_head)
  {
    return;
  }

  if (uart_tx_head > uart_tx_tail)
  {
    uart_tx_dma_length = (uint8_t)(uart_tx_head - uart_tx_tail);
  }
  else
  {
    uart_tx_dma_length = (uint8_t)(UART_TX_QUEUE_SIZE - uart_tx_tail);
  }

  uart_tx_busy = 1U;

  if (HAL_UART_Transmit_DMA(&huart1, &uart_tx_queue[uart_tx_tail],
                            uart_tx_dma_length) != HAL_OK)
  {
    uart_tx_busy = 0U;
  }
}

static void UART_QueueResponse(uint8_t command, uint8_t value)
{
  uint8_t free_space = (uint8_t)((uart_tx_tail + UART_TX_QUEUE_SIZE -
                                  uart_tx_head - 1U) % UART_TX_QUEUE_SIZE);
  if (free_space < 4U)
  {
    return;
  }

  uart_tx_queue[uart_tx_head] = command;
  uart_tx_queue[(uart_tx_head + 1U) % UART_TX_QUEUE_SIZE] = value;
  uart_tx_queue[(uart_tx_head + 2U) % UART_TX_QUEUE_SIZE] = '\r';
  uart_tx_queue[(uart_tx_head + 3U) % UART_TX_QUEUE_SIZE] = '\n';
  uart_tx_head = (uint8_t)((uart_tx_head + 4U) % UART_TX_QUEUE_SIZE);
  UART_StartNextTransmit();
}

static void UART_ProcessReceivedByte(uint8_t received)
{
  if (received == 'R' || received == 'G')
  {
    uart_pending_command = received;
  }
  else if ((received == '1' || received == '0') &&
           (uart_pending_command == 'R' || uart_pending_command == 'G'))
  {
    uint16_t led_pin = (uart_pending_command == 'R') ? LED1_Pin : LED2_Pin;
    GPIO_PinState led_state = (received == '1') ? GPIO_PIN_RESET : GPIO_PIN_SET;

    HAL_GPIO_WritePin(GPIOE, led_pin, led_state);
    UART_QueueResponse(uart_pending_command, received);
    uart_pending_command = 0U;
  }
  else if (received != '\r' && received != '\n')
  {
    uart_pending_command = 0U;
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
  if (huart->Instance == USART1)
  {
    for (uint16_t index = 0U; index < size; index++)
    {
      UART_ProcessReceivedByte(uart_rx_buffer[index]);
    }

    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_buffer,
                                     UART_RX_BUFFER_SIZE) == HAL_OK)
    {
      __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
    }
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    uart_tx_tail = (uint8_t)((uart_tx_tail + uart_tx_dma_length) % UART_TX_QUEUE_SIZE);
    uart_tx_busy = 0U;
    UART_StartNextTransmit();
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  //char message[] = "Hello World";
  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_buffer,
                                  UART_RX_BUFFER_SIZE) != HAL_OK)
  {
    Error_Handler();
  }
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    //HAL_UART_Transmit(&huart1,(uint8_t*)message,strlen(message),100);
    //HAL_Delay(1000);
    
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
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
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
