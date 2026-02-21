#ifndef SHARED_HANDLERS_H
#define SHARED_HANDLERS_H

#include "stm32f4xx_hal.h"

TIM_HandleTypeDef* get_shared_tim_handle(TIM_TypeDef* instance);
uint32_t get_timer_clk_freq(TIM_TypeDef* TIMx);

ADC_HandleTypeDef* get_shared_adc_handle(ADC_TypeDef* instance);

// reusables
extern TIM_ClockConfigTypeDef sClockSourceConfig;
extern TIM_MasterConfigTypeDef sMasterConfig;
extern TIM_Encoder_InitTypeDef sConfig;
extern TIM_OC_InitTypeDef sConfigOC;
extern TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig;

#endif