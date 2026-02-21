#include "hal_utils.h"

#ifndef ETH_CTRL
const uint8_t _ls_json_upload_sector = 0; // clear compiler errors when linker script isn't used.
const uint8_t _ls_json_storage_sector = 0;     
#endif

extern SD_HandleTypeDef hsd;
extern DMA_HandleTypeDef hdma_sdio_rx;
extern DMA_HandleTypeDef hdma_sdio_tx;

PWMName getPWMName(PinName pwm_pin)        
{
    return (PWMName)pinmap_peripheral(pwm_pin, PinMap_PWM);
}

SPIName getSPIPeripheralName(PinName mosi, PinName miso, PinName sclk)        
{
    SPIName spi_mosi = (SPIName)pinmap_peripheral(mosi, PinMap_SPI_MOSI);
    SPIName spi_miso = (SPIName)pinmap_peripheral(miso, PinMap_SPI_MISO);
    SPIName spi_sclk = (SPIName)pinmap_peripheral(sclk, PinMap_SPI_SCLK);

    SPIName spi_per;

    // MISO or MOSI may be not connected
    if (miso == NC) {
        spi_per = (SPIName)pinmap_merge(spi_mosi, spi_sclk);
    } else if (mosi == NC) {
        spi_per = (SPIName)pinmap_merge(spi_miso, spi_sclk);
    } else {
        SPIName spi_data = (SPIName)pinmap_merge(spi_mosi, spi_miso);
        spi_per = (SPIName)pinmap_merge(spi_data, spi_sclk);
    }

    return spi_per;
}

void enableSPIClocks(SPI_TypeDef* spi_instance) 
{
    if (spi_instance == SPI1) __HAL_RCC_SPI1_CLK_ENABLE();
    else if (spi_instance == SPI2) __HAL_RCC_SPI2_CLK_ENABLE();
    else if (spi_instance == SPI3) __HAL_RCC_SPI3_CLK_ENABLE();
    else if (spi_instance == SPI4) __HAL_RCC_SPI4_CLK_ENABLE();
}

void initDMAClocks(SPI_TypeDef* spi_instance)     // todo - remove entirely. 
{
    if (spi_instance == SPI1)
    {
        __HAL_RCC_DMA2_CLK_ENABLE();            
    }
    else if (spi_instance == SPI2)
    {
        __HAL_RCC_DMA1_CLK_ENABLE();  
    }
    else if (spi_instance == SPI3)
    {
        __HAL_RCC_DMA1_CLK_ENABLE();  
    }
    else
    {
        printf("Invalid SPI bus selected, please check your SPI configuration in your PlatformIO SPI1-3\n");
        return;
    }
}

Pin* createPinFromPinMap(const std::string& portAndPin, PinName pinName, const PinMap* map, uint32_t gpio_mode, uint32_t gpio_pull, uint32_t gpio_speed) 
{ 
    uint32_t function = STM_PIN_AFNUM(pinmap_function(pinName, map));
    return new Pin(portAndPin, gpio_mode, gpio_pull, gpio_speed, function);
}

void delay_ms(uint32_t ms) 
{
    HAL_Delay(ms);
}

void mass_erase_config_storage(void) 
{
    mass_erase_flash_sector(Platform_Config::JSON_Config_Storage_Sector);
}

void mass_erase_upload_storage(void) 
{
    mass_erase_flash_sector(Platform_Config::JSON_Config_Upload_Sector);
}

void mass_erase_flash_sector(uint32_t Sector) {
    FLASH_EraseInitTypeDef FLASH_EraseInitStruct;
    uint32_t error = 0;

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
    
    FLASH_EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    FLASH_EraseInitStruct.Sector = Sector;
    FLASH_EraseInitStruct.NbSectors = 1;
    FLASH_EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    
    if (HAL_FLASHEx_Erase(&FLASH_EraseInitStruct, &error) != HAL_OK) {
        Error_Handler();
    }
}
