#ifndef BOARDCONFIG_H
#define BOARDCONFIG_H

#include "pin.h"

/*
struct StepgenConfig {
    const char* Comment;
    const int JointNumber;
    const char* StepPin;
    const char* DirectionPin;
    const char* EnablePin;
}; */

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


//Module config

#define BOARD "Expatria Flexi-HAL"

//Base thread objects - Stepgens, encoders, and RC servos supported here

StepgenConfig StepgenConfigs[] = {{"X-Axis", 0, "PA_3", "PC_2", }, //Comment, joint number, step pin, dir pin, enable pin
                                    {"Y-Axis", 1, "PC_1", "PC_0", },
                                    {"Z-Axis", 2, "PB_8", "PC_15", },
                                    {"A-Axis", 3, "PD_2", "PC_12", },
                                    {"B-Axis", 4, "PB_14", "PB_15", }};


//Servo thread objects - eStop, Reset Pin, Blink, Digital Pin, PWM, Temperature, Switch, QEI

DigitalPinConfig DOConfigs[] = {{"AUX2", "PA_4", PULLNONE, false, 0}, //Comment, pin, modifier, invert, data bit
                                {"AUX3", "PA_6", PULLNONE, false, 1},
                                {"MIST", "PA_7", PULLNONE, false, 2},
                                {"AUX1", "PB_0", PULLNONE, false, 3},
                                {"SPINDLE_EN", "PB_2", PULLNONE, false, 4},
                                {"AUX0", "PB_13", PULLNONE, false, 5},
                                {"COOLANT", "PC_9", PULLNONE, false, 6},
                                {"STEP_EN", "PA_13", PULLNONE, false, 7},
                                {"STEP_EN_Z", "PA_14", PULLNONE, false, 8},
                                {"SPINDLE_DIR", "PB_1", PULLNONE, true, 9}};

DigitalPinConfig DIConfigs[] = {{"X_LIMIT", "PA_5", PULLNONE, true, 0}, //Comment, pin, modifier, invert, data bit
                                {"A_LIMIT", "PB_6", PULLNONE, true, 1},
                                {"PROBE", "PB_7", PULLNONE, true, 2},
                                {"Y_LIMIT", "PB_9", PULLNONE, true, 3},
                                {"KPSTR", "PB_10", PULLNONE, true, 4},
                                {"DOOR", "PC_4", PULLNONE, true, 5},
                                {"HALT", "PB_12", PULLNONE, true, 6},
                                {"FD_HLD", "PC_8", PULLNONE, true, 7},
                                {"CYC_START", "PC_11", PULLNONE, true, 8},
                                {"Z_LIMIT", "PC_13", PULLNONE, true, 9},
                                {"B_LIMIT", "PC_14", PULLNONE, true, 10},
                                {"ENC_A", "PA_0", PULLNONE, true, 11},
                                {"ENC_B", "PA_1", PULLNONE, true, 12},
                                {"ENC_Z", "PA_2", PULLNONE, true, 13}}; 

PWMPinConfig PWMConfigs[] = {{"Spindle PWM", "PA_8"}};

const char* PRU_Reset_Pin = "PC_3";


 

//On Load objects - MCP4451, Motor Power, TMC2208, TMC2209. None currently implemented here.



#endif