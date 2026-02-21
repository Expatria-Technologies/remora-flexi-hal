#include "tmc.h"
#include <cstdint>

#define TOFF_VALUE  4 // [1... 15]

std::shared_ptr<Module> TMC2209::create(const JsonObject& config, Remora* instance) {
    printf("Creating TMC2209 module\n");

    const char* comment = config["Comment"];
    printf("Comment: %s\n", comment);

    std::string RxPin = config["RX pin"];
    float RSense = config["RSense"];
    uint8_t address = config["Address"];
    uint16_t current = config["Current"];
    uint16_t microsteps = config["Microsteps"];
    uint16_t stall = config["Stall sensitivity"];
    bool stealthchop = (strcmp(config["Stealth chop"], "on") == 0);

    return std::make_shared<TMC2209>(std::move(RxPin), RSense, address, current, microsteps, stealthchop, stall, instance);
}

TMC2209::TMC2209(std::string _rxtxPin, float _Rsense, uint8_t _addr, uint16_t _mA, uint16_t _microsteps, bool _stealth, uint16_t _stall, Remora* _instance)
    : TMC{_instance, _Rsense},  // Call base class constructor
      rxtxPin(std::move(_rxtxPin)),
      addr(_addr),
      mA(_mA),
      microsteps(_microsteps),
      stealth(_stealth),
      stall(_stall),
      driver(std::make_unique<TMC2209Stepper>(rxtxPin, rxtxPin, Rsense, addr)) {}


void TMC2209::configure()
{
    printf("\nStarting the Serial thread\n");
    instance->getSerialThread()->startThread();

    auto self = shared_from_this();
    instance->getSerialThread()->registerModule(self);

    driver->begin();

    printf("Testing connection to TMC driver... ");
    uint16_t result = driver->test_connection();
    
    if (result) {
        printf("Failed!\nLikely cause: ");
        switch(result) {
            case 1: printf("Loose connection\n"); break;
            case 2: printf("No power\n"); break;
            default: printf("Unknown issue\n"); break;
        }
        printf("Fix the problem and reset the board.\n");
        instance->setStatus(makeRemoraStatus(RemoraErrorSource::TMC_DRIVER, RemoraErrorCode::TMC_DRIVER_ERROR, true));
    } else {
        printf("OK\n");
    }

    if(instance->getStatus() & 0x80)
    {
        printf("\nStopping the Serial thread\n");
        instance->getSerialThread()->stopThread();
        instance->getSerialThread()->unregisterModule(self);
        return;
    }

    // Configure driver settings
    printf("Configuring driver\n");
    driver->toff(TOFF_VALUE);
    driver->blank_time(24);
    driver->rms_current(mA);
    driver->microsteps(microsteps);
    driver->TCOOLTHRS(0xFFFFF);  // 20-bit max threshold for smart energy CoolStep
    driver->semin(5);             // CoolStep lower threshold
    driver->semax(2);             // CoolStep upper threshold
    driver->sedn(0b01);           // CoolStep decrement rate
    driver->en_spreadCycle(!stealth);
    driver->pwm_autoscale(true);

    if (stealth && stall) {
        // StallGuard sensitivity threshold (higher = more sensitive)
        driver->SGTHRS(stall);
    }

    driver->iholddelay(10);
    driver->TPOWERDOWN(128);  // ~2s until driver lowers to hold current

    printf("\nStopping the Serial thread\n");
    instance->getSerialThread()->stopThread();
    instance->getSerialThread()->unregisterModule(self);
}

void TMC2209::update()
{
    driver->SWSerial->tickerHandler();
}
