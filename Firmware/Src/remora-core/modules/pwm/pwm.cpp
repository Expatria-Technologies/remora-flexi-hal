#include "pwm.h"

#define PID_PWM_MAX 256		// 8 bit resolution

/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON     
************************************************************************/
std::shared_ptr<Module> PWM::create(const JsonObject& config, Remora* instance)
{
    volatile float*     variable_pointers[Config::variables];

    int sp = config["SP[i]"];  // when reading this value off the rx buffer, it will return the duty cycle.  
    int pwmMax = config["PWM Max"];
    const char* pin = config["PWM Pin"];
    const char* hardware = config["Hardware PWM"];
    const char* variable = config["Variable Freq"]; // by default all PWMs are variable.
    int period_sp = config["Period SP[i]"];
    int period_us = config["Period us"];

    //printf("\n%s\n",comment);
    printf("\nCreating PWM at pin %s\n", pin);

    /*printf("SP[i]: %d\n" // Enable debug if needed
       "PWM Max: %d\n"
       "PWM Pin: %s\n"
       "Hardware PWM: %s\n"
       "Variable Freq: %s\n"
       "Period SP[i]: %d\n"
       "Period us: %d\n",
       sp, pwmMax, pin, hardware, variable, period_sp, period_us);*/
   
    // Create pointers for set point variables

variable_pointers[sp] = &instance->getRxData()->setPoint[sp];  // sp value will store the duty cycle.  
    variable_pointers[period_sp] = &instance->getRxData()->setPoint[period_sp]; // todo - if this isn't enabled what does it do, see if we can check for errors. 
    
    if (!strcmp(hardware, "False")) // Software PWM
    {
        printf("Creating Software PWM at pin %s\n", pin);
        return std::make_unique<PWM>(*variable_pointers[sp], pwmMax, pin, false);
    }

    bool variable_freq = !strcmp(variable, "True");
	return std::make_unique<PWM>(*variable_pointers[period_sp], *variable_pointers[sp], variable_freq, period_us, pwmMax, pin);
}

/***********************************************************************
                METHOD DEFINITIONS
 ************************************************************************/

// Software PWM constructor
PWM::PWM(volatile float &_ptrPwmPulseWidth, int _pwmMax, std::string _pin, bool _useSoftware):
    ptrPwmPeriod(nullptr),
    pwmMax(_pwmMax),
    pin(_pin),
    useSoftwarePWM(_useSoftware),
    variable_freq(false),
    pwmLast(-1.0f)
{
    pwmPulseWidth = _ptrPwmPulseWidth;
    ptrPwmPulseWidth = &_ptrPwmPulseWidth;
    setPwmMax(pwmMax);
    software_PWM = new SoftPWM(pin);
    software_PWM->setMaxPwm(pwmMax);
    hardware_PWM = nullptr;
}

// Hardware PWM constructor (pointer version)
PWM::PWM(volatile float *_ptrPwmPeriod, volatile float *_ptrPwmPulseWidth, bool _variable_freq, int _fixed_period_us, int _pwmMax, std::string _pin):
    ptrPwmPeriod(_ptrPwmPeriod),
    variable_freq(_variable_freq),
    ptrPwmPulseWidth(_ptrPwmPulseWidth),
    pwmMax(_pwmMax),
    pin(_pin)
{
    // set initial period and pulse width
    if (variable_freq == true)
    {
        pwmPeriod_us = *ptrPwmPeriod;
    }
    else 
    {
        pwmPeriod_us = _fixed_period_us;    
    }

    if (pwmPeriod_us < 1)
    {
        pwmPeriod_us = DEFAULT_PWM_PERIOD;
    }

    pwmPulseWidth = *ptrPwmPulseWidth;

    setPwmMax(pwmMax);
    
    hardware_PWM = new HardwarePWM(pwmPeriod_us, pwmPulseWidth, pin); 
    software_PWM = nullptr;
}

void PWM::setPwmMax(int pwmMax) 
{ 
    pwmMax = pwmMax; 
}

void PWM::update()
{
    if (useSoftwarePWM)
    {
        float val = *(ptrPwmPulseWidth);
        if (val != pwmLast)
        {
            pwmPulseWidth = val;
            pwmLast = val;

            if (pwmPulseWidth <= 0)
            {
                software_PWM->setPwmSP(0);
            }
            else if (pwmPulseWidth >= 100)
            {
                software_PWM->setPwmSP(255);  // Max valid value for SoftPWM
            }
            else
            {
                int sp = (int)(255 * (pwmPulseWidth / 100.0));
                software_PWM->setPwmSP(sp);
            }
        }
        software_PWM->update();
    }
    else if (variable_freq == true) 
    {    
        if (*(ptrPwmPeriod) != 0 && (*(ptrPwmPeriod) != pwmPeriod_us))
        {
            // PWM period has changed
            pwmPeriod_us = *(ptrPwmPeriod);
            hardware_PWM->change_period(pwmPeriod_us);
            recalculate_pulsewidth();
        }
    }

    if (*(ptrPwmPulseWidth) != pwmPulseWidth)
    {
        // PWM duty has changed
        pwmPulseWidth = *(ptrPwmPulseWidth);
        recalculate_pulsewidth();
    } 
}

void PWM::recalculate_pulsewidth(void) 
{
    // clamp the percentage in case LinuxCNC sends something other than 0-100%
    if (pwmPulseWidth <= 0) 
    {
        pwmPulseWidth = 0;
    }
    if (pwmPulseWidth > 100)
    {
        pwmPulseWidth = 100;
    }

    // Convert duty cycle % to us and update in hardware. 
    // First check if the the duty cycle has exceeded PWM Max 1-256, and substitute the value if so.
    // pwmPulseWidth and ptrPwmPulseWidth need to keep their values intact or else update method will run continuously.

    if (pwmMax > 0 && (pwmPulseWidth / 100) * PWMMAX > pwmMax)
    {
        pwmPulseWidth = ((float)pwmMax / (float)PWMMAX) * 100;
    }
    hardware_PWM->change_pulsewidth(pwmPulseWidth);
}


void PWM::slowUpdate()
{
	return;
}