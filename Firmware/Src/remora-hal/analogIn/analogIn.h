#ifndef ANALOGIN_H
#define ANALOGIN_H

#include <cstdint>
#include <string>
#include "stm32f4xx_hal.h"
#include "../pinNames.h"
#include "../PinNamesTypes.h"
#include "../peripheralPins.h"
#include "../pin/pin.h"
#include "../shared_handlers.h"

void enableADCClock(ADC_TypeDef* instance);
uint32_t getADCChannelConstant(int channel);

class AnalogIn {
private:
    std::string portAndPin;
    uint8_t portIndex;
    uint16_t pinNumber;
    uint16_t pin;
    PinName pinName;
    
    uint8_t channel;
    uint8_t differential;

    Pin* analogInPin;
    ADC_HandleTypeDef* ptr_adc_handle = nullptr;
    ADC_ChannelConfTypeDef sConfig = {0};

public:
    AnalogIn(const std::string& portAndPin);
    uint32_t read();
};


#endif //ANALOGIN_H