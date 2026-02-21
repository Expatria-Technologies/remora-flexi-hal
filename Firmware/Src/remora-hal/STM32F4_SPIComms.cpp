#include "STM32F4_SPIComms.h"
#include "stm32f4xx_ll_dma.h"

static volatile uint8_t HDMARXinterruptType = DMA_OTHER;
static volatile uint8_t HDMATXinterruptType = DMA_OTHER; 

static void DMA_RxHalfCplt_Callback(DMA_HandleTypeDef *hdma);
static void DMA_RxCplt_Callback(DMA_HandleTypeDef *hdma);
static void DMA_RxM1Cplt_Callback(DMA_HandleTypeDef *hdma);
static void DMA_RxM1HalfCplt_Callback(DMA_HandleTypeDef *hdma);
static void DMA_RxError_Callback(DMA_HandleTypeDef *hdma);
static void DMA_TxCplt_Callback(DMA_HandleTypeDef *hdma);
static void DMA_TxError_Callback(DMA_HandleTypeDef *hdma);

volatile DMA_RxBuffer_t rxDMABuffer;
volatile uint8_t STM32F4_SPIComms::RxDMAmemoryIdx = 1;  // default to alternate buffer so as to not miss first write packet. 

STM32F4_SPIComms* STM32F4_SPIComms::instance = nullptr; 

STM32F4_SPIComms::STM32F4_SPIComms(volatile rxData_t* _ptrRxData, volatile txData_t* _ptrTxData, std::string _mosiPortAndPin, std::string _misoPortAndPin, std::string _clkPortAndPin, std::string _csPortAndPin) :
    mosiPortAndPin(_mosiPortAndPin),
    misoPortAndPin(_misoPortAndPin),
	clkPortAndPin(_clkPortAndPin),
    csPortAndPin(_csPortAndPin)
{
    STM32F4_SPIComms::instance = this;  // need to access some internal functions statically from outside of the class. 

    ptrRxData = _ptrRxData;
	ptrTxData = _ptrTxData;

    copyRXbuffer = false;
    newWriteData = false;

    mosiPinName = portAndPinToPinName(mosiPortAndPin.c_str());
    misoPinName = portAndPinToPinName(misoPortAndPin.c_str());
    clkPinName = portAndPinToPinName(clkPortAndPin.c_str());
    csPinName = portAndPinToPinName(csPortAndPin.c_str());

    ptrRxDMABuffer = &rxDMABuffer;
}

STM32F4_SPIComms::~STM32F4_SPIComms() 
{
    // do nothing for now. 
}

