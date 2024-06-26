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
#include "cmsis_os.h"
#include "lwip.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip/udp.h"
#include <string.h>
#include "../../BSP/STM32746G-Discovery/stm32746g_discovery.h"
#include "../../Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_lcd.h"
#include "../../Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_ts.h"
#include "../../Drivers/BSP/STM32746G-Discovery/stm32746g_discovery_camera.h"
#include "ip4_addr.h"
#include "conf_network.h"
#include "socket_client.h"
#include "analiza_obrazu.h"
//#include "apps/httpd.h"
//#include "lwip/apps/httpd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
union _uniaIP
{
	uint32_t u32;
	struct ip4_addr ip;
	uint8_t u8[4];
} uniaIP;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc3;

CRC_HandleTypeDef hcrc;

DCMI_HandleTypeDef hdcmi;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

QSPI_HandleTypeDef hqspi;

RTC_HandleTypeDef hrtc;

SAI_HandleTypeDef hsai_BlockA2;
SAI_HandleTypeDef hsai_BlockB2;

SD_HandleTypeDef hsd1;

SPDIFRX_HandleTypeDef hspdif;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim12;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart6;

SDRAM_HandleTypeDef hsdram1;

osThreadId defaultTaskHandle;
/* USER CODE BEGIN PV */
osThreadId lwipWriteTaskHandle;
osThreadId lwipReadTaskHandle;
uint8_t chBuforTerminala[2048] __attribute__((section(".BuforKamery")));
uint8_t chRodzajWyswietlania = 0;
volatile uint8_t chZdjecieGotowe;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC3_Init(void);
static void MX_CRC_Init(void);
static void MX_DCMI_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C3_Init(void);
static void MX_LTDC_Init(void);
static void MX_QUADSPI_Init(void);
static void MX_RTC_Init(void);
static void MX_SAI2_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_SPDIFRX_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM5_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM12_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART6_UART_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Funkcja wykrywa nacisnięcie przycisku ekranowego
//zwraca wynik w postaci tabeli gdzie 1 oznacza przycisk naciśnięty
void WykryjNacisniecie(TS_StateTypeDef *touch, uint8_t *chTabPrzyc)
{
	uint8_t n;

	for (n=0; n<2*LICZBA_PRZYCISKOW; n++)
		*(chTabPrzyc + n) = 0;	//inicjuj tablicę

	if (!touch->touchDetected)
		return;

	for (n=0; n<LICZBA_PRZYCISKOW; n++)
	{
		if ((touch->touchX[0] >= KLAW_POZ_X1)  && (touch->touchX[0] < KLAW_POZ_X2)  && (touch->touchY[0] >= (KLAW1_Y + n*KLAW_ROZSTAW_Y)) && (touch->touchY[0] < (KLAW1_Y + n*KLAW_ROZSTAW_Y + KLAW_ROZM_Y)))
			*(chTabPrzyc + n * 2 + 0) = 1;
		if ((touch->touchX[0] >= KLAW_POZ_X2)  && (touch->touchY[0] >= (KLAW1_Y + n*KLAW_ROZSTAW_Y)) && (touch->touchY[0] < (KLAW1_Y + n*KLAW_ROZSTAW_Y + KLAW_ROZM_Y)))
			*(chTabPrzyc + n * 2 + 1) = 1;
	}
}


