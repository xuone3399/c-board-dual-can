/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 作业四 · C 板双 CAN 接口数据收发
  *                   许竣皓 202605311924
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
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

static const uint8_t k_fixed_data[8] =
{
  0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00
};

static uint32_t last_tx_ms = 0;

static volatile uint32_t g_tx_count = 0U;
static volatile uint32_t g_echo_count = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static uint8_t Can_Filter_Init(CAN_HandleTypeDef *hcan, uint32_t bank, uint32_t id, uint32_t mask);
static uint8_t Can_Start(CAN_HandleTypeDef *hcan);
static uint8_t Can_Send(CAN_HandleTypeDef *hcan, uint32_t id, const uint8_t *data, uint32_t dlc);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint8_t Can_Filter_Init(CAN_HandleTypeDef *hcan, uint32_t bank, uint32_t id, uint32_t mask)
{
  CAN_FilterTypeDef f;

  f.FilterBank           = bank;
  f.FilterMode           = CAN_FILTERMODE_IDMASK;
  f.FilterScale          = CAN_FILTERSCALE_32BIT;
  f.FilterIdHigh         = ((id & 0x7FFU) << 5);
  f.FilterIdLow          = 0U;
  f.FilterMaskIdHigh     = ((mask & 0x7FFU) << 5);
  f.FilterMaskIdLow      = 0U;
  f.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  f.FilterActivation     = ENABLE;
  f.SlaveStartFilterBank = 14;

  return (HAL_CAN_ConfigFilter(hcan, &f) == HAL_OK) ? 1U : 0U;
}

static uint8_t Can_Start(CAN_HandleTypeDef *hcan)
{
  if (HAL_CAN_Start(hcan) != HAL_OK)
  {
    return 0U;
  }
  if (HAL_CAN_ActivateNotification(hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
  {
    return 0U;
  }
  return 1U;
}

static uint8_t Can_Send(CAN_HandleTypeDef *hcan, uint32_t id, const uint8_t *data, uint32_t dlc)
{
  CAN_TxHeaderTypeDef tx;
  uint32_t mailbox;
  HAL_StatusTypeDef st;

  if ((hcan == NULL) || (data == NULL) || (dlc > 8U))
  {
    return 0U;
  }
  if (HAL_CAN_GetTxMailboxesFreeLevel(hcan) == 0U)
  {
    return 0U;
  }

  tx.StdId              = id;
  tx.ExtId              = 0U;
  tx.IDE                = CAN_ID_STD;
  tx.RTR                = CAN_RTR_DATA;
  tx.DLC                = dlc;
  tx.TransmitGlobalTime = DISABLE;

  st = HAL_CAN_AddTxMessage(hcan, &tx, (uint8_t *)data, &mailbox);

  if (st == HAL_OK)
  {
    return 1U;
  }
  return 0U;
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
  MX_CAN1_Init();
  MX_CAN2_Init();
  /* USER CODE BEGIN 2 */

  if (!Can_Filter_Init(&hcan1, 0U, 0x100U, 0x000U))
  {
    Error_Handler();
  }
  if (!Can_Filter_Init(&hcan2, 14U, 0x200U, 0x7FFU))
  {
    Error_Handler();
  }

  if (!Can_Start(&hcan1))
  {
    Error_Handler();
  }
  if (!Can_Start(&hcan2))
  {
    Error_Handler();
  }

  last_tx_ms = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t now;

    now = HAL_GetTick();
    if ((now - last_tx_ms) >= 1000U)
    {
      last_tx_ms = now;
      if (Can_Send(&hcan1, 0x100U, k_fixed_data, 8U))
      {
        g_tx_count++;
      }
    }

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
  }
  /* USER CODE END 4 */
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
  RCC_OscInitStruct.PLL.PLLM = 6;
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

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef rx;
  uint8_t buf[8];

  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx, buf) != HAL_OK)
  {
    return;
  }

  if (hcan->Instance == CAN2)
  {
    if (rx.StdId == 0x200U)
    {
      if (Can_Send(hcan, 0x201U, buf, rx.DLC))
      {
        g_echo_count++;
      }
    }
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
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: source file line number
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