void STM32F4_SPIComms::init() {
    HAL_Delay(100);  // Experienced some stability issues when initialising SPI so soon after SDIO has finished. Weird..

    printf("SPIComms Init\n");

    spiHandle.Instance = (SPI_TypeDef* )getSPIPeripheralName(mosiPinName, misoPinName, clkPinName);

    initDMAClocks(spiHandle.Instance);  
    enableSPIClocks(spiHandle.Instance);

    // Configure the NSS (chip select) pin as interrupt
    csPin = new Pin(csPortAndPin, GPIO_MODE_IT_RISING, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0);
    delete csPin;

    printf("initialising SPI bus\n");
    spiHandle.Init.Mode           			= SPI_MODE_SLAVE;
    spiHandle.Init.Direction      			= SPI_DIRECTION_2LINES;
    spiHandle.Init.DataSize       			= SPI_DATASIZE_8BIT;
    spiHandle.Init.CLKPolarity    			= SPI_POLARITY_LOW;    // CPOL = 0
    spiHandle.Init.CLKPhase       			= SPI_PHASE_1EDGE;     // CPHA = 0 . Mode 0
    spiHandle.Init.NSS            			= SPI_NSS_HARD_INPUT; 
    spiHandle.Init.FirstBit       			= SPI_FIRSTBIT_MSB;
    spiHandle.Init.TIMode         			= SPI_TIMODE_DISABLE;
    spiHandle.Init.CRCCalculation 			= SPI_CRCCALCULATION_DISABLE;
    spiHandle.Init.CRCPolynomial  			= 10; 

    if (HAL_SPI_Init(&this->spiHandle) != HAL_OK) 
    {
        printf("Error initialising SPI\n");
    }    

    printf("initialising SPI pins\n");

    // Create alternate function SPI pins
    mosiPin = createPinFromPinMap(mosiPortAndPin, mosiPinName, PinMap_SPI_MOSI);
    misoPin = createPinFromPinMap(misoPortAndPin, misoPinName, PinMap_SPI_MISO);
    clkPin  = createPinFromPinMap(clkPortAndPin,  clkPinName,  PinMap_SPI_SCLK);
    csPin   = createPinFromPinMap(csPortAndPin,   csPinName,   PinMap_SPI_SSEL);

    printf("Setting up SPI DMA\n");
    irqNss = SPI_CS_IRQ;

    if (spiHandle.Instance == SPI1)
    {
        printf("Initialising SPI1 DMA\n");
        initSPIDMA(DMA2_Stream0, DMA2_Stream3, DMA_CHANNEL_3);
        irqDMArx = DMA2_Stream0_IRQn;
        irqDMAtx = DMA2_Stream3_IRQn;  
    }
    else if (spiHandle.Instance == SPI2)
    {
        printf("Initialising SPI2 DMA\n");   
        initSPIDMA(DMA1_Stream3, DMA1_Stream4, DMA_CHANNEL_0);
        irqDMArx = DMA1_Stream3_IRQn;
        irqDMAtx = DMA1_Stream4_IRQn;             
    }
    else if (spiHandle.Instance == SPI3)
    {
        printf("Initialising SPI3 DMA\n");
        initSPIDMA(DMA1_Stream0, DMA1_Stream5, DMA_CHANNEL_0);
        irqDMArx = DMA1_Stream0_IRQn;
        irqDMAtx = DMA1_Stream5_IRQn;            
    }
    else
    {
        printf("Unknown SPI instance, please configure in your PlatformIO SPI1-3\n");
        return;
    }

    // Register the NSS (slave select) interrupt
    NssInterrupt = new ModuleInterrupt<STM32F4_SPIComms>(
        irqNss,
        this,
        &STM32F4_SPIComms::handleNssInterrupt
    );
    HAL_NVIC_SetPriority(irqNss, Config::spiNssIrqPriority, 0);
    HAL_NVIC_EnableIRQ(irqNss);

    // Register the DMA Rx interrupt
    dmaRxInterrupt = new ModuleInterrupt<STM32F4_SPIComms>(
        irqDMArx,
        this,
        &STM32F4_SPIComms::handleRxInterrupt
    );
    HAL_NVIC_SetPriority(irqDMArx, Config::spiDmaRxIrqPriority, 0);
    HAL_NVIC_EnableIRQ(irqDMArx);

    // Register the DMA Tx interrupt
    dmaTxInterrupt = new ModuleInterrupt<STM32F4_SPIComms>(
        irqDMAtx,
        this,
        &STM32F4_SPIComms::handleTxInterrupt
    );
    HAL_NVIC_SetPriority(irqDMAtx, Config::spiDmaTxIrqPriority, 0); // TX needs higher priority than RX
    HAL_NVIC_EnableIRQ(irqDMAtx); 

    printf("Initialising DMA for Memory to Memory transfer\n");

    hdma_memtomem.Instance 					= DMA2_Stream1;         // F4 doesn't have dedicated mem2mem so will manually use DMA1. No interrupt needed as this memtomem is polled
    hdma_memtomem.Init.Channel 				= DMA_CHANNEL_0;        // aparently not needed for mem2mem but using an unused one anyway. 
    hdma_memtomem.Init.Direction 			= DMA_MEMORY_TO_MEMORY;
    hdma_memtomem.Init.PeriphInc 			= DMA_PINC_ENABLE;
    hdma_memtomem.Init.MemInc 				= DMA_MINC_ENABLE;
    hdma_memtomem.Init.PeriphDataAlignment 	= DMA_PDATAALIGN_BYTE;
    hdma_memtomem.Init.MemDataAlignment 	= DMA_MDATAALIGN_BYTE;
    hdma_memtomem.Init.Mode 				= DMA_NORMAL;
    hdma_memtomem.Init.Priority 			= DMA_PRIORITY_MEDIUM;
    hdma_memtomem.Init.FIFOMode 			= DMA_FIFOMODE_ENABLE;
    hdma_memtomem.Init.FIFOThreshold 		= DMA_FIFO_THRESHOLD_FULL;
    hdma_memtomem.Init.MemBurst 			= DMA_MBURST_SINGLE;
    hdma_memtomem.Init.PeriphBurst 			= DMA_PBURST_SINGLE;

    if (HAL_DMA_Init(&hdma_memtomem) != HAL_OK) 
    {
        printf("Error initialising SPI mem to mem\n");
    }
}

