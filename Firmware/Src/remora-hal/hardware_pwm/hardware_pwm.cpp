#include "hardware_pwm.h"

HardwarePWM* HardwarePWM::head = nullptr;

HardwarePWM::HardwarePWM(uint32_t _initial_period_us, float _initial_pulsewidth_percent, std::string _pwm_pin_str):
	pwm_pin_str(_pwm_pin_str)
{
    this->prev = HardwarePWM::head; // see comment in change duty cycle function. 
    HardwarePWM::head = this;

    pwm_pin_name = portAndPinToPinName(pwm_pin_str.c_str());   
    uint32_t pwm_function = pinmap_function(pwm_pin_name, PinMap_PWM);

    if (pwm_function != (uint32_t)NC)
    {
        setTimerAndChannelInstance(pwm_function); // sets pwm_tim_handler.Instance and the channel from the pinmap

        if (ptr_tim_handler->Instance == TIM2 ||     // TODO - refactor once the configured timers are visible in the configuration.h file
            ptr_tim_handler->Instance == TIM3) 
        {
            printf("Error: Timer instance clashes with one of the timers used for Remora BASE or SERVO threads, this may crash the PRU");
        }

        // Initialise the pin clocks, timer, channel and pin
        initialise_pwm_pin_clocks();
        initialise_timers();
        initialise_pwm_channels();  
        pwm_pin = createPinFromPinMap(pwm_pin_str, pwm_pin_name, PinMap_PWM, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW);  // reenable after test

        // check that initialisation worked.
        HAL_StatusTypeDef PWM_Start_result; 

        // start the PWM or use the alternate start function for TIMx_CHyN pins
        if (!inverted_pwm)
            PWM_Start_result = HAL_TIM_PWM_Start(ptr_tim_handler, pwm_tim_channel_used); 
        else
            PWM_Start_result = HAL_TIMEx_PWMN_Start(ptr_tim_handler, pwm_tim_channel_used);         

        if (PWM_Start_result != HAL_OK)
        {
            Error_Handler();
        }  

        // set up default values. 
        change_period(_initial_period_us);
        change_pulsewidth(_initial_pulsewidth_percent);

        //printf("Timer clk frequency: %lu Hz\n", this->timer_clk_hz);  // debug if needed
        //printf("Prescaler: %lu\n", this->pwm_tim_handler.Init.Prescaler);
    }
    else
    {
        printf("Error, could not set up pin %s as PWM. Please check the available list of PWM pins on your hardware target and try again\n", pwm_pin_str.c_str());
    }    
}

void HardwarePWM::setTimerAndChannelInstance(uint32_t pwm_pinmap_function)
{
    TIM_TypeDef* timx_instance = ((TIM_TypeDef *)getPWMName(pwm_pin_name));
    // Use the shared global handle
    ptr_tim_handler = get_shared_tim_handle(timx_instance);

    if (ptr_tim_handler == nullptr) 
    {
        printf("Error: Unsupported timer instance\n");
        Error_Handler();
    }

    // Cast the peripheral pin style timer to STM style timer
    ptr_tim_handler->Instance = timx_instance;

    // reverse the stored encoding of the function to derive the channel used
    uint32_t pin_function_timer_channel = ((pwm_pinmap_function) >> STM_PIN_CHAN_SHIFT) & STM_PIN_CHAN_MASK;

    switch (pin_function_timer_channel) 
    {
        case 1: 
            pwm_tim_channel_used = TIM_CHANNEL_1;   
            break;        
        case 2: 
            pwm_tim_channel_used = TIM_CHANNEL_2;   
            break;
        case 3: 
            pwm_tim_channel_used = TIM_CHANNEL_3;   
            break;
        case 4: 
            pwm_tim_channel_used = TIM_CHANNEL_4;   
            break;
    }

    // check for Inverted output, e.g TIMx_CHyN , in which case a different start function is used to init the PWM. 
    inverted_pwm = ((((pwm_pinmap_function) >> STM_PIN_INV_SHIFT) & STM_PIN_INV_MASK) == 1) ? true : false;
}

void HardwarePWM::initialise_timers(void) 
{
    // note that pwm_tim_handler.Instance and pwm_tim_channel_used are set prior to calling this in constructor.  
    timer_clk_hz = get_timer_clk_freq(ptr_tim_handler->Instance); 
    ptr_tim_handler->Init.Prescaler = (this->timer_clk_hz / 1000000) - 1;
    ptr_tim_handler->Init.CounterMode = TIM_COUNTERMODE_UP;
    ptr_tim_handler->Init.Period = 0xffff; 
    ptr_tim_handler->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    ptr_tim_handler->Init.RepetitionCounter = 0;
    ptr_tim_handler->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(ptr_tim_handler) != HAL_OK)
    {
        Error_Handler(); 
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    
    if (HAL_TIM_ConfigClockSource(ptr_tim_handler, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    
    if (HAL_TIM_PWM_Init(ptr_tim_handler) != HAL_OK)
    {
        Error_Handler();
    }
    
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    
    if (HAL_TIMEx_MasterConfigSynchronization(ptr_tim_handler, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void HardwarePWM::initialise_pwm_channels(void) 
{
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = ptr_tim_handler->Init.Period * 0.5; // gives this an inital value for sanity checking, will be overwritten on startup. 
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    
    if (HAL_TIM_PWM_ConfigChannel(ptr_tim_handler, &sConfigOC, pwm_tim_channel_used) != HAL_OK)
    {
        Error_Handler();
    }

    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;  // We may need to adjust this for safety when using PWM to drive higher current loads. Obvious use case is spindle PWM, but if ever used for anything else may need to be revisited
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    
    if (HAL_TIMEx_ConfigBreakDeadTime(ptr_tim_handler, &sBreakDeadTimeConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

void HardwarePWM::initialise_pwm_pin_clocks(void) 
{
    if (ptr_tim_handler->Instance == TIM1) 
        __HAL_RCC_TIM1_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM2) 
        __HAL_RCC_TIM2_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM3) 
        __HAL_RCC_TIM3_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM4) 
        __HAL_RCC_TIM4_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM5) 
        __HAL_RCC_TIM5_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM6) 
        __HAL_RCC_TIM6_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM7) 
        __HAL_RCC_TIM7_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM8) 
        __HAL_RCC_TIM8_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM9) 
        __HAL_RCC_TIM9_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM10) 
        __HAL_RCC_TIM10_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM11) 
        __HAL_RCC_TIM11_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM12) 
        __HAL_RCC_TIM12_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM13) 
        __HAL_RCC_TIM13_CLK_ENABLE();
    else if (ptr_tim_handler->Instance == TIM14) 
        __HAL_RCC_TIM14_CLK_ENABLE();
    else 
        printf("incorrect timer selected, please refer to documentation.\n");        
}

