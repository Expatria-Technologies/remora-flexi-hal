
#ifndef REMORA_H
#define REMORA_H

#define JOINTS 5    // Number of joints - set this the same as Remora firmware code. Max 8 joints
#define VARIABLES 1 // Number of command values - set this the same Remora firmware code. Setpoints and Process Varaibles use this.
#define DIGITAL_OUTPUTS 10
#define DIGITAL_INPUTS 12



const char INPUT_NAMES[DIGITAL_INPUTS][14] = {"X_LIMIT", "A_LIMIT", "PROBE", "Y_LIMIT", "KPSTR", "DOOR", "HALT", "FEED_HOLD", "CYCLE_START", "Z_LIMIT", "B_LIMIT", "SPINDLE_INDEX"};
const char OUTPUT_NAMES[DIGITAL_OUTPUTS][13] = {"AUX2", "AUX3", "MIST", "AUX1", "SPINDLE_EN", "AUX0", "COOLANT", "STEP_EN", "STEP_EN_Z", "SPINDLE_DIR"};
const char SETPOINT_NAMES[VARIABLES][12] = {"SPINDLE_PWM"};
const char PV_NAMES[VARIABLES][12] = {"SPINDLE_ENC"};

#define SPIBUFSIZE 64 //(4+4*JOINTS+4*COMMANDS+1) //(MAX_MSG*4) //20  SPI buffer size ......FIFO buffer size is 64 bytes?

#define PRU_DATA 0x64617461  // "data" SPI payload
#define PRU_READ 0x72656164  // "read" SPI payload
#define PRU_WRITE 0x77726974 // "writ" SPI payload
#define PRU_ESTOP 0x65737470 // "estp" SPI payload

#define STEPBIT 22 // bit location in DDS accum
#define STEP_MASK (1L << STEPBIT)
#define STEP_OFFSET (1L << (STEPBIT - 1))

#define PRU_BASEFREQ 140000 // Base freq of the PRU stepgen in Hz

#endif
