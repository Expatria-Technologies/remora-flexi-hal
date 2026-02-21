#ifndef TIMERINTERRUPT_H
#define TIMERINTERRUPT_H

#include "../interrupt/interrupt.h"

class pruTimer;

class TimerInterrupt : public Interrupt
{
private:
    pruTimer* interruptOwnerPtr;

public:
    TimerInterrupt(int interruptNumber, pruTimer* ownerptr);
    void ISR_Handler(void);
};

#endif