void STM32F4_SPIComms::initSPIDMA(DMA_Stream_TypeDef* DMA_RX_Stream, DMA_Stream_TypeDef* DMA_TX_Stream, uint32_t DMA_channel) 
{
    // RX
    hdma_spi_rx.Instance = DMA_RX_Stream;
    hdma_spi_rx.Init.Channel = DMA_channel; 
    hdma_spi_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_spi_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi_rx.Init.Mode = DMA_CIRCULAR;
    hdma_spi_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH; 
    hdma_spi_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_spi_rx) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(&spiHandle, hdmarx, hdma_spi_rx);

    // TX
    hdma_spi_tx.Instance = DMA_TX_Stream;
    hdma_spi_tx.Init.Channel = DMA_channel;
    hdma_spi_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi_tx.Init.Mode = DMA_CIRCULAR;
    hdma_spi_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_spi_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_spi_tx) != HAL_OK)
    {
        Error_Handler();
    }

    __HAL_LINKDMA(&spiHandle, hdmatx, hdma_spi_tx);
}

void STM32F4_SPIComms::start() {

    // Initialize the data buffers
    std::fill(std::begin(ptrRxDMABuffer->buffer[0].rxBuffer), std::end(ptrRxDMABuffer->buffer[0].rxBuffer), 0);
    std::fill(std::begin(ptrRxDMABuffer->buffer[1].rxBuffer), std::end(ptrRxDMABuffer->buffer[1].rxBuffer), 0);

    //Start the multi-buffer DMA SPI communication
    dmaStatus = startMultiBufferDMASPI(
        (uint8_t*)ptrTxData->txBuffer,
        (uint8_t*)ptrRxDMABuffer->buffer[0].rxBuffer,
        (uint8_t*)ptrRxDMABuffer->buffer[1].rxBuffer,
        Config::dataBuffSize
    );

    //Check for DMA initialization errors
    if (dmaStatus != HAL_OK) 
    {
        printf("DMA SPI error\n");
    }    
}

