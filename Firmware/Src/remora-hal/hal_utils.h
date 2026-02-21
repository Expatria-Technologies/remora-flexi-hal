#ifndef HAL_UTILS_H
#define HAL_UTILS_H

#include "main.h"
#include "remora-hal/platform_configuration.h"
#include "pin/pin.h"
#include "peripheralPins.h"
#include "pinNames.h"
#include "PinNamesTypes.h" 
#include <string.h>

// Macro's for Remora Core, these map 1:1 in STM32, but in other platforms will require custom  calls to replace functionality. 
#define lock_flash          HAL_FLASH_Lock
#define unlock_flash        HAL_FLASH_Unlock
#define mass_erase_flash    HAL_FLASHEx_Erase
#define pru_reboot          HAL_NVIC_SystemReset

#define write_to_flash_byte(addr, data)       HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,       addr, data)
#define write_to_flash_halfword(addr, data)   HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,   addr, data)
#define write_to_flash_word(addr, data)       HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,       addr, data)
#define write_to_flash_doubleword(addr, data) HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, data)

PWMName getPWMName(PinName);
SPIName getSPIPeripheralName(PinName, PinName, PinName);
void enableSPIClocks(SPI_TypeDef *);
void initDMAClocks(SPI_TypeDef *);

Pin* createPinFromPinMap(const std::string&, PinName, const PinMap*,
               uint32_t = GPIO_MODE_AF_PP,
               uint32_t = GPIO_NOPULL,
               uint32_t = GPIO_SPEED_FREQ_VERY_HIGH);

void delay_ms(uint32_t);

void mass_erase_config_storage(void);
void mass_erase_upload_storage(void);
void mass_erase_flash_sector(uint32_t);

#endif