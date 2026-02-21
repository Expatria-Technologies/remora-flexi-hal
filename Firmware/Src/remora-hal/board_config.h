#ifndef BOARDCONFIG_H
#define BOARDCONFIG_H

#include <cstdint>
#include <cstddef>

#include "remora-hal/pin/pin.h"

/*
struct StepgenConfig {
    const char* Comment;
    const int JointNumber;
    const char* StepPin;
    const char* DirectionPin;
    const char* EnablePin;
}; */

const uint32_t baseFreq = 120000;
const uint32_t servoFreq = 1000;

struct StepgenConfig {
    const char* Comment;
    const int JointNumber;
    const char* StepPin;
    const char* DirectionPin;
};

struct DigitalPinConfig {
    const char* Comment;
    const char* Pin;
    const int Modifier; // OPENDRAIN, PULLUP, PULLDOWN, PULLNONE, NONE
    const bool Invert;
    const int DataBit;
};

struct PWMPinConfig {
    const char* Comment;
    const char* Pin;

};

extern const StepgenConfig StepgenConfigs[];
extern const size_t StepgenConfigCount;
extern const DigitalPinConfig DOConfigs[];
extern const size_t DigitalOutputCount;
extern const DigitalPinConfig DIConfigs[];
extern const size_t DigitalInputCount;
extern const PWMPinConfig PWMConfigs[];
extern const size_t PWMCount;
extern const char* PRU_Reset_Pin;



#endif