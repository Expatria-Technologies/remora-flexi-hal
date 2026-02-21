#include "analogPin.h"

std::shared_ptr<Module> AnalogPin::create(const JsonObject& config, Remora* instance)
{
    const char* comment = config["Comment"];    
    printf("%s\n", comment);

    int pv = config["PV[i]"];
    volatile float* ptrProcessVariable = &instance->getTxData()->processVariable[pv];
	
    const char* pin = config["Pin"];
    printf("Creating AnalogPin module: Pin=%s\n", pin);

    return std::make_unique<AnalogPin>(*ptrProcessVariable, pin);
}

AnalogPin::AnalogPin(volatile float &ptrFeedback, std::string _portAndPin) :
    ptrFeedback(&ptrFeedback),
    portAndPin(std::move(_portAndPin))
{
    this->adc = new AnalogIn(this->portAndPin);
}

void AnalogPin::update()
{
	this->analogRead = this->adc->read();
    *(this->ptrFeedback) = this->analogRead;
}

void AnalogPin::slowUpdate()
{
    return;
}
