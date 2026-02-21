#ifndef DATA_H
#define DATA_H

#include <stdint.h>  // Add this line for uint8_t type
#include "configuration.h"

#pragma pack(push, 1)
typedef union rxData_t
{
  // this allow structured access to the incoming data without having to move it
  struct
  {
    uint8_t rxBuffer[Config::dataBuffSize];
  };
  struct
  {
    int32_t header;
    volatile int32_t jointFreqCmd[Config::joints]; 	// Base thread commands ?? - basically motion
    float setPoint[Config::variables];		  // Servo thread commands ?? - temperature SP, PWM etc
    uint8_t jointEnable;
    uint16_t outputs;
    uint8_t spare0;
  };

  rxData_t() {
      header = 0;
      outputs = 0;
      jointEnable = 0;
      for (uint8_t i=0;i<Config::joints;i++) {
         jointFreqCmd[i] = 0;
      }
      for (uint8_t i=0;i<Config::variables;i++) {
         setPoint[i] = 0.0;
     }
  }
} __attribute__((aligned(32))) rxData_t;


typedef union txData_t
{
  // this allow structured access to the out going SPI data without having to move it
  struct
  {
    uint8_t txBuffer[Config::dataBuffSize];
  };
  struct
  {
    int32_t header;
    int32_t jointFeedback[Config::joints];	  // Base thread feedback ??
    float processVariable[Config::variables];		     // Servo thread feedback ??
	uint16_t inputs;
  };

  txData_t() {
      header = 0;
      inputs = 0;
      for (uint8_t i=0;i<Config::joints;i++) {
         jointFeedback[i] = 0;
      }
      for (uint8_t i=0;i<Config::variables;i++) {
         processVariable[i] = 0.0;
     }
  }
} __attribute__((aligned(32))) txData_t;


typedef struct {
    volatile rxData_t buffer[2]; // DMA RX buffers
} DMA_RxBuffer_t;


#pragma pack(pop)

// Global Data Buffers
extern volatile txData_t txData;
extern volatile rxData_t rxData;
extern volatile DMA_RxBuffer_t rxDMABuffer;	// DMA SPI double buffers

#endif
