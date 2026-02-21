#include "analogIn.h"
#include <cstdio>

AnalogIn::AnalogIn(const std::string& portAndPin) 
    : portAndPin(portAndPin)
{
    uint32_t function = (uint32_t)NC;

    if (portAndPin[0] == 'P') {
        portIndex = portAndPin[1] - 'A';
        pinNumber = std::stoi(portAndPin.substr(3));
        pin = 1 << pinNumber;
    } else {
        printf("Invalid port and pin definition\n");
        return;
    }
    pinName = static_cast<PinName>((portIndex << 4) | pinNumber);

    // Get ADC instance
    ADC_TypeDef* adc_instance = (ADC_TypeDef *)pinmap_peripheral(pinName, PinMap_ADC);
    ptr_adc_handle = get_shared_adc_handle(adc_instance);

    if (ptr_adc_handle == nullptr) 
    {
        printf("Error assigning ADC handle\n");
    }

    // handle enough of the GPIO pin to derive details without initialising it yet. 
    function = pinmap_function(pinName, PinMap_ADC);
    channel = STM_PIN_CHANNEL(function);
    differential = STM_PIN_INVERTED(function);

    // Ensure we don't double up on shared instance setup
    if (ptr_adc_handle->Instance == 0)
    {
        // Configure ADC settings
        ptr_adc_handle->Instance = adc_instance;
        ptr_adc_handle->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
        ptr_adc_handle->Init.Resolution = ADC_RESOLUTION_12B;
        ptr_adc_handle->Init.ScanConvMode = DISABLE;
        ptr_adc_handle->Init.ContinuousConvMode = DISABLE;
        ptr_adc_handle->Init.DiscontinuousConvMode = DISABLE;
        ptr_adc_handle->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
        ptr_adc_handle->Init.ExternalTrigConv = ADC_SOFTWARE_START;
        ptr_adc_handle->Init.DataAlign = ADC_DATAALIGN_RIGHT;
        ptr_adc_handle->Init.NbrOfConversion = 1;
        ptr_adc_handle->Init.DMAContinuousRequests = DISABLE;
        ptr_adc_handle->Init.EOCSelection = ADC_EOC_SINGLE_CONV;    

        if (HAL_ADC_Init(ptr_adc_handle) != HAL_OK) {
            printf("HAL_ADC_Init failed\n");
        }
    }

    // start the ADC clock
    enableADCClock(ptr_adc_handle->Instance);

    // initialise the GPIO
    analogInPin = new Pin(portAndPin, STM_PIN_FUNCTION(function));

    // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time 
    sConfig.Channel = getADCChannelConstant(channel);
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

    read(); // do an initial read to finalise setup. 
}

uint32_t AnalogIn::read()
{
    if (HAL_ADC_ConfigChannel(ptr_adc_handle, &sConfig) != HAL_OK) // this really does need to be called every time. Perhaps if we swapped the ADC out for DMA transfers...
    {
        printf("HAL_ADC_ConfigChannel failed\n");
    }    

    if (HAL_ADC_Start(ptr_adc_handle) != HAL_OK) {
        printf("HAL_ADC_Start failed\n");
    }

    uint16_t adcValue = 0;

    if (HAL_ADC_PollForConversion(ptr_adc_handle, 10) == HAL_OK) {
        //adcValue = static_cast<uint16_t>(HAL_ADC_GetValue(ptr_adc_handle));
        adcValue = HAL_ADC_GetValue(ptr_adc_handle);
    }

    return adcValue;
}

// --- Helper functions ---

void enableADCClock(ADC_TypeDef* instance)
{
    if (instance == ADC1) { 
        __HAL_RCC_ADC1_CLK_ENABLE();
    }    
    else if (instance == ADC2) {
        __HAL_RCC_ADC2_CLK_ENABLE();
    } 
    else if (instance == ADC3) {
        __HAL_RCC_ADC3_CLK_ENABLE();
    } 
    else {
        printf("Unknown ADC instance\n");
    }
}

uint32_t getADCChannelConstant(int channel)
{
    static const uint32_t channelTable[] = {
        ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3,
        ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7,
        ADC_CHANNEL_8, ADC_CHANNEL_9, ADC_CHANNEL_10, ADC_CHANNEL_11,
        ADC_CHANNEL_12, ADC_CHANNEL_13, ADC_CHANNEL_14, ADC_CHANNEL_15,
        ADC_CHANNEL_16, ADC_CHANNEL_17, ADC_CHANNEL_18
    };

    if (channel < 0 || channel >= static_cast<int>(sizeof(channelTable) / sizeof(channelTable[0]))) {
        printf("Invalid ADC channel %d\n", channel);
        return 0;
    }

    return channelTable[channel];
}

