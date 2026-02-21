#ifndef STEPGEN_H
#define STEPGEN_H

#include <cstdint>

#include "../../remora.h"
#include "../../modules/module.h"
#include "../../../remora-hal/pin/pin.h"

/**
 * @class Stepgen
 * @brief Stepper motor control module for handling pulse generation.
 *
 * The Stepgen class is responsible for controlling a stepper motor by generating
 * pulses based on frequency commands received from a controller, as well as managing
 * the motor's direction and enable states.
 */
class Stepgen : public Module
{
private:

	int jointNumber;               			/**< LinuxCNC joint number */
	const char* enable;            			/**< Pin for enabling the stepper motor */
	const char* step;              			/**< Pin for generating step pulses */
	const char* direction;         			/**< Pin for setting direction */
	int32_t stepBit;               			/**< Position in the DDS accumulator that triggers a step pulse */

	volatile int32_t* ptrFrequencyCommand; 	/**< Pointer to the frequency command data */
	volatile int32_t* ptrFeedback; 			/**< Pointer for feedback data */
	volatile uint8_t* ptrJointEnable; 		/**< Pointer for joint enable data */

	Pin enablePin, stepPin, directionPin; 	/**< Pins for controlling the motor's enable, step, and direction */

	int32_t rawCount;              			/**< The current position raw count (not used yet) */
	int32_t DDSaccumulator;        			/**< The Direct Digital Synthesis (DDS) accumulator */
	float frequencyScale;          			/**< Frequency scale factor */
	int32_t frequencyCommand;      			/**< The frequency command from LinuxCNC */
	int32_t DDSaddValue;           			/**< Value added to the DDS accumulator */

	int mask;                      			/**< Mask for enabling the step generator for specific joint */

	bool isEnabled;                			/**< Flag indicating whether the step generator is enabled */
	bool isForward;                			/**< Current direction (forward or backward) */
	bool isStepping;               			/**< Flag indicating whether stepping is occurring */

	void makePulses();             			/**< Generates step pulses */
	void stopPulses();             			/**< Stops the pulse generation */

public:

	Stepgen(int32_t _threadFreq, int _jointNumber, const char* _enable, const char* _step, const char* _direction, int _stepBit, volatile int32_t &_ptrFrequencyCommand, volatile int32_t &_ptrFeedback, volatile uint8_t &_ptrJointEnable, bool _usesModulePost);
	static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);

	void update(void) override;
	void updatePost(void) override;
	void slowUpdate(void) override;
	void setEnabled(bool state);

};

#endif // STEPGEN_H
