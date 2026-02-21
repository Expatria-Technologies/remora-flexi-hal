

#include <stdio.h>
#include "STM32F4_timer.h"
#include "../remora-core/thread/timerInterrupt.h"
#include "../remora-core/thread/pruThread.h"

STM32F4_timer::STM32F4_timer(TIM_TypeDef* _timer, IRQn_Type _irq, uint32_t _frequency, pruThread* _ownerPtr, int _irqPriority)
    : timer(_timer), irq(_irq), irqPriority(_irqPriority)
{
    frequency = _frequency;
    timerOwnerPtr = _ownerPtr;
    interruptPtr = std::make_unique<TimerInterrupt>(irq, this);
    timerRunning = false;
}

void STM32F4_timer::configTimer()
{
    uint32_t TIM_CLK = SystemCoreClock / 2;
	
	if (timer == TIM2)
    {
        printf("Power on Timer 2\n\r");
        __HAL_RCC_TIM2_CLK_ENABLE();
    }
    else if (timer == TIM3)
    {
        printf("Power on Timer 3\n\r");
        __HAL_RCC_TIM3_CLK_ENABLE();
    }
    else if (timer == TIM4)
    {
        printf("Power on Timer 4\n\r");
        __HAL_RCC_TIM4_CLK_ENABLE();
    }

    //Note: timer update frequency = TIM_CLK/(TIM_PSC+1)/(TIM_ARR + 1)
    timer->CR2 &= 0;                                            // UG used as trigg output
    timer->PSC = TIM_PSC-1;                                     // prescaler
    timer->ARR = ((TIM_CLK / TIM_PSC / frequency) - 1);   		// period
    timer->EGR = TIM_EGR_UG;                                    // reinit the counter
    timer->DIER = TIM_DIER_UIE;                                 // enable update interrupts

	NVIC_SetPriority(irq, irqPriority);
}

void STM32F4_timer::startTimer()
{
    timer->CR1 |= TIM_CR1_CEN;
    NVIC_EnableIRQ(irq);
    timerRunning = true;
    printf("Timer started\n");
}

void STM32F4_timer::stopTimer()
{
    NVIC_DisableIRQ(irq);
    timer->CR1 &= ~TIM_CR1_CEN;
    timerRunning = false;
    printf("Timer stopped\n");
}

void STM32F4_timer::timerTick() {
    if (timerOwnerPtr) {
        timerOwnerPtr->update();
    }
}
