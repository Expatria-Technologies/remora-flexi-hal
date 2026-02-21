#include <cstdio>
#include "interrupt.h"
#include "../../hardware.h"

Interrupt* Interrupt::ISRVectorTable[PERIPH_COUNT_IRQn] = {nullptr};

// Register an interrupt with a specific IRQ number
void Interrupt::Register(uint32_t interruptNumber, Interrupt* intThisPtr) {
    if (interruptNumber < PERIPH_COUNT_IRQn) {
        printf("Registering interrupt for IRQ %ld\n", interruptNumber);
        ISRVectorTable[interruptNumber] = intThisPtr;
    }
}

// Generic IRQ dispatcher
void Interrupt::InvokeHandler(uint32_t interruptNumber) {
    if (interruptNumber < PERIPH_COUNT_IRQn && ISRVectorTable[interruptNumber]) {
        ISRVectorTable[interruptNumber]->ISR_Handler();
    }
}
