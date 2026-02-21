/*
Remora firmware for LinuxCNC
Copyright (C) 2025  Scott Alford (aka scotta)


STM32F4 Port by Ben Jacobson.
Credits to Cakeslob and Expatria Technologies for their Ethernet communications work ported into this project.
Modified for the Flexi-HAL by Mike MacWillie.


This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "main.h"

#include <stdio.h>
#include <cstring>
#include <sys/errno.h>

#include "remora-core/remora.h"
#include "remora-hal/board_led_status.h"

#ifdef ETH_CTRL
    #include "remora-hal/STM32F4_EthComms.h"
#else
    #include "remora-hal/STM32F4_SPIComms.h"
#endif

#include "remora-hal/STM32F4_timer.h"

UART_HandleTypeDef uart_handle;
SD_HandleTypeDef hsd;

void SystemClock_Config(void);
static void MX_UART_Init(void);

#ifdef SPI_CTRL
    static void MX_SDIO_SD_Init(void);
#endif

//#define THREAD_DEBUG
#ifdef THREAD_DEBUG
Pin* thread_debug = nullptr;
#endif

// re-target printf to defined UART by redeclaring weak function in syscalls.c
extern "C" {
    int _write(int file, char *ptr, int len) {
        HAL_UART_Transmit(&uart_handle, (uint8_t*)ptr, len, HAL_MAX_DELAY);
        return len;
    }
}

int main(void)
{
    #ifdef HAS_BOOTLOADER
        HAL_RCC_DeInit();
        HAL_DeInit();
        extern uint8_t _FLASH_VectorTable;
        __disable_irq();
        SCB->VTOR = (uint32_t)&_FLASH_VectorTable;
        __DSB();
        __enable_irq();
    #endif   

    HAL_Init();
    SystemClock_Config();
    init_board_status_led("PB_10"); //TODO - Make sure this is actually okay. KPSTR.
    MX_UART_Init(); 

    #ifndef STATIC_CONFIG
        MX_SDIO_SD_Init(); 
        MX_FATFS_Init();
    #else
        printf("Using static config.\n");
    #endif   


    HAL_Delay(2000); 
    printf("Initialising Remora...\n");
    printf("CPU Clock: %u...\n\n\r", HAL_RCC_GetSysClockFreq());


    #ifdef THREAD_DEBUG
        thread_debug = new Pin("PC_3", GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, 0);
    #endif

    std::unique_ptr<CommsInterface> comms;

    #ifdef ETH_CTRL
        #ifndef WIZ_RST
            #error "Please configure your WIZ_RST pin in platformio.ini"
        #endif
        comms = std::make_unique<STM32F4_EthComms>(&rxData, &txData, SPI_MOSI, SPI_MISO, SPI_CLK, SPI_CS, WIZ_RST);
    #else
        comms = std::make_unique<STM32F4_SPIComms>(&rxData, &txData, SPI_MOSI, SPI_MISO, SPI_CLK, SPI_CS);
    #endif

  	auto commsHandler = std::make_shared<CommsHandler>();
    commsHandler->setInterface(std::move(comms));

    auto baseTimer = std::make_unique<STM32F4_timer>(TIM3, TIM3_IRQn, Config::pruBaseFreq, nullptr, Config::baseThreadIrqPriority);
    auto servoTimer = std::make_unique<STM32F4_timer>(TIM2, TIM2_IRQn, Config::pruServoFreq, nullptr, Config::servoThreadIrqPriority);
    //auto serialTimer = std::make_unique<STM32F4_timer>(TIM4, TIM4_IRQn, Config::pruSerialFreq, nullptr, Config::serialThreadIrqPriority); // Disabled in this build

    Remora* remora = new Remora(
            commsHandler,
            std::move(baseTimer),
            std::move(servoTimer),
            nullptr //std::move(serialTimer)  // serialTimer disabled on this build
    );

    remora->run();
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
  RCC_OscInitStruct.PLL.PLLQ = 8;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
        flash_led_error(CRITICAL_HAL_ERROR);
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
        flash_led_error(CRITICAL_HAL_ERROR);
        Error_Handler();
  }
}

static void MX_UART_Init(void)
{
    #ifdef UART_PORT
        uart_handle.Instance = UART_PORT;
    #else
        uart_handle.Instance = USART2;
    #endif
    uart_handle.Init.BaudRate = Config::pcBaud;
    uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    uart_handle.Init.StopBits = UART_STOPBITS_1;
    uart_handle.Init.Parity = UART_PARITY_NONE;
    uart_handle.Init.Mode = UART_MODE_TX_RX;
    uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&uart_handle) != HAL_OK)
    {
        flash_led_error(CRITICAL_HAL_ERROR);
        Error_Handler();
    }
}

static void MX_SDIO_SD_Init(void)
{
    hsd.Instance = SDIO;
    hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    hsd.Init.BusWide = SDIO_BUS_WIDE_1B; // SDIO_BUS_WIDE_4B; // We initialise in 1B mode,this is re-init later as 4bit
    hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd.Init.ClockDiv = 118;

    if (HAL_SD_Init(&hsd) != HAL_OK)
    {
        flash_led_error(SD_CARD_HW_ERROR);
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1);
}