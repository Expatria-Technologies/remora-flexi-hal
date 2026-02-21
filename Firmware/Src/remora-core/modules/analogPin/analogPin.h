#ifndef ANALOGPIN_H
#define ANALOGPIN_H

#include <string>

#include "../../remora.h"
#include "../../modules/module.h"
#include "../../../remora-hal/analogIn/analogIn.h"

/**
 * @class AnalogPin
 * @brief Analog input read module.
 * 
 * The AnalogPin class manages creation and reading of analog inputs
 */
class AnalogPin : public Module
{
  private:
    volatile float* ptrFeedback;       	   // pointer where to put the feedback
    float analogRead;

    std::string portAndPin;	             // physical pins connections
    AnalogIn *adc;

  public:
    AnalogPin(volatile float&, std::string); 
    static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
    
    virtual void update(void);
    virtual void slowUpdate(void);
};


#endif // ANALOGPIN_H
