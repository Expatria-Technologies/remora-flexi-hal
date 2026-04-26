#include "SoftPWM.h"

#include <algorithm>

#define PID_PWM_MAX 256
#define confine(value, min, max) (((value) < (min)) ? (min) : (((value) > (max)) ? (max) : (value)))

SoftPWM::SoftPWM(std::string pin) :
    pinName(pin),
    pwmMax(PID_PWM_MAX - 1),
    pwmSP(0),
    SDaccumulator(0),
    SDdirection(false)
{
    pwmPin = new Pin(pinName, OUTPUT);
}


void SoftPWM::setMaxPwm(int pwmMax)
{
    this->pwmMax = confine(pwmMax, 0, PID_PWM_MAX - 1);
}


void SoftPWM::setPwmSP(int newPwmSP)
{
    this->pwmSP = newPwmSP;
}


void SoftPWM::update()
{
    if ((this->pwmSP < 0) || this->pwmSP >= PID_PWM_MAX)
    {
        return;
    }
    else if (this->pwmSP == 0)
    {
        this->pwmPin->set(false);
        return;
    }
    else if (this->pwmSP == PID_PWM_MAX - 1)
    {
        this->pwmPin->set(true);
        return;
    }

    SDaccumulator = confine(SDaccumulator, -PID_PWM_MAX, PID_PWM_MAX << 1);

    if (this->SDdirection == false)
    {
        this->SDaccumulator += this->pwmSP;
        if (this->SDaccumulator >= (PID_PWM_MAX >> 1))
            this->SDdirection = true;
    }
    else
    {
        this->SDaccumulator -= (PID_PWM_MAX - this->pwmSP);
        if (this->SDaccumulator <= 0)
            this->SDdirection = false;
    }

    this->pwmPin->set(this->SDdirection);
}

void SoftPWM::slowUpdate()
{
    return;
}