HAL_StatusTypeDef STM32F4_SPIComms::startMultiBufferDMASPI(uint8_t *pTxBuffer,
                                                   uint8_t *pRxBuffer0, uint8_t *pRxBuffer1,
                                                   uint16_t Size)
{
    if (spiHandle.State != HAL_SPI_STATE_READY)
    {
        return HAL_BUSY;
    }

    if ((pRxBuffer0 == NULL) || (Size == 0UL))
    {
        return HAL_ERROR;
    }

    if (pRxBuffer1 == NULL) // can disable multibuffering by providing the RX buffer twice in the function arguments
    {
        pRxBuffer1 = pRxBuffer0;
    }

    // Lock the process 
    __HAL_LOCK(&spiHandle);

    // Set the transaction information 
    spiHandle.State       = HAL_SPI_STATE_BUSY_TX_RX;
    spiHandle.ErrorCode   = HAL_SPI_ERROR_NONE;
    spiHandle.TxXferSize  = Size;
    spiHandle.TxXferCount = Size;
    spiHandle.RxXferSize  = Size;
    spiHandle.RxXferCount = Size;

    // Init unused fields in handle to zero 
    spiHandle.RxISR       = NULL;
    spiHandle.TxISR       = NULL;
    
    __HAL_SPI_CLEAR_OVRFLAG(&spiHandle);  // Clear any stale OVR

    // Set Full-Duplex mode 
    CLEAR_BIT(spiHandle.Instance->CR1, SPI_CR1_BIDIMODE); 

    // Reset the Tx/Rx DMA bits 
    CLEAR_BIT(spiHandle.Instance->CR2, SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN); 

    // Clear bits SSOE, FRF, ERRIE, etc. while keeping TX/RX DMA enabled
    spiHandle.Instance->CR2 &= ~(SPI_CR2_SSOE | SPI_CR2_FRF | SPI_CR2_ERRIE);
    spiHandle.Instance->CR2 |= SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;

    // Register RX DMA callbacks 
    hdma_spi_rx.XferCpltCallback   = DMA_RxCplt_Callback;                   
    hdma_spi_rx.XferHalfCpltCallback = DMA_RxHalfCplt_Callback;
    hdma_spi_rx.XferM1CpltCallback = DMA_RxM1Cplt_Callback;                 
    hdma_spi_rx.XferM1HalfCpltCallback = DMA_RxM1HalfCplt_Callback;
    hdma_spi_rx.XferErrorCallback  = DMA_RxError_Callback;

    // Start RX DMA in double-buffer interrupt mode.
    if (HAL_DMAEx_MultiBufferStart_IT(&hdma_spi_rx,
                                    (uint32_t)&(spiHandle.Instance->DR),  
                                    (uint32_t)pRxBuffer0,       
                                    (uint32_t)pRxBuffer1,      
                                    Size) != HAL_OK)
    {
        Error_Handler();
    }

    // Enable SPI DMA requests (TX & RX) in SPI CR2 
    SET_BIT(spiHandle.Instance->CR2, SPI_CR2_RXDMAEN);

    // Register TX callbacks
    hdma_spi_tx.XferErrorCallback = DMA_TxError_Callback;    
    hdma_spi_tx.XferHalfCpltCallback = [](DMA_HandleTypeDef *hdma) {};      // not needed;  
    hdma_spi_tx.XferCpltCallback = DMA_TxCplt_Callback; 

    if (HAL_DMA_Start_IT(&hdma_spi_tx, (uint32_t)pTxBuffer, (uint32_t)&spiHandle.Instance->DR, Size) != HAL_OK) 
    {
        printf("TX DMA SPI error\n");
        (void)HAL_DMA_Abort(&hdma_spi_tx);
        __HAL_UNLOCK(&spiHandle);
        return HAL_ERROR;
    }
  
    // Enable SPI periph and error interrupt 
    __HAL_SPI_ENABLE(&spiHandle);
    __HAL_SPI_ENABLE_IT(&spiHandle, SPI_IT_ERR); 

    __HAL_UNLOCK(&spiHandle);

    return HAL_OK;
}

void STM32F4_SPIComms::handleNssInterrupt()
{
	// SPI packet has been fully received
	// Flag the copy the RX buffer if new WRITE data has been received
	// DMA copy is performed during the servo thread update
	if (newWriteData)
	{
		copyRXbuffer = true;
		newWriteData = false;
	}
}

void STM32F4_SPIComms::handleTxInterrupt()
{
    HAL_DMA_IRQHandler(&hdma_spi_tx); // We can probably be rid of these now, instead of using Interrupt::InvokeHandler, can just run directly in the IRQ, see irqHandlers.h
}

