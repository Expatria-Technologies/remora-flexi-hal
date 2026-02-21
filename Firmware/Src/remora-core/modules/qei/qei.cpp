#include "qei.h"

/***********************************************************************
                MODULE CONFIGURATION AND CREATION FROM JSON     
************************************************************************/
std::shared_ptr<Module> QEI::create(const JsonObject& config, Remora* instance) 
{
    const char* comment = config["Comment"];
    printf("%s\n",comment);

    const char* modifier = config["Modifier"];
    int pv = config["PV[i]"];
    int dataBit = config["Data Bit"];
    const char* index = config["Enable Index"];

    printf("Creating QEI, hardware quadrature encoder interface\n");

    int mod;

	if (!strcmp(modifier, "Open Drain")) {          // the documentation offers open drain but have noticed noticed it isn't implemented yet in hardware, simulating for now with a pullup.
		mod = GPIO_PULLUP;
	} else if (!strcmp(modifier, "Pull Up")) {
		mod = GPIO_PULLUP;
	} else if (!strcmp(modifier, "Pull Down")) {
		mod = GPIO_PULLDOWN;
	} else if (!strcmp(modifier, "Pull None")) {
		mod = GPIO_NOPULL;
	} else {
		mod = GPIO_NOPULL;
	}           

    volatile float* ptrProcessVariable = &instance->getTxData()->processVariable[pv];
	volatile uint16_t* ptrInputs = &instance->getTxData()->inputs;

    if (!strcmp(index,"True"))
    {
        printf("  Encoder has index\n");
        return std::make_unique<QEI>(*ptrProcessVariable, *ptrInputs, dataBit, mod);
    }
    else
    {
        return std::make_unique<QEI>(*ptrProcessVariable, mod);
    }
}

/***********************************************************************
*                METHOD DEFINITIONS                                    *
************************************************************************/

QEI::QEI(volatile float &ptrEncoderCount, int modifier) :
	ptrEncoderCount(&ptrEncoderCount)
{
    hasIndex = false;
    hardware_qei = new Hardware_QEI(hasIndex, modifier);     
}

QEI::QEI(volatile float &ptrEncoderCount, volatile uint16_t &ptrData, int bitNumber, int modifier) :
	ptrEncoderCount(&ptrEncoderCount),
    ptrData(&ptrData),
    bitNumber(bitNumber)
{
    hasIndex = true;
    indexPulse = 100;                             
	count = 0;								    
    pulseCount = 0;                               
    mask = 1 << bitNumber;

    hardware_qei = new Hardware_QEI(hasIndex, modifier);     
}

void QEI::update()
{
    count = hardware_qei->get();

    if (hasIndex)                                     // we have an index pin
    {
        // handle index, index pulse and pulse count
        if (hardware_qei->indexDetected && (pulseCount == 0))    // index interrupt occured: rising edge on index pulse
        {
            *(ptrEncoderCount) = hardware_qei->indexCount;
            pulseCount = indexPulse;        
            *(ptrData) |= mask;                 // set bit in data source high
        }
        else if (pulseCount > 0)                      // maintain both index output and encoder count for the latch period
        {
            hardware_qei->indexDetected = false;
            pulseCount--;                             // decrement the counter
        }
        else
        {
            *(ptrData) &= ~mask;                // set bit in data source low
            *(ptrEncoderCount) = count;         // update encoder count
        }
    }
    else
    {
        *(ptrEncoderCount) = count;             // update encoder count
    }
}
