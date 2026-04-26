#ifndef SOFTPWM_H
#define SOFTPWM_H

#include <cstdint>
#include <string>

#include "../../../../remora-hal/pin/pin.h"

class SoftPWM
{
private:
    std::string pinName;
    int pwmMax;
    int pwmSP;

    int SDaccumulator;
    bool SDdirection;

    Pin* pwmPin;

public:
    SoftPWM(std::string pin);

    void setMaxPwm(int pwmMax);
    void setPwmSP(int newPwmSP);

    void update(void);
    void slowUpdate(void);
};

#endif