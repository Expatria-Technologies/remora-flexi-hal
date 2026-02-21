#include "STM32F4_EthComms.h"
#include "../remora-core/drivers/W5500_Networking/W5500_Networking.h"

#include "remora-core/configuration.h"  
#ifdef ETH_CTRL

//extern volatile DMA_RxBuffer_t rxDMABuffer; // if you don't also have the SPI comms interface, remove extern

STM32F4_EthComms::STM32F4_EthComms(volatile rxData_t* _ptrRxData, volatile txData_t* _ptrTxData, std::string _mosiPortAndPin, std::string _misoPortAndPin, std::string _clkPortAndPin, std::string _csPortAndPin, std::string _rstPortAndPin) :
    mosiPortAndPin(_mosiPortAndPin),
    misoPortAndPin(_misoPortAndPin),
	clkPortAndPin(_clkPortAndPin),
    csPortAndPin(_csPortAndPin),
    rstPortAndPin(_rstPortAndPin),
    newDataFlagged(false)
{
    ptrRxData = _ptrRxData;
	ptrTxData = _ptrTxData;

    //ptrRxDMABuffer = &rxDMABuffer;

    mosiPinName = portAndPinToPinName(mosiPortAndPin.c_str());
    misoPinName = portAndPinToPinName(misoPortAndPin.c_str());
    clkPinName = portAndPinToPinName(clkPortAndPin.c_str());
    csPinName = portAndPinToPinName(csPortAndPin.c_str());
    rstPinName = portAndPinToPinName(rstPortAndPin.c_str());

    // irqNss = SPI_CS_IRQ; // need to find a new way to interrupt and advise that new data is waiting to be streamed in. May not even be interrupt based. 
}

STM32F4_EthComms::~STM32F4_EthComms() {

}

void STM32F4_EthComms::init(void) {
    printf("EthComms Init\n");

    spiHandle.Instance = (SPI_TypeDef* )getSPIPeripheralName(mosiPinName, misoPinName, clkPinName); // get the SPI handle from the list of GPIO used for Mosi, Miso, SCK

    //InitDMAIRQs(spiHandle.Instance); 
    initDMAClocks(spiHandle.Instance);
    enableSPIClocks(spiHandle.Instance);

    printf("initialising SPI pins\n");
    csPin = new Pin(csPortAndPin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, 0);  
    rstPin = new Pin(rstPortAndPin, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_VERY_HIGH, 0);

    // Create alternate function SPI pins
    mosiPin = createPinFromPinMap(mosiPortAndPin, mosiPinName, PinMap_SPI_MOSI);
    misoPin = createPinFromPinMap(misoPortAndPin, misoPinName, PinMap_SPI_MISO);
    clkPin  = createPinFromPinMap(clkPortAndPin,  clkPinName,  PinMap_SPI_SCLK);

    printf("Setting up SPI and DMA\n");

    IRQn_Type irqSPI; 

    if (spiHandle.Instance == SPI1)
    {
        printf("Initialising SPI1 DMA\n");
        irqSPI = SPI1_IRQn;
        initSPIDMA(DMA2_Stream0, DMA2_Stream3, DMA_CHANNEL_3);       
        irqDMArx = DMA2_Stream0_IRQn;           
        irqDMAtx = DMA2_Stream3_IRQn; 
    }
    else if (spiHandle.Instance == SPI2)
    {
        printf("Initialising SPI2 DMA\n");
        irqSPI = SPI2_IRQn;
        initSPIDMA(DMA1_Stream3, DMA1_Stream4, DMA_CHANNEL_0);     
        irqDMArx = DMA1_Stream3_IRQn;           
        irqDMAtx = DMA1_Stream4_IRQn;         
    }
    else if (spiHandle.Instance == SPI3)
    {
        printf("Initialising SPI3 DMA\n");
        irqSPI = SPI3_IRQn;
        initSPIDMA(DMA1_Stream0, DMA1_Stream5, DMA_CHANNEL_0);                  
        irqDMArx = DMA1_Stream0_IRQn;           
        irqDMAtx = DMA1_Stream5_IRQn;           
    }
    else
    {
        printf("Invalid SPI bus selected, please check your SPI configuration in your PlatformIO SPI1-3\n");
        return;
    }    

    // Register our new SPI Interrupts so that we can handle the callback internally
    spiInterrupt = new ModuleInterrupt<STM32F4_EthComms>(
        irqSPI,
        this,
        &STM32F4_EthComms::handleSPIInterrupt
    );
    HAL_NVIC_SetPriority(irqSPI, 0, 0);
    HAL_NVIC_EnableIRQ(irqSPI); 

    // Register the DMA Rx interrupt so that we can handle the callback internally
    dmaRxInterrupt = new ModuleInterrupt<STM32F4_EthComms>(
        irqDMArx,
        this,
        &STM32F4_EthComms::handleRxInterrupt
    );
    HAL_NVIC_SetPriority(irqDMArx, Config::spiDmaRxIrqPriority, 0);
    HAL_NVIC_EnableIRQ(irqDMArx);

    // Register the DMA Rx interrupt so that we can handle the callback internally
    dmaTxInterrupt = new ModuleInterrupt<STM32F4_EthComms>(
        irqDMAtx,
        this,
        &STM32F4_EthComms::handleTxInterrupt
    );
    HAL_NVIC_SetPriority(irqDMAtx, Config::spiDmaTxIrqPriority, 0); // TX needs higher priority than RX
    HAL_NVIC_EnableIRQ(irqDMAtx);    

    // Note that spiHandle.instance is set up in the class constructor, we now need to init the rest of SPI handle
    spiHandle.Init.Mode           			= SPI_MODE_MASTER;
    spiHandle.Init.Direction      			= SPI_DIRECTION_2LINES;
    spiHandle.Init.DataSize       			= SPI_DATASIZE_8BIT;
    spiHandle.Init.CLKPolarity    			= SPI_POLARITY_LOW;
    spiHandle.Init.CLKPhase       			= SPI_PHASE_1EDGE;
    spiHandle.Init.NSS            			= SPI_NSS_SOFT; 
    spiHandle.Init.BaudRatePrescaler        = SPI_BAUDRATEPRESCALER_2; 
    spiHandle.Init.FirstBit       			= SPI_FIRSTBIT_MSB;
    spiHandle.Init.TIMode         			= SPI_TIMODE_DISABLE;
    spiHandle.Init.CRCCalculation 			= SPI_CRCCALCULATION_DISABLE;
    spiHandle.Init.CRCPolynomial  			= 10; 

    if (HAL_SPI_Init(&this->spiHandle) != HAL_OK)
    {
        printf("Error initialising SPI\n");
    }
}

