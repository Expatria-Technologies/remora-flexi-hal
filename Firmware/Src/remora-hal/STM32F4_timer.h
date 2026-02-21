#ifndef STM32F4_timer_H
#define STM32F4_timer_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <memory>

#include "../remora-core/thread/pruTimer.h"

#define TIM_PSC 1

class TimerInterrupt;
class pruThread;

class STM32F4_timer : public pruTimer {
	friend class timerInterrupt;

private:
    TIM_TypeDef* timer;
    IRQn_Type irq;
    int irqPriority;

public:
    STM32F4_timer(TIM_TypeDef* _timer, IRQn_Type _irq, uint32_t _frequency, pruThread* _ownerPtr, int _irqPriority = 0);
	
	void configTimer() override;
    void startTimer() override;
    void stopTimer() override;
    void timerTick() override;
};

#endif
