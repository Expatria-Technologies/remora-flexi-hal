#ifndef MODULEINTERRUPT_H
#define MODULEINTERRUPT_H

#include "../interrupt/interrupt.h"
#include "module.h"

// Derived class for module interrupts

template <typename DerivedModule>
class ModuleInterrupt : public Interrupt
{
private:
	DerivedModule* InterruptOwnerPtr;                                // Pointer to the owning module
	void (DerivedModule::*InterruptHandler)();                       // Member function pointer for the ISR
	
public:
    ModuleInterrupt(IRQn_Type interruptNumber, DerivedModule* ownerPtr, void (DerivedModule::*handler)())
        : InterruptOwnerPtr(ownerPtr), InterruptHandler(handler)
    {
        // Register the interrupt
        Interrupt::Register(interruptNumber, this);
    }

    void ISR_Handler(void) {
        if (this->InterruptOwnerPtr && this->InterruptHandler) {
            (this->InterruptOwnerPtr->*InterruptHandler)(); // Call the member function of the owner
        }
    }
};

#endif