void STM32F4_SPIComms::handleRxInterrupt()
{
    // use default IRQ handler, note that interrupt type is set in the ISRs
    HAL_DMA_IRQHandler(&hdma_spi_rx);
}

void STM32F4_SPIComms::tasks() 
{
    if (copyRXbuffer == true)
    {
	    uint8_t* srcBuffer = (uint8_t*)ptrRxDMABuffer->buffer[RXbufferIdx].rxBuffer;
	    uint8_t* destBuffer = (uint8_t*)ptrRxData->rxBuffer;

	    __disable_irq();

	    dmaStatus = HAL_DMA_Start( 
	    							&hdma_memtomem,
									(uint32_t)srcBuffer,
									(uint32_t)destBuffer,
									Config::dataBuffSize
	    							);

	    if (dmaStatus == HAL_OK) {
	        dmaStatus = HAL_DMA_PollForTransfer(&hdma_memtomem, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
	    }

	    __enable_irq();
	    HAL_DMA_Abort(&hdma_memtomem);
		copyRXbuffer = false;
    }
}

void STM32F4_SPIComms::CheckHeader(void) 
{
    switch (ptrRxDMABuffer->buffer[RxDMAmemoryIdx].header)  
    {
        case Config::pruRead:
            // No action needed for PRU_READ.
            dataCallback(true);
            break;

        case Config::pruWrite:
            // Valid PRU_WRITE header, flag RX data transfer.
            dataCallback(true);
            newWriteData = true;
            RXbufferIdx = RxDMAmemoryIdx;
            break;

        default:
            dataCallback(false);
            break;
    }
}

static void DMA_RxHalfCplt_Callback(DMA_HandleTypeDef *hdma) // pRxBuffer0 half complete
{
    // Half Transfer complete, 
    HDMARXinterruptType = DMA_HALF_TRANSFER;

    // Process headers
    if (STM32F4_SPIComms::instance != nullptr) 
    {  
        STM32F4_SPIComms::instance->CheckHeader();
    }       

    // Flip the dma memory id. 
    STM32F4_SPIComms::RxDMAmemoryIdx = 0;     
}

static void DMA_RxCplt_Callback(DMA_HandleTypeDef *hdma) // pRxBuffer0 complete
{
    // full transfer complete, but stay in zero. 
    HDMARXinterruptType = DMA_TRANSFER_COMPLETE;
    STM32F4_SPIComms::RxDMAmemoryIdx = 0;             
}

static void DMA_RxM1HalfCplt_Callback(DMA_HandleTypeDef *hdma) // pRxBuffer1 half complete
{
    // Half Transfer complete
    HDMARXinterruptType = DMA_HALF_TRANSFER;
    
    // Process headers
    if (STM32F4_SPIComms::instance != nullptr) 
    {  
        STM32F4_SPIComms::instance->CheckHeader();
    }        

    // Flip the dma memory id. 
    STM32F4_SPIComms::RxDMAmemoryIdx = 1;   
}

static void DMA_RxM1Cplt_Callback(DMA_HandleTypeDef *hdma) // pRxBuffer1 complete
{
    // full transfer complete and ready to flip the dma memory id over to 1. 
    HDMARXinterruptType = DMA_TRANSFER_COMPLETE;
    STM32F4_SPIComms::RxDMAmemoryIdx = 1;         
}

static void DMA_RxError_Callback(DMA_HandleTypeDef *hdma)
{
    HDMARXinterruptType = DMA_OTHER; 
    Error_Handler();
}

static void DMA_TxCplt_Callback(DMA_HandleTypeDef *hdma)
{
    HDMATXinterruptType = DMA_TRANSFER_COMPLETE;  
}

static void DMA_TxError_Callback(DMA_HandleTypeDef *hdma)
{
    HDMATXinterruptType = DMA_OTHER; 
    Error_Handler();
}