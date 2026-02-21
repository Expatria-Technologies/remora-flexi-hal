#ifndef QEI_H
#define QEI_H

#include <string>
#include "../../remora.h"
#include "../../modules/module.h"
#include "remora-hal/hardware_qei/hardware_qei.h"

class QEI : public Module
{

	private:

        Hardware_QEI*           hardware_qei; 

		volatile float*         ptrEncoderCount; 	    // pointer to the data source

        volatile uint16_t*      ptrData; 	            // pointer to the data source
		int                     bitNumber;				// location in the data source
        int                     mask;

        bool                    hasIndex;
        int32_t                 count;
        int8_t                  indexPulse;
        int8_t                  pulseCount;

	public:

        QEI(volatile float &ptrEncoderCount, int modifier);                                                // for channel A & B
        QEI(volatile float &ptrEncoderCount, volatile uint16_t &ptrData, int bitNumber, int modifier);     // For channels A & B, and index

        static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
		virtual void update(void);
};

#endif
