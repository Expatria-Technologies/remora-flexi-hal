#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <cstdint>

class Interrupt {
protected:
    static Interrupt* ISRVectorTable[];

public:
    Interrupt() = default;
    virtual ~Interrupt() = default;

    static void Register(uint32_t interruptNumber, Interrupt* intThisPtr);
    static void InvokeHandler(uint32_t interruptNumber);
	
    virtual void ISR_Handler() = 0;
};

#endif // INTERRUPT_H
