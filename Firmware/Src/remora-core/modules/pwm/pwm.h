#ifndef PWM_H
#define PWM_H

#include <string>
#include <memory>

#include "../../remora.h"
#include "../../modules/module.h"
#include "remora-hal/hardware_pwm/hardware_pwm.h"

#define DEFAULT_PWM_PERIOD 100 // 100us
#define PWMMAX 256

class PWM : public Module
{
	private:
		std::string pin;			        // PWM output pin
		int pwmMax;					        // maximum PWM output

		HardwarePWM *hardware_PWM;

        volatile float *ptrPwmPeriod; 	    // pointer to the data source
		volatile float *ptrPwmPulseWidth; 	// pointer to the data source

        float pwmPeriod_us;                      // Period (us)
        float pwmPulseWidth;                // Pulse width (%)

		bool variable_freq;

		void recalculate_pulsewidth(void);

	public:
		PWM(volatile float&, volatile float&, bool, int, int, std::string);
		static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);

		virtual void update(void);          // Module default interface
		virtual void slowUpdate(void);      // Module default interface

		void setPwmMax(int);
		float getPwmPeriod(void);			// getters, primarily for testing
		float getPwmPulseWidth(void);
		int getPwmPulseWidth_us(void);
};

#endif