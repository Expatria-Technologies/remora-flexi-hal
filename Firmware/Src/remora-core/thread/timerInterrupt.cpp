#include "../interrupt/interrupt.h"
#include "timerInterrupt.h"
#include "pruTimer.h"

TimerInterrupt::TimerInterrupt(int interruptNumber, pruTimer* owner)
{
    interruptOwnerPtr = owner;
    Interrupt::Register(interruptNumber, this);
}

void TimerInterrupt::ISR_Handler(void)
{
    interruptOwnerPtr->timerTick();
}
