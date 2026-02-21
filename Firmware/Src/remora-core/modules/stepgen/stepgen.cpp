#include "stepgen.h"


std::shared_ptr<Module> Stepgen::create(const JsonObject& config, Remora* instance)
	{
	    const char* comment = config["Comment"];
	    uint32_t threadFreq = config["ThreadFreq"];

	    printf("%s\n", comment);

	    int joint = config["Joint Number"];
	    const char* enable = config["Enable Pin"];
	    const char* step = config["Step Pin"];
	    const char* dir = config["Direction Pin"];

	    // Configure pointers to data source and feedback location
	    volatile int32_t* ptrJointFreqCmd = &instance->getRxData()->jointFreqCmd[joint];
	    volatile int32_t* ptrJointFeedback = &instance->getTxData()->jointFeedback[joint];
	    volatile uint8_t* ptrJointEnable = &instance->getRxData()->jointEnable;

	    bool usesModulePost = true;		// stepgen uses the thread modulesPost vector

	    // Create the step generator and register it in the thread
	    return std::make_unique<Stepgen>(threadFreq, joint, enable, step, dir, Config::stepBit, *ptrJointFreqCmd, *ptrJointFeedback, *ptrJointEnable, usesModulePost);
	}

/**
 * @brief Constructor for the Stepgen class.
 * 
 * Initializes the step generator with the provided configuration parameters
 * and allocates the necessary pins for the step, direction, and enable signals.
 * 
 * @param _threadFreq The thread frequency used for scaling the frequency command.
 * @param _jointNumber The joint number for which the step generator is configured.
 * @param _enable The name of the pin used to enable the step generator.
 * @param _step The name of the pin used for stepping.
 * @param _direction The name of the pin used for direction.
 * @param _stepBit The number of bits used for the step value.
 * @param _ptrFrequencyCommand A reference to the frequency command data for the joint.
 * @param _ptrFeedback A reference to the feedback data for the joint.
 * @param _ptrJointEnable A reference to the joint enable data.
 */
Stepgen::Stepgen(int32_t _threadFreq, int _jointNumber, const char* _enable, const char* _step, const char* _direction, int _stepBit, volatile int32_t& _ptrFrequencyCommand, volatile int32_t& _ptrFeedback,  volatile uint8_t& _ptrJointEnable, bool _usesModulePost)
    : jointNumber(_jointNumber),
      enable(_enable),
      step(_step),
      direction(_direction),
      stepBit(_stepBit),
      ptrFrequencyCommand(&_ptrFrequencyCommand),
      ptrFeedback(&_ptrFeedback),
      ptrJointEnable(&_ptrJointEnable),
	  enablePin(_enable, OUTPUT),
      stepPin(_step, OUTPUT),
      directionPin(_direction, OUTPUT),
      rawCount(0),
      DDSaccumulator(0),
      frequencyScale(1.0f * (1 << _stepBit) / _threadFreq),  // Frequency scaling without unnecessary cast
      mask(1 << _jointNumber),  // Mask for checking the joint number
      isEnabled(false),
      isForward(false),
      isStepping(false)
{
	usesModulePost = _usesModulePost;
}

/**
 * @brief Updates the Stepgen by generating pulses.
 * 
 * This method generates pulses for stepping according to the current
 * frequency command and direction.
 */
void Stepgen::update()
{
    makePulses();  // Generate pulses for stepping and direction
}

/**
 * @brief Post-update method for the Stepgen.
 * 
 * This method stops any ongoing pulses after the update phase.
 */
void Stepgen::updatePost()
{
    stopPulses();  // Stop pulse generation after update
}

/**
 * @brief Performs a slow update (no operation in this case).
 * 
 * This is a placeholder for performing any slow or low-priority updates,
 * though it currently does nothing.
 */
void Stepgen::slowUpdate()
{
    // Currently no operation for slow update
}

/**
 * @brief Generates pulses for step and direction.
 * 
 * This method calculates the next step and updates the step and direction
 * pins accordingly. It uses the DDS (Direct Digital Synthesis) technique
 * to generate precise frequency-based stepping.
 */
void Stepgen::makePulses()
{
    isEnabled = ((*(ptrJointEnable) & mask) != 0);
    if (!isEnabled)
    {
        enablePin.set(true);  	// Disable the driver if not enabled
        return;  				// Exit early if the generator is disabled
    }

    enablePin.set(false); 		// Enable the driver

    // Get the current frequency command and scale it using the frequency scale
    frequencyCommand = *ptrFrequencyCommand;
    DDSaddValue = frequencyCommand * frequencyScale;

    // Save the current DDS accumulator value and update it
    int32_t stepNow = DDSaccumulator;
    DDSaccumulator += DDSaddValue;

    // Check for changes in the low half of the DDS accumulator
    stepNow ^= DDSaccumulator;
    stepNow &= (1L << stepBit);  // Check for the step bit

    // Determine direction based on the sign of DDSaddValue
    isForward = DDSaddValue > 0;

    // If a step is to be made, set the direction and step pins accordingly
    if (stepNow)
    {
        directionPin.set(isForward);  // Set direction pin
        stepPin.set(true);  // Set the step pin
        rawCount += (isForward ? 1 : -1);  // Update rawCount based on direction
        *ptrFeedback = rawCount;  // Update the feedback with the raw count
        isStepping = true;  // Indicate that stepping is occurring
    }
}

/**
 * @brief Stops the pulse generation.
 * 
 * This method resets the step pin to low and stops any ongoing stepping.
 */
void Stepgen::stopPulses()
{
    stepPin.set(false);  // Reset step pin to low
    isStepping = false;  // Indicate that stepping has stopped
}

/**
 * @brief Enables or disables the Stepgen.
 * 
 * @param state The desired state of the Stepgen (true to enable, false to disable).
 */
void Stepgen::setEnabled(bool state)
{
    isEnabled = state;
}
