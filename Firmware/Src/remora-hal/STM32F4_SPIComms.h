#ifndef STM32F4_SPIComms_H
#define STM32F4_SPIComms_H

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"

#include <memory>
#include <algorithm>

#include "../remora-core/remora.h"
#include "../remora-core/comms/commsInterface.h"
#include "../remora-core/modules/moduleInterrupt.h"
#include "hal_utils.h"

typedef enum {
    DMA_HALF_TRANSFER = 1,          // Half-transfer completed
    DMA_TRANSFER_COMPLETE = 2,      // Full transfer completed
    DMA_OTHER = 3                   // Other or error status
} DMA_TransferStatus_t;

class STM32F4_SPIComms : public CommsInterface {
private:
    volatile DMA_RxBuffer_t* 	ptrRxDMABuffer;

    SPI_TypeDef*        		spiType;
    SPI_HandleTypeDef   		spiHandle; 

    DMA_HandleTypeDef   		hdma_spi_tx;
    DMA_HandleTypeDef   		hdma_spi_rx;
    DMA_HandleTypeDef   		hdma_memtomem;
    HAL_StatusTypeDef   		dmaStatus;

    std::string                 mosiPortAndPin; 
    std::string                 misoPortAndPin; 
    std::string                 clkPortAndPin; 
    std::string                 csPortAndPin; 

    PinName                     mosiPinName;
    PinName                     misoPinName;
    PinName                     clkPinName;
    PinName                     csPinName;

    Pin*                        mosiPin;
    Pin*                        misoPin;
    Pin*                        clkPin;
    Pin*                        csPin;

    uint8_t						RXbufferIdx;
    bool						copyRXbuffer;

    ModuleInterrupt<STM32F4_SPIComms>*	spiInterrupt; 
	ModuleInterrupt<STM32F4_SPIComms>*	NssInterrupt;
    ModuleInterrupt<STM32F4_SPIComms>*	dmaTxInterrupt;
	ModuleInterrupt<STM32F4_SPIComms>*	dmaRxInterrupt;

	IRQn_Type					irqNss;
	IRQn_Type					irqDMArx;
	IRQn_Type					irqDMAtx;

    bool						newWriteData;

    void initSPIDMA(DMA_Stream_TypeDef* DMA_RX_Stream, DMA_Stream_TypeDef* DMA_TX_Stream, uint32_t DMA_channel);

    HAL_StatusTypeDef startMultiBufferDMASPI(uint8_t*, uint8_t*, uint8_t*, uint16_t);
	void handleRxInterrupt(void);
	void handleTxInterrupt(void);
	void handleNssInterrupt(void);

public:
    static STM32F4_SPIComms* instance;    
    static volatile uint8_t RxDMAmemoryIdx;

    STM32F4_SPIComms(volatile rxData_t*, volatile txData_t*, std::string, std::string, std::string, std::string);
	virtual ~STM32F4_SPIComms();

    void init(void);
    void start(void);
    void tasks(void);
    void CheckHeader(void);
};

#endif

