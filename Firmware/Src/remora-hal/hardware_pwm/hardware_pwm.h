#ifndef HARDWAREPWM_H
#define HARDWAREPWM_H

#include <string>
#include <iostream>

#include "stm32f4xx_hal.h"
#include "../hal_utils.h"
#include "../shared_handlers.h"

#define DEFAULT_PWMPERIOD 200

class HardwarePWM
{
private:
        std::string pwm_pin_str;			     
        PinName pwm_pin_name;
        Pin* pwm_pin;

        TIM_HandleTypeDef* ptr_tim_handler = nullptr;
        uint32_t pwm_tim_channel_used; 

        uint32_t timer_clk_hz;
        bool inverted_pwm = false;

        void setTimerAndChannelInstance(uint32_t);
        void initialise_timers(void);
        void initialise_pwm_channels(void);
        void initialise_pwm_pin_clocks(void);

public:
        uint32_t pulsewidth_us, period_us;
        float period_percent;   
        static HardwarePWM* head;   // see comment in change_pulsewidth function
        HardwarePWM* prev;              

        HardwarePWM(uint32_t, float, std::string);
                ~HardwarePWM(void);			        
        void change_period(uint32_t);
        void change_pulsewidth(float);
};

#endif