#include "sigmaDelta.h"
#include <algorithm>
#include <cstdio>

#define CONFINE(value, min, max) (((value) < (min)) ? (min) : (((value) > (max)) ? (max) : (value)))
#define PID_SD_MAX 256 // 8-bit resolution

std::shared_ptr<Module> SigmaDelta::create(const JsonObject& config, Remora* instance) {
    const char* comment = config["Comment"];
    printf("%s\n", comment);

    int spIndex = config["SP[i]"];
    const char* pin = config["SD Pin"];

    // Get pointer to the setpoint from the Remora instance
    volatile float* ptrSP = &instance->getRxData()->setPoint[spIndex];

    printf("Creating SigmaDelta module: Pin=%s, SP Index=%d\n", pin, spIndex);

    // Check if "SD Max" exists in the config
    if (config["SD Max"].is<int>()) {
        int SDmax = config["SD Max"];
        printf("Using SD Max=%d\n", SDmax);
        return std::make_shared<SigmaDelta>(pin, ptrSP, SDmax);
    } else {
        printf("Using default SD Max\n");
        return std::make_shared<SigmaDelta>(pin, ptrSP);
    }
}

SigmaDelta::SigmaDelta(const std::string& pin, volatile float* ptrSP) :
    pin(pin),
    SDmax(PID_SD_MAX - 1),
    setPoint(0),
    SDaccumulator(0),
    SDdirection(false),
    SDpin(new Pin(pin, OUTPUT)),
    ptrSP(ptrSP) {}

SigmaDelta::SigmaDelta(const std::string& pin, volatile float* ptrSP, int SDmax) :
    pin(pin),
    SDmax(CONFINE(SDmax, 0, PID_SD_MAX - 1)),
    setPoint(0),
    SDaccumulator(0),
    SDdirection(false),
    SDpin(new Pin(pin, OUTPUT)),
    ptrSP(ptrSP) {}

void SigmaDelta::setMaxSD(int SDmax) {
    this->SDmax = CONFINE(SDmax, 0, PID_SD_MAX - 1);
}

void SigmaDelta::setSDsetpoint(int newSdSP) {
    // Ensure the input percentage is confined within 0-100%
    newSdSP = CONFINE(newSdSP, 0, 100);
    // Scale the percentage to fit within the range of PID_SD_MAX
    this->setPoint = (newSdSP * (PID_SD_MAX - 1)) / 100;
}

void SigmaDelta::update() {
    // Read set point from the data source
    float newSP = *ptrSP;

    // Scale from 0-100% to 0-SDmax
    int scaledSP = static_cast<int>((newSP / 100.0f) * SDmax);

    // Check if the setpoint has changed
    if (scaledSP != setPoint) {
        setPoint = CONFINE(scaledSP, 0, SDmax);
    }

    // Sigma-Delta modulation logic
    if (setPoint <= 0) {
        SDpin->set(false);
        return;
    } else if (setPoint >= SDmax) {
        SDpin->set(true);
        return;
    }

    SDaccumulator = CONFINE(SDaccumulator, -SDmax, SDmax << 1);

    if (!SDdirection) {
        SDaccumulator += setPoint;
        if (SDaccumulator >= (SDmax >> 1)) {
            SDdirection = true;
        }
    } else {
        SDaccumulator -= (SDmax - setPoint);
        if (SDaccumulator <= 0) {
            SDdirection = false;
        }
    }

    SDpin->set(SDdirection);
}

void SigmaDelta::slowUpdate() {}
