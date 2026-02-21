#ifndef QEIDRIVER_H
#define QEIDRIVER_H

#include "stm32f4xx_hal.h"
#include "../../remora-core/configuration.h"
#include "../../remora-core/modules/moduleInterrupt.h"
#include "../hal_utils.h"
#include "../shared_handlers.h"

#define QEI_TIMER_INSTANCE      TIM8
#define QEI_TIM_CLK_ENABLE      __HAL_RCC_TIM8_CLK_ENABLE
#define QEI_ALT                 GPIO_AF3_TIM8
#define PULSE_DIVIDER           2       // QEI counts on every rise and fall, but to correct for actual PPR, needs to be >> 2, divide by 4. 

class Hardware_QEI
{
    private:
        TIM_HandleTypeDef*              ptrTimHandler = nullptr;

        IRQn_Type 		                irqIndex;       
    	ModuleInterrupt<Hardware_QEI>*	IndexInterrupt;

        std::string                     chAPortAndPin = "PC_6";
        std::string                     chBPortAndPin = "PC_7";
        std::string                     indexPortAndPin = "PA_8";

        Pin*                            indexPin;
        Pin*                            chAPin;
        Pin*                            chBPin;

        bool                            hasIndex;
        int                             modifier; 

        void handleIndexInterrupt(void);

    public:
        bool                            indexDetected;
        int32_t                         indexCount;

        Hardware_QEI(bool _hasIndex, int _modifier); 
        void init(void);
        uint32_t get(void);
};

#endif
