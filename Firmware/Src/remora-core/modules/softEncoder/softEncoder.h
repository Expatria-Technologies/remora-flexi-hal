#ifndef SOFTENCODER_H
#define SOFTENCODER_H

#include <string>
#include "../../remora.h"
#include "../../modules/module.h"

/**
 * @class Software Encoder
 * @brief Digital I/O pin control module.
 * 
 * The DigitalPin class manages digital input and output operations, allowing
 * interaction with external devices via GPIO.
 */

class SoftEncoder : public Module
{
	private:
		volatile float *ptrEncoderCount; 	// pointer to the data source

        bool hasIndex;
        volatile uint16_t *ptrData; 	// pointer to the data source
		int bitNumber;				    // location in the data source

        std::string portAndPinChA;			    // physical pin connection
        std::string portAndPinChB;			    // physical pin connection
        std::string portAndPinIndex;			// physical pin connection

        int8_t  modifier;
        int mask;

        uint8_t state;
        int32_t count;
        int32_t indexCount;
        int8_t  indexPulse;
        int8_t  pulseCount;

		Pin* pinA;      // channel A
        Pin* pinB;      // channel B
        Pin* pinI;      // index    

	public:
        SoftEncoder(volatile float &ptrEncoderCount, std::string ChA, std::string ChB, int modifier);
        SoftEncoder(volatile float &ptrEncoderCount, volatile uint16_t &ptrData, int bitNumber, std::string _portAndPinChA, std::string _portAndPinChB, std::string _portAndPinIndex, int modifier);

        static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
		virtual void update(void);	// Module default interface
};

#endif
