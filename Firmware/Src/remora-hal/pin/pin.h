#ifndef PIN_H
#define PIN_H

#include <cstdint>
#include <string>
#include "stm32f4xx_hal.h"

#define INPUT 0x0
#define OUTPUT 0x1

#define NONE        0b000
#define OPENDRAIN   0b001
#define PULLUP      0b010
#define PULLDOWN    0b011
#define PULLNONE    0b100

class Pin {
private:
    std::string portAndPin;
    uint8_t mode;
    uint32_t gpio_mode;
    uint8_t modifier;
    uint8_t portIndex;
    uint16_t pinNumber;
    uint16_t pin;
    uint32_t pull;
    uint32_t gpio_pull;
    uint32_t gpio_speed;
    uint32_t gpio_alt;
    GPIO_TypeDef* GPIOx;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    void configurePin();
    void enableClock();
    void initialisePin();
    void initialiseGPIO();

public:
    Pin(const std::string& portAndPin, int mode);
    Pin(const std::string& portAndPin, uint32_t gpio_mode, uint32_t gpio_pull, uint32_t gpio_speed, uint32_t gpio_alt);
    Pin(const std::string& portAndPin, int mode, int modifier);
    ~Pin();
    bool get() const;
    void set(bool value);
    void setAsOutput();
    void setAsInput();
    void setPullNone();
    void setPullUp();
    void setPullDown();
};

#endif // PIN_H