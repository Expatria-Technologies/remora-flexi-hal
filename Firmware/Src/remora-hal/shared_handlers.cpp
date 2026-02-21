#include "shared_handlers.h"

// STM32 HAL Timer handles make heavy use of locks which doesn't gel well with storing these handlers in the class like we do elsewhere
// updating a timer and triggering the lock could lead to clashes and other things that could crash the PRU
// This class will allocate the correct handler and initialise them first time, keeping them set up for future uses. 
TIM_HandleTypeDef htim1;        // note ch1 and ch2 are being used by QEI
//TIM_HandleTypeDef htim2; // disabled due to being used by Remora threads
//TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
//TIM_HandleTypeDef htim6; // not used for PWM on this target
//TIM_HandleTypeDef htim7;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim9;
TIM_HandleTypeDef htim10;
TIM_HandleTypeDef htim11;
TIM_HandleTypeDef htim12;
TIM_HandleTypeDef htim13;
TIM_HandleTypeDef htim14;

TIM_ClockConfigTypeDef sClockSourceConfig = {0};
TIM_MasterConfigTypeDef sMasterConfig = {0};
TIM_Encoder_InitTypeDef sConfig  = {0};
TIM_OC_InitTypeDef sConfigOC = {0};
TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

ADC_HandleTypeDef hadc1, hadc2, hadc3;

TIM_HandleTypeDef* get_shared_tim_handle(TIM_TypeDef* instance) 
{
    if (instance == TIM1)  
        return &htim1;
    if (instance == TIM4)  
        return &htim4;
    if (instance == TIM5)  
        return &htim5;
    if (instance == TIM8)  
        return &htim8;
    if (instance == TIM9)  
        return &htim9;
    if (instance == TIM10) 
        return &htim10;
    if (instance == TIM11) 
        return &htim11;
    if (instance == TIM12) 
        return &htim12;
    if (instance == TIM13) 
        return &htim13;
    if (instance == TIM14) 
        return &htim14;

    return nullptr; // catch all
}

uint32_t get_timer_clk_freq(TIM_TypeDef* TIMx)  // probably should move to hal_utils. 
{
    uint32_t pclk, multiplier;

    if (TIMx == TIM1 || TIMx == TIM8 || TIMx == TIM9 || TIMx == TIM10 || TIMx == TIM11) 
    {       
        // On APB2
        pclk = HAL_RCC_GetPCLK2Freq();
        uint32_t ppre2 = ((RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos);
        multiplier = (ppre2 < 4) ? 1 : 2;
    }
    else    
    {
        // On APB1
        pclk = HAL_RCC_GetPCLK1Freq();
        uint32_t ppre1 = ((RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos);
        multiplier = (ppre1 < 4) ? 1 : 2;       
    }
    return pclk * multiplier;
}

ADC_HandleTypeDef* get_shared_adc_handle(ADC_TypeDef* instance) 
{
    if (instance == ADC1) 
        return &hadc1;
    if (instance == ADC2) 
        return &hadc2;
    if (instance == ADC3)
        return &hadc3;
    return nullptr; // catch all
}