//Module config

#include "board_config.h"



//Base thread objects - Stepgens, encoders, and RC servos supported here

const StepgenConfig StepgenConfigs[] = {{"X-Axis", 0, "PA_3", "PC_2", }, //Comment, joint number, step pin, dir pin
                                    {"Y-Axis", 1, "PC_1", "PC_0", },
                                    {"Z-Axis", 2, "PB_8", "PC_15", },
                                    {"A-Axis", 3, "PD_2", "PC_12", },
                                    {"B-Axis", 4, "PB_14", "PB_15", }};
const size_t StepgenConfigCount = sizeof(StepgenConfigs)/sizeof(StepgenConfigs[0]);



//Servo thread objects - eStop, Reset Pin, Blink, Digital Pin, PWM, Temperature, Switch, QEI


const DigitalPinConfig DOConfigs[] = {{"AUX2", "PA_4", GPIO_NOPULL, false, 0}, //Comment, pin, modifier, invert, data bit
                                {"AUX3", "PA_6", GPIO_NOPULL, false, 1},
                                {"MIST", "PA_7", GPIO_NOPULL, false, 2},
                                {"AUX1", "PB_0", GPIO_NOPULL, false, 3},
                                {"SPINDLE_EN", "PB_2", GPIO_NOPULL, false, 4},
                                {"AUX0", "PB_13", GPIO_NOPULL, false, 5},
                                {"COOLANT", "PC_9", GPIO_NOPULL, false, 6},
                                //{"STEP_EN", "PA_13", GPIO_NOPULL, false, 7},
                                //{"STEP_EN_Z", "PA_14", GPIO_NOPULL, false, 8}, //disabled for now so we can use the debugger which shares these pins
                                {"SPINDLE_DIR", "PB_1", GPIO_NOPULL, true, 9}};
const size_t DigitalOutputCount = sizeof(DOConfigs) / sizeof(DOConfigs[0]);


const DigitalPinConfig DIConfigs[] = {{"X_LIMIT", "PA_5", GPIO_NOPULL, true, 0}, //Comment, pin, modifier, invert, data bit
                                {"A_LIMIT", "PB_6", GPIO_NOPULL, true, 1},
                                {"PROBE", "PB_7", GPIO_NOPULL, true, 2},
                                {"Y_LIMIT", "PB_9", GPIO_NOPULL, true, 3},
                                {"KPSTR", "PB_10", GPIO_NOPULL, true, 4},
                                {"DOOR", "PC_4", GPIO_NOPULL, true, 5},
                                {"HALT", "PB_12", GPIO_NOPULL, true, 6},
                                {"FD_HLD", "PC_8", GPIO_NOPULL, true, 7},
                                {"CYC_START", "PC_11", GPIO_NOPULL, true, 8},
                                {"Z_LIMIT", "PC_13", GPIO_NOPULL, true, 9},
                                {"B_LIMIT", "PC_14", GPIO_NOPULL, true, 10}}; 
const size_t DigitalInputCount = sizeof(DIConfigs) / sizeof(DIConfigs[0]);


const PWMPinConfig PWMConfigs[] = {{"SPINDLE_PWM", "PA_8"}};
const size_t PWMCount = sizeof(PWMConfigs) / sizeof(PWMConfigs[0]);


const QEIPinConfig QEIConfigs[] = {{"SPINDLE_ENC", GPIO_NOPULL, 11, "True"}}; //Comment, modifier, data bit (shared with inputs), enable index
const size_t QEICount = sizeof(QEIConfigs) / sizeof(QEIConfigs[0]);

const char* PRU_Reset_Pin = "PC_3";