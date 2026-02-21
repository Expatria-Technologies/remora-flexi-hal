#ifndef STM32F4_ETHComms_H
#define STM32F4_ETHComms_H

#include "remora-core/configuration.h"  
#ifdef ETH_CTRL // only compile this if ETH_CTRL and libraries are set up in platformio.ini

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"

#include <stdio.h>
#include <string.h>
#include <memory>

#include "../remora-core/remora.h"
#include "../remora-core/comms/commsInterface.h"
#include "../remora-core/modules/moduleInterrupt.h"
#include "hal_utils.h"

class STM32F4_EthComms : public CommsInterface {
    private:
        SPI_HandleTypeDef           spiHandle;
        DMA_HandleTypeDef           hdma_spi_tx;
        DMA_HandleTypeDef           hdma_spi_rx;          

        std::string                 mosiPortAndPin; 
        std::string                 misoPortAndPin; 
        std::string                 clkPortAndPin; 
        std::string                 csPortAndPin; 
        std::string                 rstPortAndPin;

        PinName                     mosiPinName;
        PinName                     misoPinName;
        PinName                     clkPinName;
        PinName                     csPinName;
        PinName                     rstPinName;

        Pin*                        mosiPin;
        Pin*                        misoPin;
        Pin*                        clkPin;
        Pin*                        csPin;
        Pin*                        rstPin;

        ModuleInterrupt<STM32F4_EthComms>*	spiInterrupt;        
        ModuleInterrupt<STM32F4_EthComms>*	dmaTxInterrupt;
        ModuleInterrupt<STM32F4_EthComms>*	dmaRxInterrupt;

        IRQn_Type					irqDMArx;
        IRQn_Type					irqDMAtx;

        bool newWriteData;
        bool newDataFlagged;

        void initSPIDMA(DMA_Stream_TypeDef* DMA_RX_Stream, DMA_Stream_TypeDef* DMA_TX_Stream, uint32_t DMA_channel);
        void handleRxInterrupt(void);
        void handleTxInterrupt(void);        
        void handleSPIInterrupt(void);
        
    public:   
        STM32F4_EthComms(volatile rxData_t*, volatile txData_t*, std::string, std::string, std::string, std::string, std::string);
        virtual ~STM32F4_EthComms();

        void init(void);
        void start(void);
        void tasks(void);

        uint8_t read_byte(void) override;
        uint8_t write_byte(uint8_t) override;
        void DMA_write(uint8_t*, uint16_t) override;
        void DMA_read(uint8_t*, uint16_t) override;
        virtual void flag_new_data(void) override;
};

#endif

#endif