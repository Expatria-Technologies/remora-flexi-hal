/*
    Remora is a free, opensource LinuxCNC component and Programmable Realtime
	Unit (PRU) firmware to implement a LinuxCNC based CNC controller.
	Copyright (C) 2025 Scott Alford (scotta)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "remora.h"
#include "../irqHandlers.h"
#include "interrupt/interrupt.h"

#ifndef STATIC_CONFIG
    #include "config/jsonConfigHandler.h"
#else
    #include "config/staticConfigHandler.h"
    #include "remora-hal/board_config.h"
#endif

// unions for TX and RX data
volatile txData_t txData;
volatile rxData_t rxData;

Remora::Remora(std::shared_ptr<CommsHandler> commsHandler,
               std::unique_ptr<pruTimer> baseTimer,
               std::unique_ptr<pruTimer> servoTimer,
               std::unique_ptr<pruTimer> serialTimer)
	: currentState(ST_SETUP),
	  prevState(ST_SETUP),
	  ptrTxData(&txData),
	  ptrRxData(&rxData),
	  reset(false),
	  configHandler(nullptr),
	  comms(std::move(commsHandler)),
	  baseThread(nullptr),
	  servoThread(nullptr),
	  serialThread(nullptr),
	  onLoad(),
	  baseFreq(baseTimer->getFrequency()),
	  servoFreq(servoTimer->getFrequency()),
	  serialFreq(serialTimer ? serialTimer->getFrequency() : 0),
	  threadsRunning(false)
{
	#ifndef STATIC_CONFIG //TODO - Static config
        configHandler = std::make_unique<JsonConfigHandler>(this);
    #else
        configHandler = std::make_unique<StaticConfigHandler>(this); //TODO
    #endif

    updateHeader();

    comms->init();
    comms->start();

    baseThread = std::make_unique<pruThread>("BaseThread");
    baseTimer->setOwner(baseThread.get());
    baseThread->setTimer(std::move(baseTimer));


    servoThread = std::make_unique<pruThread>("ServoThread");
    servoTimer->setOwner(servoThread.get());
    servoThread->setTimer(std::move(servoTimer));

    if (serialTimer) {
        serialThread = std::make_unique<pruThread>("SerialThread");
        serialThread->setTimer(std::move(serialTimer));
        serialTimer->setOwner(serialThread.get());
    }

    servoThread->registerModule(comms);
}

void Remora::updateHeader()
{
    ptrTxData->header = Config::pruData | remoraStatus;
}

void Remora::transitionToState(State newState)
{
    if (currentState != newState) {
        printStateEntry(newState);
        prevState = currentState;
        currentState = newState;
    }
}

void Remora::printStateEntry(State state)
{
    const char* stateNames[] = {
        "Setup", "Start", "Idle", "Running", "Stop", "Reset", "System Reset"
    };
    printf("\n## Transitioning to %s state\n", stateNames[state]);
}

void Remora::handleSetupState()
{
    #ifndef STATIC_CONFIG
    loadModules();
    #else
    loadStaticModules();
    #endif
    transitionToState(ST_START);
}

void Remora::handleStartState()
{
    for (const auto& module : onLoad) {
        if (module) {
            module->configure();
        }
    }

    if (!threadsRunning) {
        startThread(servoThread, "Servo");
        startThread(baseThread, "Base");
        threadsRunning = true;
    }

    transitionToState(ST_IDLE);
}

void Remora::handleIdleState()
{
    if (comms->getStatus()) {
        transitionToState(ST_RUNNING);
    }
}

void Remora::handleRunningState()
{
    if (!comms->getStatus()) {
        transitionToState(ST_RESET);
    }

    if (reset) {
        transitionToState(ST_SYSRESET);
    }
}

void Remora::handleResetState()
{
    printf("Resetting rxBuffer\n");
    resetBuffer(ptrRxData->rxBuffer, Config::dataBuffSize);
    transitionToState(ST_IDLE);
}

void Remora::handleSysResetState()
{
    HAL_NVIC_SystemReset();
}

void Remora::startThread(const std::unique_ptr<pruThread>& thread, const char* name)
{
    printf("\nStarting the %s thread\n", name);
    thread->startThread();
}

void Remora::resetBuffer(volatile uint8_t* buffer, size_t size)
{
    memset((void*)buffer, 0, size);
}

void Remora::run()
{
    while (true) {
        updateHeader();

        if (remoraStatus & 0x80) {
            if (!fatalErrorHandled) {
                printf("Fatal error detected. Halting Remora state machine.\n");
                fatalErrorHandled = true;
            }

            comms->tasks(); 
            continue;
        }

        switch (currentState) {
            case ST_SETUP:
                handleSetupState();
                break;
            case ST_START:
                handleStartState();
                break;
            case ST_IDLE:
                handleIdleState();
                break;
            case ST_RUNNING:
                handleRunningState();
                break;
            case ST_RESET:
                handleResetState();
                break;
            case ST_SYSRESET:
                handleSysResetState();
                break;
            default:
                printf("Error: Invalid state\n");
                break;
        }

        #ifdef ETH_CTRL
        if (JsonConfigHandler::new_flash_json)
        {
            printf("Checking new configuration file\n");
            if (configHandler->json_check_length_and_CRC() > 0)
            {
                printf("Moving new config file to Flash storage\n");
                configHandler->store_json_in_flash();

                // force a reset to load new JSON configuration
                printf("Success. Forcing reboot now...\n");
                pru_reboot();
            }
        }
        #endif

        comms->tasks();
    }
}

#ifdef STATIC_CONFIG
    void Remora::loadStaticModules()
    {
        printf("Loading static modules...");

        //std::shared_ptr<Module> _mod = factory->createModule(threadName, moduleType, modules[i], this);
        
        // Create Stepgen modules directly rather than using module factory which is based on json config. TODO, stop being lazy here.
        for (size_t i = 0; i < StepgenConfigCount; i++) {
            printf("\nCreating step generator for Joint %i, %s\n", i, StepgenConfigs[i].Comment);

            volatile int32_t* ptrJointFreqCmd = &rxData.jointFreqCmd[i];
	        volatile int32_t* ptrJointFeedback = &txData.jointFeedback[i];
	        volatile uint8_t* ptrJointEnable = &rxData.jointEnable;

            std::shared_ptr<Module> stepgen = std::make_shared<Stepgen>(
                baseFreq,
                StepgenConfigs[i].JointNumber,
                "PB_0", //enable pin, handled elsewhere for Flexi. TODO, deal with this properly.
                StepgenConfigs[i].StepPin,
                StepgenConfigs[i].DirectionPin,
                Config::stepBit,
                *ptrJointFreqCmd,
                *ptrJointFeedback,
                *ptrJointEnable,
                true // usesModulePost
            );
            baseThread->registerModule(stepgen);
            baseThread->registerModulePost(stepgen);
        }

        //Digital outputs
        volatile uint16_t* ptrOutputs = &rxData.outputs;

        for (size_t i = 0; i < DigitalOutputCount; i++) {
            printf("\nCreating digital output for %s\n", DOConfigs[i].Comment);
            std::shared_ptr<Module> digitalOutput = std::make_shared<DigitalPin>(
                *ptrOutputs,
                1, //mode, 1 = output
                DOConfigs[i].Pin,
                DOConfigs[i].DataBit,
                DOConfigs[i].Invert,
                DOConfigs[i].Modifier
            );
            servoThread->registerModule(digitalOutput);
        }


        //Digital inputs
        volatile uint16_t* ptrInputs = &txData.inputs;

        for (size_t i = 0; i < DigitalInputCount; i++) {
            printf("\nCreating digital input for %s\n", DIConfigs[i].Comment);
            std::shared_ptr<Module> digitalInput = std::make_shared<DigitalPin>(
                *ptrInputs,
                0, //mode, 0 = input
                DIConfigs[i].Pin,
                DIConfigs[i].DataBit,
                DIConfigs[i].Invert,
                DIConfigs[i].Modifier
            );
            servoThread->registerModule(digitalInput);
        }

        //PRU Reset
        volatile bool PRUreset;
        printf("Creating PRU Reset Pin at pin %s\n", PRU_Reset_Pin);
        std::shared_ptr<Module> resetPin = std::make_shared<ResetPin>(
            &PRUreset,
            PRU_Reset_Pin
            );

        baseThread->registerModule(resetPin);  // Register PRU reset module

        //Spindle PWM
        for (size_t i = 0; i < PWMCount; i++) {
            printf("\nCreating PWM for %s at pin %s\n", PWMConfigs[i].Comment, PWMConfigs[i].Pin);
            printf("TODO. Not yet implemented.");
            //TODO - Port in software PWM module. Needed here.
        }


        //QEI, Process Variable 0
        //TODO





    }
#else
    void Remora::loadModules()
    {
        ModuleFactory* factory = ModuleFactory::getInstance();
        JsonArray modules = configHandler->getModules();
        if (modules.isNull()) {
        // Log something about missing modules or return early
        }

        printf("\nCreating modules from config\n");

        for (size_t i = 0; i < modules.size(); i++) {
            if (modules[i]["Thread"].is<const char*>() && modules[i]["Type"].is<const char*>()) {
                const char* threadName = modules[i]["Thread"];
                const char* moduleType = modules[i]["Type"];
                uint32_t threadFreq = 0;

                // Determine the thread frequency based on the thread name
                if (strcmp(threadName, "Servo") == 0) {
                    threadFreq = servoFreq;
                } else if (strcmp(threadName, "Base") == 0) {
                    threadFreq = baseFreq;
                }

                // Add the "ThreadFreq" key and its value to the module's JSON object
                modules[i]["ThreadFreq"] = threadFreq;

                // Create module using factory
                std::shared_ptr<Module> _mod = factory->createModule(threadName, moduleType, modules[i], this);

                // Check if the module creation was successful
                if (!_mod) {
                    printf("Error: Failed to create module of type '%s' for thread '%s'. Skipping registration.\n",
                            moduleType, threadName);
                    continue; // Skip to the next iteration
                }

                bool _modPost = _mod->getUsesModulePost();

                if (strcmp(threadName, "Servo") == 0) {
                    servoThread->registerModule(_mod);
                    if (_modPost) {
                        servoThread->registerModulePost(_mod);
                    }
                }
                else if (strcmp(threadName, "Base") == 0) {
                    baseThread->registerModule(_mod);
                    if (_modPost) {
                        baseThread->registerModulePost(_mod);
                    }
                }
                else {
                    onLoad.push_back(std::move(_mod));
                }
            }
        }
    }
#endif
