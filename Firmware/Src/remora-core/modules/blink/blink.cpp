#include "blink.h"


std::shared_ptr<Module> Blink::create(const JsonObject& config, Remora* instance) {
    const char* pin = config["Pin"];
    int frequency = config["Frequency"];
    uint32_t threadFreq = config["ThreadFreq"];
    
    printf("Creating Blink module on pin %s with frequency %d Hz\n", pin, frequency);
	return std::make_unique<Blink>(pin, threadFreq, frequency);
}


/**
 * @brief Constructs a Blink module.
 * 
 * Initializes the blink pin and sets up the toggle period based on the 
 * servo thread frequency and desired blink frequency.
 */
Blink::Blink(std::string _portAndPin, uint32_t _threadFreq, uint32_t _freq) :
	bState(false),
    periodCount(_threadFreq / _freq),
    blinkCount(0),
    blinkPin(std::make_unique<Pin>(_portAndPin, OUTPUT))
{
	blinkPin->set(bState);
}

void Blink::update(void)
{
	++blinkCount;
	if (blinkCount >= periodCount / 2)
	{
        bState = !bState;
        blinkPin->set(bState);
        blinkCount = 0;
	}
}

void Blink::slowUpdate(void)
{
	return;
}
