#ifndef SIGMADELTA_H
#define SIGMADELTA_H

#include <string>
#include "../../remora.h"
#include "../../modules/module.h"
#include "../../../remora-hal/pin/pin.h"

class SigmaDelta : public Module {
private:
    std::string pin;       // SD output pin
    int SDmax;             // Maximum SD output (8-bit resolution: 0 to 255)
    int setPoint;          // SD setpoint (percentage scaled to SDmax)
    int SDaccumulator;     // Sigma-Delta accumulator
    bool SDdirection;      // Direction of Sigma-Delta accumulator update
    Pin* SDpin;            // Pin object
    volatile float* ptrSP; // Pointer to the data source

public:
    SigmaDelta(const std::string& pin, volatile float* ptrSP);
    SigmaDelta(const std::string& pin, volatile float* ptrSP, int SDmax);
    static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);

    void setMaxSD(int SDmax);
    void setSDsetpoint(int newSdSP);
    void update() override;
    void slowUpdate() override;

};

#endif // SIGMADELTA_H
