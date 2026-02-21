#include "digitalPin.h"

std::shared_ptr<Module> DigitalPin::create(const JsonObject& config, Remora* instance) {
	const char* pin = config["Pin"];
	const char* mode = config["Mode"];
	const char* invert = config["Invert"];
	const char* modifier = config["Modifier"];
	int dataBit = config["Data Bit"];

	int mod;
	bool inv;

	if (!strcmp(modifier, "Open Drain")) {
		mod = OPENDRAIN;
	} else if (!strcmp(modifier, "Pull Up")) {
		mod = PULLUP;
	} else if (!strcmp(modifier, "Pull Down")) {
		mod = PULLDOWN;
	} else if (!strcmp(modifier, "Pull None")) {
		mod = PULLNONE;
	} else {
		mod = NONE;
	}

	inv = !strcmp(invert, "True");

	volatile uint16_t* ptrData = (!strcmp(mode, "Output")) ? &instance->getRxData()->outputs : &instance->getTxData()->inputs;

	printf("Creating DigitalPin module: Mode=%s, Pin=%s\n", mode, pin);
	return std::make_unique<DigitalPin>(*ptrData, (!strcmp(mode, "Output")) ? 1 : 0, pin, dataBit, inv, mod);
}

DigitalPin::DigitalPin(volatile uint16_t& _ptrData, int _mode, std::string _portAndPin, 
                       int _bitNumber, bool _invert, int _modifier) :
    ptrData(&_ptrData),
    mode(_mode),
    portAndPin(std::move(_portAndPin)),
    bitNumber(_bitNumber),
    invert(_invert),
    modifier(_modifier),
    pin(std::make_unique<Pin>(portAndPin, mode, modifier)),
    mask(1 << bitNumber)
{
}

void DigitalPin::update()
{
    bool pinState;
    if (mode == 0) {  // Input mode
        pinState = pin->get();
        if (invert) {
            pinState = !pinState;
        }
        if (pinState) {
            *ptrData |= mask;
        } else {
            *ptrData &= ~mask;
        }
    } else {  // Output mode
        pinState = (*ptrData & mask) ? true : false;
        if (invert) {
            pinState = !pinState;
        }
        pin->set(pinState);
    }
}

void DigitalPin::slowUpdate()
{
    return;
}
