#ifndef BLINK_H
#define BLINK_H

#include <cstdint>
#include <string>

#include "../../remora.h"
#include "../../modules/module.h"
#include "../../../remora-hal/pin/pin.h"

/**
 * @class Blink
 * @brief A module for toggling a pin at a specific frequency.
 * 
 * The Blink class controls a GPIO pin, toggling its state at a specified frequency.
 */
class Blink : public Module
{
private:

	bool 					bState;
	uint32_t 				periodCount;
	uint32_t 				blinkCount;

	std::unique_ptr<Pin> 	blinkPin;

public:

	Blink(std::string _portAndPin, uint32_t _threadFreq, uint32_t _freq);
	static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);

	virtual void update(void);
	virtual void slowUpdate(void);
};

#endif

