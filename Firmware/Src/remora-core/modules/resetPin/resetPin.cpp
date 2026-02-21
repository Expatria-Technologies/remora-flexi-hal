#include "resetPin.h"
#include <cstdio>

std::shared_ptr<Module> ResetPin::create(const JsonObject& config, Remora* instance) {
	const char* comment = config["Comment"];
	printf("%s\n", comment);

	const char* pin = config["Pin"];
	printf("Make Reset Pin at pin %s\n", pin);

	return std::make_unique<ResetPin>(instance->getReset(), pin);
}

ResetPin::ResetPin(volatile bool* ptrReset, const std::string& portAndPin) :
    ptrReset(ptrReset),
    portAndPin(portAndPin),
    pin(new Pin(portAndPin, 0)) {}  // Input mode (0x0)

void ResetPin::update() {
    *ptrReset = pin->get();
}

void ResetPin::slowUpdate() {
    return;
}
