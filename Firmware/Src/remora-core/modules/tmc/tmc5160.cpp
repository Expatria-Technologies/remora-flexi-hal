#include "tmc.h"
#include <cstdint>

#define TOFF_VALUE  4 // [1... 15]
#define TMC_DEBUG   0

std::shared_ptr<Module> TMC5160::create(const JsonObject& config, Remora* instance) {
    printf("Creating TMC5160 module\n");

    const char* comment = config["Comment"];
    printf("Comment: %s\n", comment);

    std::string pinCS = config["CS pin"];
    std::string pinMOSI = config["MOSI pin"];
    std::string pinMISO = config["MISO pin"];
    std::string pinSCK = config["SCK pin"];
    float RSense = config["RSense"];
    uint8_t address = config["Address"];
    uint16_t current = config["Current"];
    uint16_t microsteps = config["Microsteps"];
    uint16_t stall = config["Stall sensitivity"];
    bool stealthchop = (strcmp(config["Stealth chop"], "on") == 0);

    return std::make_shared<TMC5160>(std::move(pinCS), std::move(pinMOSI), std::move(pinMISO), std::move(pinSCK), RSense, address, current, microsteps, stealthchop, stall, instance);
}

TMC5160::TMC5160(std::string _pinCS, std::string _pinMOSI, std::string _pinMISO, std::string _pinSCK, float _Rsense, uint8_t _addr, uint16_t _mA, uint16_t _microsteps, bool _stealth, uint16_t _stall, Remora* _instance)
    : TMC{_instance, _Rsense},  // Call base class constructor
      pinCS(std::move(_pinCS)),
	  pinMOSI(std::move(_pinMOSI)),
	  pinMISO(std::move(_pinMISO)),
	  pinSCK(std::move(_pinSCK)),
      addr(_addr),
      mA(_mA),
      microsteps(_microsteps),
      stealth(_stealth),
      stall(_stall),
      driver(std::make_unique<TMC5160Stepper>(pinCS, _Rsense, pinMOSI, pinMISO, pinSCK)) {}


void TMC5160::configure()
{
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
        printf("OK - Driver Version: %i\n\r", driver->version());
    }

    if(instance->getStatus() & 0x80)
    {
        return;
    }

    // Use library accessors for DRV_STATUS flags for better accuracy
    printf("Initial DRV_STATUS: 0x%08lX\n\r", driver->DRV_STATUS());
    if (driver->ola()) printf("  OLA (Open Load A)\n\r");
    if (driver->olb()) printf("  OLB (Open Load B)\n\r");
    if (driver->s2ga()) printf("  S2GA (Short to Gnd A)\n\r");
    if (driver->s2gb()) printf("  S2GB (Short to Gnd B)\n\r");
    
    // Note: TMC5160 DRV_STATUS has S2VSA and S2VSB, not directly in TMC2130 base class.
    // The TMC2130 base class accessors for s2ga/s2gb might map to the correct bits for 5160.
    if (driver->otpw()) printf("  OTPW (Overtemp Prewarning)\n\r");
    if (driver->ot()) printf("  OT (Overtemperature)\n\r");
    if (driver->stst()) printf("  STST (Standstill)\n\r");

    // Explicitly read and print IOIN register (raw value)
    // The TMC5160Stepper class should have public constants for register addresses
    // Assuming TMC5160Stepper::IOIN is the address (typically 0x06)
    printf("Raw IOIN read: 0x%08lX (Expected version in bits 31:24)\n\r", driver->IOIN());

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
    driver->pwm_autoscale(true);
    driver->iholddelay(10);
    driver->TPOWERDOWN(128);  // ~2s until driver lowers to hold current

#if TMC_DEBUG
    // Read back and print key configurations
    printf("--- Final Configuration Readback ---\n\r");
    printf("GCONF:      0x%08lX\n\r", driver->GCONF());
    printf("CHOPCONF:   0x%08lX\n\r", driver->CHOPCONF());
    printf("IHOLD_IRUN: 0x%08lX\n\r", driver->IHOLD_IRUN());
    printf("PWMCONF:    0x%08lX\n\r", driver->PWMCONF());
    printf("COOLCONF:   0x%08lX\n\r", driver->COOLCONF());
    printf("GLOBALSCALER: %u\n\r", driver->GLOBAL_SCALER());
    printf("GSTAT (after config): 0x%02X (Reset: %d, drv_err: %d, uv_cp: %d)\n\r",
           (uint8_t)driver->GSTAT(), driver->reset(), driver->drv_err(), driver->uv_cp());
    
    uint32_t final_drv_status = driver->DRV_STATUS();
    printf("DRV_STATUS (after config): 0x%08lX\n\r", final_drv_status);
    if (driver->s2ga()) printf("  S2GA (Short to Gnd A) active\n\r");
    if (driver->s2gb()) printf("  S2GB (Short to Gnd B) active\n\r");
    if (driver->ola())  printf("  OLA (Open Load A) active\n\r");
    if (driver->olb())  printf("  OLB (Open Load B) active\n\r");
    if (driver->otpw()) printf("  OTPW (Overtemp Prewarning) active\n\r");
    if (driver->ot())   printf("  OT (Overtemperature) active\n\r");
    if (driver->stst()) printf("  STST (Standstill) active\n\r");
#endif
}

void TMC5160::update(){}
