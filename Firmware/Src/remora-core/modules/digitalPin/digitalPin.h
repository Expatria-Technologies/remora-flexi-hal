#ifndef DIGITALPIN_H
#define DIGITALPIN_H

#include <cstdint>
#include <memory>
#include <string>

#include "../../remora.h"
#include "../../modules/module.h"
#include "../../../remora-hal/pin/pin.h"

/**
 * @class DigitalPin
 * @brief Digital I/O pin control module.
 * 
 * The DigitalPin class manages digital input and output operations, allowing
 * interaction with external devices via GPIO.
 */
class DigitalPin : public Module
{
private:
    volatile uint16_t* ptrData; /**< Pointer to the data source. */
    int mode;                   /**< Pin mode (input or output). */
    std::string portAndPin;     /**< Pin identifier. */
    int bitNumber;              /**< Bit position in the data source. */
    bool invert;                /**< Flag for inverting logic. */
    int modifier;               /**< Modifier (e.g., pull-up, open-drain). */
    std::unique_ptr<Pin> pin;   /**< Pointer to the GPIO pin object. */
    int mask;                   /**< Bit mask for data manipulation. */

public:
    DigitalPin(volatile uint16_t& _ptrData, int _mode, std::string _portAndPin, 
               int _bitNumber, bool _invert, int _modifier);
    static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
    void update(void) override;
    void slowUpdate(void) override;


};

#endif // DIGITALPIN_H