////////////////////////////////////////////////////////////////////////////////
// Aktywne oczekiwanie na zmianę stanu zmiennej z timeoutem
// Parametry:
// [we] *chBit - wskaźnik na zmienną typu volatile, która ma zmienić swój stan w przerwaniu
// [we] sTimeout - czas oczekiwania liczony w obiegach pętli
// Zwraca: systemowy kod błędu
////////////////////////////////////////////////////////////////////////////////
uint8_t CzekajNaBit(volatile uint8_t *chBit, uint16_t sTimeout)
{
	do
	{
		sTimeout--;
	}
	while (sTimeout && (*chBit == 0));

	if (sTimeout)
		return ERR_OK;
	else
		return ERR_TIMEOUT;
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

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC3_Init();
  MX_CRC_Init();
  MX_DCMI_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_QUADSPI_Init();
  MX_RTC_Init();
  MX_SAI2_Init();
  //MX_SDMMC1_SD_Init();
  MX_SPDIFRX_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM5_Init();
  MX_TIM8_Init();
  MX_TIM12_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */

  //httpd_init();
  BSP_LED_Init(LED1);
  BSP_PB_Init(BUTTON_TAMPER, BUTTON_MODE_GPIO);
  BSP_TS_Init(480, 272);	//480x272
  BSP_CAMERA_Init(CAMERA_R320x240);
  BSP_LED_Init(0);




  //Initialize the LCD using the BSP_LCD_Init() function.
  //o Apply the Layer configuration using the BSP_LCD_LayerDefaultInit() function.
  //o Select the LCD layer to be used using the BSP_LCD_SelectLayer() function.
  //o Enable the LCD display using the BSP_LCD_DisplayOn() function.
  BSP_LCD_Init();
  BSP_LCD_SelectLayer(0);
  BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
  BSP_LCD_LayerRgb565Init(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));
  BSP_LCD_SelectLayer(1);
  BSP_LCD_Clear(LCD_COLOR_BLACK);
  BSP_LCD_SetTransparency(1, 0xFF);

  BSP_LCD_SelectLayer(0);
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_SetTransparency(0, 0xFF);

  BSP_LCD_DisplayOn();

  uint8_t chNapis[45];
  uint8_t chNapisyPrzyciskow[2*LICZBA_PRZYCISKOW][6] = {"Fotka", "Film>", "Cz-Bi",  "HisCB", "KrawR", "KrawS", "HisKr", "Odszu", "Dylat", "MonIP", "Julia", "Mandl"};

  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  sprintf((char*)chNapis, "OV9655 na STM32F746G-DISCO @%lu MHz", (uint32_t)HAL_RCC_GetSysClockFreq()/1000000);
  BSP_LCD_DisplayStringAt(0, 5, chNapis, CENTER_MODE);

  //przyciski tło
  BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
  for (uint8_t n=0; n<LICZBA_PRZYCISKOW; n++)
  {
	  BSP_LCD_FillRect(KLAW_POZ_X1, KLAW1_Y + n * KLAW_ROZSTAW_Y, KLAW_ROZM_X, KLAW_ROZM_Y);
	  BSP_LCD_FillRect(KLAW_POZ_X2, KLAW1_Y + n * KLAW_ROZSTAW_Y, KLAW_ROZM_X, KLAW_ROZM_Y);
  }

  //przyciski ramka
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  for (uint8_t n=0; n<LICZBA_PRZYCISKOW; n++)
  {
	  BSP_LCD_DrawRect(KLAW_POZ_X1, KLAW1_Y + n * KLAW_ROZSTAW_Y, KLAW_ROZM_X, KLAW_ROZM_Y);
	  BSP_LCD_DrawRect(KLAW_POZ_X2, KLAW1_Y + n * KLAW_ROZSTAW_Y, KLAW_ROZM_X, KLAW_ROZM_Y);
  }

  BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
  BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
  for (uint8_t n=0; n<LICZBA_PRZYCISKOW; n++)
  {
	  BSP_LCD_DisplayStringAt(KLAW_POZ_X1+5, KLAW1_Y + NAPIS_OFFY + n * KLAW_ROZSTAW_Y, &chNapisyPrzyciskow[2*n+0][0], LEFT_MODE);
	  BSP_LCD_DisplayStringAt(KLAW_POZ_X2+5, KLAW1_Y + NAPIS_OFFY + n * KLAW_ROZSTAW_Y, &chNapisyPrzyciskow[2*n+1][0], LEFT_MODE);
  }
  BSP_LCD_SetBackColor(LCD_COLOR_WHITE);

  //obraz
  BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
  BSP_LCD_DrawRect(4, 25, 321, 241);		//ramka obrazu
  BSP_LCD_SelectLayer(1);
  BSP_LCD_SetLayerWindow(1, 5, 26, 320, 240);
  BSP_LCD_SelectLayer(0);

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */

  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  MX_LWIP_Init();	//to musi być ostatnia instrukcja, bo przeskakuje do osobnego wątku i nie idzie dalej.
  while (1)
  {
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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SAI2
                              |RCC_PERIPHCLK_SDMMC1|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  PeriphClkInitStruct.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLSAI;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
  PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DCMI Initialization Function
  * @param None
  * @retval None
  */
static void MX_DCMI_Init(void)
{

  /* USER CODE BEGIN DCMI_Init 0 */

  /* USER CODE END DCMI_Init 0 */

  /* USER CODE BEGIN DCMI_Init 1 */

  /* USER CODE END DCMI_Init 1 */
  hdcmi.Instance = DCMI;
  hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
  hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_FALLING;
  hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
  hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
  hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
  hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
  hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;
  hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
  hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
  hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
  hdcmi.Init.LineSelectStart = DCMI_OELS_ODD;
  if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DCMI_Init 2 */

  /* USER CODE END DCMI_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00C0EAFF;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00C0EAFF;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 40;
  hltdc.Init.VerticalSync = 9;
  hltdc.Init.AccumulatedHBP = 53;
  hltdc.Init.AccumulatedVBP = 11;
  hltdc.Init.AccumulatedActiveW = 533;
  hltdc.Init.AccumulatedActiveH = 283;
  hltdc.Init.TotalWidth = 565;
  hltdc.Init.TotalHeigh = 285;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 480;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 272;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = 0xC0000000;
  pLayerCfg.ImageWidth = 480;
  pLayerCfg.ImageHeight = 272;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 1;
  hqspi.Init.FifoThreshold = 4;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  hqspi.Init.FlashSize = 24;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_6_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  RTC_AlarmTypeDef sAlarm = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the Alarm A
  */
  sAlarm.AlarmTime.Hours = 0x0;
  sAlarm.AlarmTime.Minutes = 0x0;
  sAlarm.AlarmTime.Seconds = 0x0;
  sAlarm.AlarmTime.SubSeconds = 0x0;
  sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
  sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
  sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
  sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
  sAlarm.AlarmDateWeekDay = 0x1;
  sAlarm.Alarm = RTC_ALARM_A;
  if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the Alarm B
  */
  sAlarm.Alarm = RTC_ALARM_B;
  if (HAL_RTC_SetAlarm(&hrtc, &sAlarm, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the TimeStamp
  */
  if (HAL_RTCEx_SetTimeStamp(&hrtc, RTC_TIMESTAMPEDGE_RISING, RTC_TIMESTAMPPIN_POS1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SAI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI2_Init(void)
{

  /* USER CODE BEGIN SAI2_Init 0 */

  /* USER CODE END SAI2_Init 0 */

  /* USER CODE BEGIN SAI2_Init 1 */

  /* USER CODE END SAI2_Init 1 */
  hsai_BlockA2.Instance = SAI2_Block_A;
  hsai_BlockA2.Init.Protocol = SAI_FREE_PROTOCOL;
  hsai_BlockA2.Init.AudioMode = SAI_MODEMASTER_TX;
  hsai_BlockA2.Init.DataSize = SAI_DATASIZE_8;
  hsai_BlockA2.Init.FirstBit = SAI_FIRSTBIT_MSB;
  hsai_BlockA2.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  hsai_BlockA2.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA2.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA2.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockA2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockA2.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_192K;
  hsai_BlockA2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockA2.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockA2.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockA2.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  hsai_BlockA2.FrameInit.FrameLength = 8;
  hsai_BlockA2.FrameInit.ActiveFrameLength = 1;
  hsai_BlockA2.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
  hsai_BlockA2.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
  hsai_BlockA2.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  hsai_BlockA2.SlotInit.FirstBitOffset = 0;
  hsai_BlockA2.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  hsai_BlockA2.SlotInit.SlotNumber = 1;
  hsai_BlockA2.SlotInit.SlotActive = 0x00000000;
  if (HAL_SAI_Init(&hsai_BlockA2) != HAL_OK)
  {
    Error_Handler();
  }
  hsai_BlockB2.Instance = SAI2_Block_B;
  hsai_BlockB2.Init.Protocol = SAI_FREE_PROTOCOL;
  hsai_BlockB2.Init.AudioMode = SAI_MODESLAVE_RX;
  hsai_BlockB2.Init.DataSize = SAI_DATASIZE_8;
  hsai_BlockB2.Init.FirstBit = SAI_FIRSTBIT_MSB;
  hsai_BlockB2.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  hsai_BlockB2.Init.Synchro = SAI_SYNCHRONOUS;
  hsai_BlockB2.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockB2.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
  hsai_BlockB2.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockB2.Init.MonoStereoMode = SAI_STEREOMODE;
  hsai_BlockB2.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockB2.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  hsai_BlockB2.FrameInit.FrameLength = 8;
  hsai_BlockB2.FrameInit.ActiveFrameLength = 1;
  hsai_BlockB2.FrameInit.FSDefinition = SAI_FS_STARTFRAME;
  hsai_BlockB2.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
  hsai_BlockB2.FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  hsai_BlockB2.SlotInit.FirstBitOffset = 0;
  hsai_BlockB2.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  hsai_BlockB2.SlotInit.SlotNumber = 1;
  hsai_BlockB2.SlotInit.SlotActive = 0x00000000;
  if (HAL_SAI_Init(&hsai_BlockB2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI2_Init 2 */

  /* USER CODE END SAI2_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 0;
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDMMC1_Init 2 */

  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief SPDIFRX Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPDIFRX_Init(void)
{

  /* USER CODE BEGIN SPDIFRX_Init 0 */

  /* USER CODE END SPDIFRX_Init 0 */

  /* USER CODE BEGIN SPDIFRX_Init 1 */

  /* USER CODE END SPDIFRX_Init 1 */
  hspdif.Instance = SPDIFRX;
  hspdif.Init.InputSelection = SPDIFRX_INPUT_IN0;
  hspdif.Init.Retries = SPDIFRX_MAXRETRIES_NONE;
  hspdif.Init.WaitForActivity = SPDIFRX_WAITFORACTIVITY_OFF;
  hspdif.Init.ChannelSelection = SPDIFRX_CHANNEL_A;
  hspdif.Init.DataFormat = SPDIFRX_DATAFORMAT_LSB;
  hspdif.Init.StereoMode = SPDIFRX_STEREOMODE_DISABLE;
  hspdif.Init.PreambleTypeMask = SPDIFRX_PREAMBLETYPEMASK_OFF;
  hspdif.Init.ChannelStatusMask = SPDIFRX_CHANNELSTATUS_OFF;
  hspdif.Init.ValidityBitMask = SPDIFRX_VALIDITYMASK_OFF;
  hspdif.Init.ParityErrorMask = SPDIFRX_PARITYERRORMASK_OFF;
  if (HAL_SPDIFRX_Init(&hspdif) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPDIFRX_Init 2 */

  /* USER CODE END SPDIFRX_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
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
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
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
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
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
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 0;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 4294967295;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim5, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */
  HAL_TIM_MspPostInit(&htim5);

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 65535;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief TIM12 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM12_Init(void)
{

  /* USER CODE BEGIN TIM12_Init 0 */

  /* USER CODE END TIM12_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM12_Init 1 */

  /* USER CODE END TIM12_Init 1 */
  htim12.Instance = TIM12;
  htim12.Init.Prescaler = 0;
  htim12.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim12.Init.Period = 65535;
  htim12.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim12.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim12) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim12, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM12_Init 2 */

  /* USER CODE END TIM12_Init 2 */
  HAL_TIM_MspPostInit(&htim12);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  huart6.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart6.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK1;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 3;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOJ_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();
  __HAL_RCC_GPIOK_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOI, ARDUINO_D7_Pin|ARDUINO_D8_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DISP_GPIO_Port, LCD_DISP_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DCMI_PWR_EN_GPIO_Port, DCMI_PWR_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, ARDUINO_D4_Pin|ARDUINO_D2_Pin|EXT_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : OTG_HS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_HS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_HS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_D7_Pin ULPI_D6_Pin ULPI_D5_Pin ULPI_D3_Pin
                           ULPI_D2_Pin ULPI_D1_Pin ULPI_D4_Pin */
  GPIO_InitStruct.Pin = ULPI_D7_Pin|ULPI_D6_Pin|ULPI_D5_Pin|ULPI_D3_Pin
                          |ULPI_D2_Pin|ULPI_D1_Pin|ULPI_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_VBUS_Pin */
  GPIO_InitStruct.Pin = OTG_FS_VBUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_VBUS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Audio_INT_Pin */
  GPIO_InitStruct.Pin = Audio_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Audio_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARDUINO_D7_Pin ARDUINO_D8_Pin LCD_DISP_Pin */
  GPIO_InitStruct.Pin = ARDUINO_D7_Pin|ARDUINO_D8_Pin|LCD_DISP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /*Configure GPIO pin : uSD_Detect_Pin */
  GPIO_InitStruct.Pin = uSD_Detect_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(uSD_Detect_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_BL_CTRL_Pin */
  GPIO_InitStruct.Pin = LCD_BL_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TP3_Pin NC2_Pin */
  GPIO_InitStruct.Pin = TP3_Pin|NC2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  /*Configure GPIO pin : DCMI_PWR_EN_Pin */
  GPIO_InitStruct.Pin = DCMI_PWR_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DCMI_PWR_EN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_INT_Pin */
  GPIO_InitStruct.Pin = LCD_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ULPI_NXT_Pin */
  GPIO_InitStruct.Pin = ULPI_NXT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(ULPI_NXT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARDUINO_D4_Pin ARDUINO_D2_Pin EXT_RST_Pin */
  GPIO_InitStruct.Pin = ARDUINO_D4_Pin|ARDUINO_D2_Pin|EXT_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_STP_Pin ULPI_DIR_Pin */
  GPIO_InitStruct.Pin = ULPI_STP_Pin|ULPI_DIR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : RMII_RXER_Pin */
  GPIO_InitStruct.Pin = RMII_RXER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RMII_RXER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ULPI_CLK_Pin ULPI_D0_Pin */
  GPIO_InitStruct.Pin = ULPI_CLK_Pin|ULPI_D0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG_HS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void vApplicationIdleHook( void )
{

}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_HOST */
  MX_USB_HOST_Init();

  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN 5 */
  //const char* message = "Hello UDP message!\n\r";
  static uint8_t bufor_kamery[1280 * 1024 * 2] __attribute__((section(".BuforKamery")));
  static uint8_t bufor_okna[320 * 240 * 2] __attribute__((section(".BuforLCD1")));
  //static uint8_t bufor_ekranu[480 * 272 * 4] __attribute__((section(".BuforLCD0")));
  static uint8_t bufor_czarnobialy[320 * 240] __attribute__((section(".BuforKamery")));
  static uint8_t bufor_cbOut[320 * 240] __attribute__((section(".BuforKamery")));
  char chNapis[DLUGOSC_NAPISU];
  uint8_t chZrobione = 0;
  uint8_t n, chFilm = 0;
  TS_StateTypeDef touch;
  extern float fImag;
  extern float fX, fY, fZoom;
  extern uint8_t chMnozPalety;
  uint8_t chWskWiersza = 0;
  uint8_t chTabPrzyciskow[2*LICZBA_PRZYCISKOW];
  uint8_t chErr;
  //uint32_t nCzas;
  uint32_t nCzasOper, nCzasCalk;		//czas całkowity i czas operacji

  struct etharp_entry {
  #if ARP_QUEUEING
    /** Pointer to queue of pending outgoing packets on this ARP entry. */
    struct etharp_q_entry *q;
  #else /* ARP_QUEUEING */
    /** Pointer to a single pending outgoing packet on this ARP entry. */
    struct pbuf *q;
  #endif /* ARP_QUEUEING */
    ip4_addr_t ipaddr;
    struct netif *netif;
    struct eth_addr ethaddr;
    u16_t ctime;
    u8_t state;
  };
  //extern struct etharp_entry arp_table[];
  extern struct netif gnetif;


#ifdef USE_SOCKET
  //Create the task to write to server
  osThreadDef(lwipWriteTask, WriteLWIPClientTask, osPriorityNormal, 0, 2048);
  lwipWriteTaskHandle = osThreadCreate(osThread(lwipWriteTask), NULL);

  //Create the task to read from server
  osThreadDef(lwipReadTask, ReadLWIPClientTask, osPriorityNormal, 0, 2048);
  lwipReadTaskHandle = osThreadCreate(osThread(lwipReadTask), NULL);
#endif


  osDelay(1000);

  /* Infinite loop */
  for (;;) {
    osDelay(100);

    BSP_LED_Toggle(0);
    BSP_TS_GetState(&touch);

    BSP_LCD_SelectLayer(0);
    sprintf(chNapis, "X=%d, Y=%d  ", touch.touchX[0], touch.touchY[0]);
    BSP_LCD_DisplayStringAt(KLAW_POZ_X1, 230, (uint8_t*)chNapis, LEFT_MODE);
    sprintf(chNapis, "T=%d, W=%d  ", touch.touchDetected, touch.touchWeight[0]);
    BSP_LCD_DisplayStringAt(KLAW_POZ_X1, 250, (uint8_t*)chNapis, LEFT_MODE);

    if (!touch.touchDetected)
    {
    	chZrobione = 0;
    }
	WykryjNacisniecie(&touch, chTabPrzyciskow);	//wykryj naciśnięcie w obrębie zdefiniowanych przycisków

    //przycisk zrób zdjęcie
    if (chTabPrzyciskow[0] && !chZrobione)
    {
    	BSP_CAMERA_SnapshotStart(bufor_okna);
    	chZrobione = 1;
    	chRodzajWyswietlania = 0;
    }

    //przycisk rozpocznij/zakończ film
    if (chTabPrzyciskow[1] && !chZrobione)
	{
    	if (chFilm)
    	{
    		BSP_CAMERA_Suspend();
    		sprintf(chNapis, "Film>");
			chFilm = 0;
    	}
    	else
    	{
			BSP_CAMERA_ContinuousStart(bufor_okna);
			sprintf(chNapis, "Film|");
			chFilm = 1;
    	}

    	//zmień status przycisku Start/Stop Film
		BSP_LCD_SelectLayer(0);
		BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
		BSP_LCD_SetBackColor(LCD_COLOR_GREEN);
		BSP_LCD_DisplayStringAt(KLAW_POZ_X2+5, KLAW1_Y + NAPIS_OFFY, (uint8_t*)chNapis, LEFT_MODE);
		BSP_LCD_SetBackColor(LCD_COLOR_WHITE);	//przywróć kolor tła
		chRodzajWyswietlania = 0;
    	chZrobione = 1;
	}

    //konwersja kolor -> C/B
    if (chTabPrzyciskow[2] && !chZrobione)
    {
    	BSP_CAMERA_SnapshotStart(bufor_kamery);
		chRodzajWyswietlania = TW_OBRAZ_CB;
		chZrobione = 1;
    }

	//histogram czarno-biały
	if (chTabPrzyciskow[3] && !chZrobione)
	{
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		chRodzajWyswietlania = TW_OBRAZ_CB_HIST;
		chZrobione = 1;
	}

    //wykrywanie krawędzi Robertsa
    if (chTabPrzyciskow[4] && !chZrobione)
    {
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		chRodzajWyswietlania = TW_OBRAZ_KRAW_ROB;
		chZrobione = 1;
    }

    //wykrywanie krawędzi Sobela
    if (chTabPrzyciskow[5] && !chZrobione)
    {
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		chRodzajWyswietlania = TW_OBRAZ_KRAW_SOB;
		chZrobione = 1;
    }

 	//histogram po detekcji krawędzi
 	if (chTabPrzyciskow[6] && !chZrobione)
 	{
 		BSP_CAMERA_SnapshotStart(bufor_kamery);
 		chRodzajWyswietlania = TW_OBRAZ_KRAW_HIST;
 		chZrobione = 1;
 	}

	/*/histogram kolorowy
	if (chTabPrzyciskow[7] && !chZrobione)
	{
		uint8_t histR[32], histG[64], histB[32];
		nCzas = HAL_GetTick();
		HistogramRGB565(bufor_kamery, histR, histG, histB, 320*240);
		nCzas = MinalCzas(nCzas);

		//rysuj histogram na  ekranie
		BSP_LCD_SelectLayer(1);
		BSP_LCD_SetTextColor(LCD_COLOR_RED);
		for (uint8_t x=0; x<32; x++)
			BSP_LCD_FillRect(x*2, 240- *(histR+x), 1, *(histR+x));

		BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
		for (uint8_t x=0; x<64; x++)
			BSP_LCD_FillRect(x*2+64, 240- *(histG+x), 1, *(histG+x));

		BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
		for (uint8_t x=0; x<32; x++)
			BSP_LCD_FillRect(x*2+192, 240- *(histB+x), 1, *(histB+x));

		sprintf(chNapis, "t=%ldms ", nCzas);
		BSP_LCD_DisplayStringAt(260, 225, (uint8_t*)chNapis, LEFT_MODE);
		chZrobione = 1;
		chRodzajWyswietlania = 0;
	} */

 	//odszumianie
	if (chTabPrzyciskow[7] && !chZrobione)
	{
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		chRodzajWyswietlania = TW_OBRAZ_ODSZUMIAN;
		chZrobione = 1;
	}

	//dylatacja wykrytych krawędzi
 	if (chTabPrzyciskow[8] && !chZrobione)
 	{
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		chRodzajWyswietlania = TW_OBRAZ_DYLATACJA;
		chZrobione = 1;
 	}

	//przycisk Monitor IP
	if (chTabPrzyciskow[9] && !chZrobione)
	{
		BSP_LCD_SelectLayer(1);
		BSP_LCD_Clear(LCD_COLOR_BLACK);
		chRodzajWyswietlania = TW_MONITOR;
		chZrobione = 1;
	}


	//przycisk fraktal Julii
	if (chTabPrzyciskow[10] && !chZrobione)
	{
		InitFraktal(0);
		chRodzajWyswietlania = TW_JULIA;
		chZrobione = 1;
	}

	//przycisk fraktal Mandelbrota
	if (chTabPrzyciskow[11] && !chZrobione)
	{
		InitFraktal(1);	//Mandelbrot, rotacja kolorów palety
		chRodzajWyswietlania = TW_MANDELBROT;
		chZrobione = 1;
	}


	//cykliczne wyświetlanie
	switch(chRodzajWyswietlania)
	{

	case TW_MONITOR:
		BSP_LCD_SelectLayer(1);
		sprintf(chNapis, "WHADDR: %d.%d.%d.%d.%d.%d ", gnetif.hwaddr[0], gnetif.hwaddr[1], gnetif.hwaddr[2], gnetif.hwaddr[3], gnetif.hwaddr[4], gnetif.hwaddr[5]);
		BSP_LCD_DisplayStringAt(5, 10, (uint8_t*)chNapis, LEFT_MODE);

		uniaIP.ip = gnetif.ip_addr;
		sprintf(chNapis, "IP Addr: %d.%d.%d.%d ", uniaIP.u8[0], uniaIP.u8[1],  uniaIP.u8[2],  uniaIP.u8[3]);
		BSP_LCD_DisplayStringAt(5, 25, (uint8_t*)chNapis, LEFT_MODE);



		//flagi są z pliku netif.h
		sprintf(chNapis, "FLAG_UP: %d ", gnetif.flags & NETIF_FLAG_UP);
		BSP_LCD_DisplayStringAt(5, 55, (uint8_t*)chNapis, LEFT_MODE);

		sprintf(chNapis, "FLAG_BROADCAST: %d ", (gnetif.flags & NETIF_FLAG_BROADCAST) == NETIF_FLAG_BROADCAST);
		BSP_LCD_DisplayStringAt(5, 70, (uint8_t*)chNapis, LEFT_MODE);

		sprintf(chNapis, "FLAG_LINK_UP: %d ", (gnetif.flags & NETIF_FLAG_LINK_UP) == NETIF_FLAG_LINK_UP);
		BSP_LCD_DisplayStringAt(5, 85, (uint8_t*)chNapis, LEFT_MODE);

		sprintf(chNapis, "FLAG_ETHARP: %d", (gnetif.flags & NETIF_FLAG_ETHARP) == NETIF_FLAG_ETHARP);
		BSP_LCD_DisplayStringAt(5, 100, (uint8_t*)chNapis, LEFT_MODE);

		sprintf(chNapis, "FLAG_ETHERNET: %d ", (gnetif.flags & NETIF_FLAG_ETHERNET) == NETIF_FLAG_ETHERNET);
		BSP_LCD_DisplayStringAt(5, 115, (uint8_t*)chNapis, LEFT_MODE);

		sprintf(chNapis, "FLAG_IGMP: %d ", (gnetif.flags & NETIF_FLAG_IGMP) == NETIF_FLAG_IGMP);
		BSP_LCD_DisplayStringAt(5, 130, (uint8_t*)chNapis, LEFT_MODE);

		sprintf(chNapis, "FLAG_MLD6: %d ", (gnetif.flags & NETIF_FLAG_MLD6) == NETIF_FLAG_MLD6);
		BSP_LCD_DisplayStringAt(5, 145, (uint8_t*)chNapis, LEFT_MODE);

		//tablica ARP
		for (n=0; n<4; n++)
		{
			//PobierzEthArp(n, &uniaIP.u32);
			//uniaIP.ip = arp_table[n].ipaddr;
			//sprintf(chNapis, "ARP[%d].IP: %d.%d.%d.%d, EthAdr:  %d.%d.%d.%d.%d.%d", n, uniaIP.u8[0], uniaIP.u8[1],  uniaIP.u8[2],  uniaIP.u8[3], arp_table[n].ethaddr.addr[0], arp_table[n].ethaddr.addr[1], arp_table[n].ethaddr.addr[2], arp_table[n].ethaddr.addr[3], arp_table[n].ethaddr.addr[4], arp_table[n].ethaddr.addr[5]);
			//sprintf(chNapis, "ARP[%d].IP: %d.%d.%d.%d, EthAdr:  %d.%d.%d.%d.%d.%d", n, uniaIP.u8[0], uniaIP.u8[1],  uniaIP.u8[2],  uniaIP.u8[3], adres_eth[0], adres_eth[1], adres_eth[2], adres_eth[3], adres_eth[4], adres_eth[5]);
			sprintf(chNapis, "ARP[%d].IP: %d.%d.%d.%d ", n, uniaIP.u8[0], uniaIP.u8[1],  uniaIP.u8[2],  uniaIP.u8[3]);
			BSP_LCD_DisplayStringAt(5, 160+n*15, (uint8_t*)chNapis, LEFT_MODE);
		}
		break;

	case TW_TERMINAL:
		BSP_LCD_SelectLayer(1);
		memcpy (chNapis, chBuforTerminala, 45);		//ogranicz napis do szerokości ekranu
		chNapis[45] = 0;
		BSP_LCD_DisplayStringAt(5, chWskWiersza*15, (uint8_t*)chNapis, LEFT_MODE);
		chWskWiersza++;
		chWskWiersza &= 0x0F;				//można wyświetlić 16 wierszy
		memset(chBuforTerminala, 0, 2048); 	//wyczyść bufor
		chRodzajWyswietlania = 0;			//wyjdź z trybu terminala do czasu przyjścia kolejnego datagramu
		break;

	case TW_OBRAZ_CB:
		chErr = CzekajNaBit(&chZdjecieGotowe, 1000);	//czekaj z timeoutem na zakończenie transferu DMA
		if (!chErr)
		{
			nCzasOper = HAL_GetTick();
			KonwersjaRGB565doCB7((uint16_t*)bufor_kamery, bufor_czarnobialy, 320*240);
			nCzasOper = MinalCzas(nCzasOper);
			KonwersjaCB7doRGB565(bufor_czarnobialy, (uint16_t*)bufor_okna, 320*240);
			BSP_LCD_SelectLayer(1);
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			sprintf(chNapis, "t=%ldms ", nCzasOper);
			BSP_LCD_DisplayStringAt(200, 225, (uint8_t*)chNapis, LEFT_MODE);
		}
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		break;

	case TW_OBRAZ_KRAW_ROB:
		chErr = CzekajNaBit(&chZdjecieGotowe, 1000);	//czekaj z timeoutem na zakończenie transferu DMA
		if (!chErr)
		{
			nCzasCalk = HAL_GetTick();
			KonwersjaRGB565doCB7((uint16_t*)bufor_kamery, bufor_czarnobialy, 320*240);
			nCzasOper = HAL_GetTick();
			DetekcjaKrawedziRoberts(bufor_czarnobialy, bufor_cbOut, 320, 240, 16);
			nCzasOper = MinalCzas(nCzasOper);
			KonwersjaCB7doRGB565(bufor_cbOut, (uint16_t*)bufor_okna, 320*240);
			nCzasCalk = MinalCzas(nCzasCalk);
			BSP_LCD_SelectLayer(1);
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			sprintf(chNapis, "to=%ld, tc=%ld ms ", nCzasOper, nCzasCalk);
			BSP_LCD_DisplayStringAt(200, 225, (uint8_t*)chNapis, LEFT_MODE);
		}
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		break;

	case TW_OBRAZ_KRAW_SOB:
			chErr = CzekajNaBit(&chZdjecieGotowe, 1000);	//czekaj z timeoutem na zakończenie transferu DMA
			if (!chErr)
			{
				nCzasCalk = HAL_GetTick();
				KonwersjaRGB565doCB7((uint16_t*)bufor_kamery, bufor_czarnobialy, 320*240);
				nCzasOper = HAL_GetTick();
				DetekcjaKrawedziSobel(bufor_czarnobialy, bufor_cbOut, 320, 240, 16);
				nCzasOper = MinalCzas(nCzasOper);
				KonwersjaCB7doRGB565(bufor_cbOut, (uint16_t*)bufor_okna, 320*240);
				nCzasCalk = MinalCzas(nCzasCalk);
				BSP_LCD_SelectLayer(1);
				BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
				sprintf(chNapis, "to=%ld, tc=%ld ms ", nCzasOper, nCzasCalk);
				BSP_LCD_DisplayStringAt(200, 225, (uint8_t*)chNapis, LEFT_MODE);
			}
			BSP_CAMERA_SnapshotStart(bufor_kamery);
			break;

	case TW_OBRAZ_CB_HIST:		//histogram dla obrazu czarno-bialego
		chErr = CzekajNaBit(&chZdjecieGotowe, 1000);	//czekaj z timeoutem na zakończenie transferu DMA
		if (!chErr)
		{
			uint8_t hist[129];
			nCzasCalk = HAL_GetTick();
			KonwersjaRGB565doCB7((uint16_t*)bufor_kamery, bufor_czarnobialy, 320*240);
			KonwersjaCB7doRGB565(bufor_czarnobialy, (uint16_t*)bufor_okna, 320*240);
			nCzasOper = HAL_GetTick();
			HistogramCB7(bufor_czarnobialy, hist, 320*240);
			nCzasOper = MinalCzas(nCzasOper);

			//rysuj histogram na  ekranie
			BSP_LCD_SelectLayer(1);
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			for (uint8_t x=0; x<129; x++)
			{
				BSP_LCD_FillRect(x*2, 240 - *(hist+x), 1, *(hist+x));
			}
			nCzasCalk = MinalCzas(nCzasCalk);
			BSP_LCD_SelectLayer(1);
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			sprintf(chNapis, "to=%ld, tc=%ld ms ", nCzasOper, nCzasCalk);
			BSP_LCD_DisplayStringAt(20, 225, (uint8_t*)chNapis, LEFT_MODE);
		}
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		break;

	case TW_OBRAZ_KRAW_HIST:	//histogram dla obrazu po detekcji krawędzi
		chErr = CzekajNaBit(&chZdjecieGotowe, 1000);	//czekaj z timeoutem na zakończenie transferu DMA
		if (!chErr)
		{
			uint8_t hist[129];
			nCzasCalk = HAL_GetTick();
			KonwersjaRGB565doCB7((uint16_t*)bufor_kamery, bufor_czarnobialy, 320*240);
			DetekcjaKrawedziRoberts(bufor_czarnobialy, bufor_cbOut, 320, 240, 0);
			KonwersjaCB7doRGB565(bufor_cbOut, (uint16_t*)bufor_okna, 320*240);
			nCzasOper = HAL_GetTick();
			HistogramCB7(bufor_cbOut, hist, 320*240);
			nCzasOper = MinalCzas(nCzasOper);
			nCzasCalk = MinalCzas(nCzasCalk);
			//rysuj histogram na  ekranie
			BSP_LCD_SelectLayer(1);
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			for (uint8_t x=0; x<129; x++)
				BSP_LCD_FillRect(x*2, 240- *(hist+x), 1, *(hist+x));

			BSP_LCD_SelectLayer(1);
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			sprintf(chNapis, "to=%ld, tc=%ld ms ", nCzasOper, nCzasCalk);
			BSP_LCD_DisplayStringAt(20, 225, (uint8_t*)chNapis, LEFT_MODE);
		}
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		break;

	case TW_OBRAZ_ODSZUMIAN:
		chErr = CzekajNaBit(&chZdjecieGotowe, 1000);	//czekaj z timeoutem na zakończenie transferu DMA
		if (!chErr)
		{
			nCzasCalk = HAL_GetTick();
			KonwersjaRGB565doCB7((uint16_t*)bufor_kamery, bufor_czarnobialy, 320*240);
			DetekcjaKrawedziSobel(bufor_czarnobialy, bufor_cbOut, 320, 240, 16);
			nCzasOper = HAL_GetTick();
			Odszumianie(bufor_cbOut, bufor_czarnobialy, 320, 240, 16);
			nCzasOper = MinalCzas(nCzasOper);
			KonwersjaCB7doRGB565(bufor_czarnobialy, (uint16_t*)bufor_okna, 320*240);
			nCzasCalk = MinalCzas(nCzasCalk);
			BSP_LCD_SelectLayer(1);
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			sprintf(chNapis, "to=%ld, tc=%ld ms ", nCzasOper, nCzasCalk);
			BSP_LCD_DisplayStringAt(200, 225, (uint8_t*)chNapis, LEFT_MODE);
		}
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		break;

	case TW_OBRAZ_DYLATACJA:
		chErr = CzekajNaBit(&chZdjecieGotowe, 1000);	//czekaj z timeoutem na zakończenie transferu DMA
		if (!chErr)
		{
			nCzasCalk = HAL_GetTick();
			KonwersjaRGB565doCB7((uint16_t*)bufor_kamery, bufor_czarnobialy, 320*240);
			DetekcjaKrawedziSobel(bufor_czarnobialy, bufor_cbOut, 320, 240, 16);
			Odszumianie(bufor_cbOut, bufor_czarnobialy, 320, 240, 16);
			nCzasOper = HAL_GetTick();
			Dylatacja(bufor_czarnobialy, bufor_cbOut, 320, 240, 16);
			nCzasOper = MinalCzas(nCzasOper);
			KonwersjaCB7doRGB565(bufor_cbOut, (uint16_t*)bufor_okna, 320*240);
			nCzasCalk = MinalCzas(nCzasCalk);
			BSP_LCD_SelectLayer(1);
			BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
			sprintf(chNapis, "to=%ld, tc=%ld ms ", nCzasOper, nCzasCalk);
			BSP_LCD_DisplayStringAt(200, 225, (uint8_t*)chNapis, LEFT_MODE);
		}
		BSP_CAMERA_SnapshotStart(bufor_kamery);
		break;

	case TW_JULIA:
		nCzasOper = HAL_GetTick();
		GenerateJulia(320, 240, 160, 120, 135, (unsigned short *)bufor_okna);
		fImag -= 0.002;
		nCzasOper = MinalCzas(nCzasOper);

		BSP_LCD_SelectLayer(1);
		sprintf(chNapis, "Julia: t=%ldms, c=%.3f ", nCzasOper, fImag);
		BSP_LCD_DisplayStringAt(6, 225, (uint8_t*)chNapis, LEFT_MODE);
		break;

	case TW_MANDELBROT:
		nCzasOper = HAL_GetTick();
		GenerateMandelbrot(fX, fY, fZoom, 30, (unsigned short *)bufor_okna);
		chMnozPalety += 1;
		nCzasOper = MinalCzas(nCzasOper);

		BSP_LCD_SelectLayer(1);
		sprintf(chNapis, "Mandelbrot: t=%ldms z=%.1f, p=%d", nCzasOper, fZoom, chMnozPalety);
		BSP_LCD_DisplayStringAt(6, 225, (uint8_t*)chNapis, LEFT_MODE);
		break;

	default:	break;
	}
  }
  /* USER CODE END 5 */
}

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x20010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_128KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER2;
  MPU_InitStruct.BaseAddress = 0x2004C000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_512B;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