void STM32F4_EthComms::initSPIDMA(DMA_Stream_TypeDef* DMA_RX_Stream, DMA_Stream_TypeDef* DMA_TX_Stream, uint32_t DMA_channel) {
    // RX
    hdma_spi_rx.Instance = DMA_RX_Stream;
    hdma_spi_rx.Init.Channel = DMA_channel; 
    hdma_spi_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;    
    hdma_spi_rx.Init.Mode = DMA_NORMAL;
    hdma_spi_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_spi_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_spi_rx) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(&spiHandle, hdmarx, hdma_spi_rx);

    // // TX
    hdma_spi_tx.Instance = DMA_TX_Stream;
    hdma_spi_tx.Init.Channel = DMA_channel;
    hdma_spi_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi_tx.Init.Mode = DMA_NORMAL;
    hdma_spi_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_spi_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_spi_tx) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(&spiHandle, hdmatx, hdma_spi_tx);
}

void STM32F4_EthComms::start(void) {
    printf("Initializing Network. Ensure ethernet cable is plugged in\n");
    network::EthernetInit(this, csPin, rstPin);
}

void STM32F4_EthComms::handleRxInterrupt(void)
{
    HAL_DMA_IRQHandler(&hdma_spi_rx);
}

void STM32F4_EthComms::handleTxInterrupt(void) 
{
    HAL_DMA_IRQHandler(&hdma_spi_tx);
}  

void STM32F4_EthComms::handleSPIInterrupt(void) 
{
    HAL_SPI_IRQHandler(&spiHandle);
}  

void STM32F4_EthComms::tasks(void) {    
    network::EthernetTasks();

    if (newDataFlagged)
    {
        newDataFlagged = false;

        switch (ptrRxData->header)
        {
            case Config::pruRead:
                // No action needed for PRU_READ.
                dataCallback(true);
                break;

            case Config::pruWrite:
                // Valid PRU_WRITE header, flag RX data transfer.
                dataCallback(true);
                newWriteData = true;
                //RXbufferIdx = RxDMAmemoryIdx;
                break;

            default:
                dataCallback(false);
                break;
        }
    }    
}

uint8_t STM32F4_EthComms::read_byte(void)
{
	spiHandle.Instance->DR = 0xFF; // Writing dummy data into Data register
    while(!__HAL_SPI_GET_FLAG(&spiHandle, SPI_FLAG_RXNE));
    return (uint8_t)spiHandle.Instance->DR;
    
    // uint8_t tx = 0xFF;
    // uint8_t rx = 0;

    // if (HAL_SPI_TransmitReceive(&spiHandle, &tx, &rx, 1, HAL_MAX_DELAY) != HAL_OK) {
    //     Error_Handler();
    // }

    // return rx;    
}

uint8_t STM32F4_EthComms::write_byte(uint8_t byte)
{
	spiHandle.Instance->DR = byte;
    while(!__HAL_SPI_GET_FLAG(&spiHandle, SPI_FLAG_TXE));
    while(!__HAL_SPI_GET_FLAG(&spiHandle, SPI_FLAG_RXNE));
    __HAL_SPI_CLEAR_OVRFLAG(&spiHandle);
    return (uint8_t)spiHandle.Instance->DR;

    // uint8_t rx = 0;

    // if (HAL_SPI_TransmitReceive(&spiHandle, &byte, &rx, 1, HAL_MAX_DELAY) != HAL_OK) {
    //     Error_Handler();
    // }

    // return rx;    
}

void STM32F4_EthComms::DMA_write(uint8_t *data, uint16_t len)
{
    if(HAL_SPI_Transmit_DMA(&spiHandle, data, len) == HAL_OK)
        while(spiHandle.State != HAL_SPI_STATE_READY);
}

void STM32F4_EthComms::DMA_read(uint8_t *data, uint16_t len)
{
    if (HAL_SPI_Receive_DMA(&spiHandle, data, len) == HAL_OK)
        while(spiHandle.State != HAL_SPI_STATE_READY);
}

void STM32F4_EthComms::flag_new_data(void) 
{
    newDataFlagged = true;
}

#endif