HardwarePWM::~HardwarePWM(void) 
{
    if (ptr_tim_handler->Instance == TIM1)  
        __HAL_RCC_TIM1_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM2)  
        __HAL_RCC_TIM2_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM3)  
        __HAL_RCC_TIM3_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM4)  
        __HAL_RCC_TIM4_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM5)  
        __HAL_RCC_TIM5_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM6)  
        __HAL_RCC_TIM6_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM7)  
        __HAL_RCC_TIM7_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM8)  
        __HAL_RCC_TIM8_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM9)  
        __HAL_RCC_TIM9_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM10) 
        __HAL_RCC_TIM10_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM11) 
        __HAL_RCC_TIM11_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM12) 
        __HAL_RCC_TIM12_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM13) 
        __HAL_RCC_TIM13_CLK_DISABLE();
    if (ptr_tim_handler->Instance == TIM14) 
        __HAL_RCC_TIM14_CLK_DISABLE();       
}

// we have an interesting technical challenge to get around in that any channel used on a TIMx shares a common period. 
// For any variable frequency pwm pins used, changing it's period will effect other pins on other channels that share that timer (see readme).
// This happens at the HAL layer so no issue there, but when you change the period we also need to recalculate the % duty cycle or your PWMs will start to act eratically.
// to get around this, we use a linked list so that the update function call can iteratively run through updating each duty cycle on the relevant timer to keep them in sync

void HardwarePWM::change_period(uint32_t new_period_us)
{
    // update all period_us of all using the same timer. 
    for (HardwarePWM* node = head; node != nullptr; node = node->prev) 
    {
        if (node->ptr_tim_handler && node->ptr_tim_handler->Instance == this->ptr_tim_handler->Instance) // only want to update handles sharing the same TIMx
        {
            node->period_us = new_period_us;
        }
    }

    // being that we have shared instances, not everything needs to be updated
    uint32_t timer_freq_after_prescaler = timer_clk_hz / (ptr_tim_handler->Init.Prescaler + 1);
    uint32_t period_ticks = (timer_freq_after_prescaler * period_us) / 1000000;

    if (period_ticks < 1) 
    {
        period_ticks = 1;
    }

    ptr_tim_handler->Init.Period = period_ticks - 1;

    // attempt to restart PWM with minimal interruption, inclduing recalculation of pulse width
    __HAL_TIM_DISABLE(ptr_tim_handler);
    __HAL_TIM_SET_AUTORELOAD(ptr_tim_handler, period_ticks - 1);

    // Ensure pulse width is not out of bounds of new period value, otherwise the counter will never reach it. 
    if (__HAL_TIM_GET_COMPARE(ptr_tim_handler, pwm_tim_channel_used) > (period_ticks - 1)) {
        __HAL_TIM_SET_COMPARE(ptr_tim_handler, pwm_tim_channel_used, period_ticks - 1);
    }

    ptr_tim_handler->Instance->EGR |= TIM_EGR_UG; // trigger reload.

    // re-enable
    __HAL_TIM_ENABLE(ptr_tim_handler); 
}

void HardwarePWM::change_pulsewidth(float new_pulsewidth_percent)
{
    period_percent = new_pulsewidth_percent;

    for (HardwarePWM* node = head; node != nullptr; node = node->prev) 
    {
        if (node->ptr_tim_handler && node->ptr_tim_handler->Instance == this->ptr_tim_handler->Instance) // only want to update handles sharing the same TIMx
        {
            node->pulsewidth_us = (node->period_us * node->period_percent) / 100.0; //convert % to us. 
            uint32_t timer_freq_after_prescaler = node->timer_clk_hz / (node->ptr_tim_handler->Init.Prescaler + 1); // convert again, but take into account clock timing
            uint32_t pulse_ticks = (timer_freq_after_prescaler * node->pulsewidth_us) / 1000000;    

            // clamp it
            if (pulse_ticks < 1) 
            {
                pulse_ticks = 0;
            }
            if (pulse_ticks > node->ptr_tim_handler->Init.Period)    
            {
                pulse_ticks = node->ptr_tim_handler->Init.Period;
            }
            __HAL_TIM_SET_COMPARE(node->ptr_tim_handler, node->pwm_tim_channel_used, pulse_ticks); // to try, this probably only needs to be run once?
        }
    }
}