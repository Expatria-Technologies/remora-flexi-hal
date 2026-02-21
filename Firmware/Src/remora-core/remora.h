#ifndef REMORA_H
#define REMORA_H

#include <memory>
#include <vector>

#include <ArduinoJson.h>

#include "configuration.h"
#include "data.h"
#include "remoraStatus.h"
#include "comms/commsInterface.h"
#include "modules/moduleFactory.h"
#include "modules/moduleList.h"
#include "thread/pruThread.h"

#define MAJOR_VERSION 	2
#define MINOR_VERSION	0
#define PATCH			0

class CommsHandler;

#ifndef STATIC_CONFIG
    class JsonConfigHandler;
#else
    class StaticConfigHandler;
#endif

class Remora {
private:

    enum State {
        ST_SETUP = 0,
        ST_START,
        ST_IDLE,
        ST_RUNNING,
        ST_STOP,
        ST_RESET,
        ST_SYSRESET
    };

    State currentState;
    State prevState;

    volatile txData_t*  ptrTxData;
    volatile rxData_t*  ptrRxData;
    bool reset = false;

    #ifndef STATIC_CONFIG
        std::unique_ptr<JsonConfigHandler> configHandler;
    #else
        std::unique_ptr<StaticConfigHandler> configHandler;
    #endif

    std::shared_ptr<CommsHandler> comms;

    std::unique_ptr<pruThread> baseThread;
    std::unique_ptr<pruThread> servoThread;
    std::unique_ptr<pruThread> serialThread;
    std::vector<std::shared_ptr<Module>> onLoad;

    uint32_t baseFreq;
    uint32_t servoFreq;
    uint32_t serialFreq;

    uint8_t remoraStatus;
    bool fatalErrorHandled;

    bool threadsRunning = false;

    void updateHeader();
    void transitionToState(State);
	void printStateEntry(State);
    void handleSetupState();
    void handleStartState();
    void handleIdleState();
    void handleRunningState();
    void handleResetState();
    void handleSysResetState();
    void startThread(const std::unique_ptr<pruThread>&, const char*);
    void resetBuffer(volatile uint8_t*, size_t);
    void loadModules();
    void loadStaticModules();

public:

    Remora(std::shared_ptr<CommsHandler> commsHandler,
           std::unique_ptr<pruTimer> baseTimer,
           std::unique_ptr<pruTimer> servoTimer,
           std::unique_ptr<pruTimer> serialTimer = nullptr);

    void run();
	
    void setBaseFreq(uint32_t freq) { baseFreq = freq; /* baseThread->setFrequency(freq); */}        // these setters and getters works at init during JSON Config, but not during run time, could use a refactor. Would require a new setter method in pruThread::
    void setServoFreq(uint32_t freq) { servoFreq = freq; /* servoThread->setFrequency(freq); */}
    uint32_t getBaseFreq(void) { return baseFreq; /* return baseThread->getFrequency();*/}
    uint32_t getServoFreq(void) { return servoFreq; /* return servoThread->getFrequency(); */}    
    void setStatus(uint8_t status) { remoraStatus = status; }
    uint8_t getStatus() { return remoraStatus; }

    volatile txData_t* getTxData() { return &txData; }
    volatile rxData_t* getRxData() { return &rxData; }
    volatile bool* getReset() { return &reset; }
    pruThread* getSerialThread() { return serialThread.get(); }
};

#endif
