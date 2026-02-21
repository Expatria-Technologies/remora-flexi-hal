#ifndef PRUTIMER_H
#define PRUTIMER_H

#include <cstdint>
#include <memory>

class TimerInterrupt;  // Forward declaration
class pruThread;

class pruTimer {
protected:
    std::unique_ptr<TimerInterrupt> interruptPtr;
    uint32_t frequency = 0;
    pruThread* timerOwnerPtr = nullptr;
    bool timerRunning = false;

public:
    virtual ~pruTimer();

    void setOwner(pruThread* owner);
    void setFrequency(uint32_t freq);
    uint32_t getFrequency() const;

    virtual void configTimer() = 0;
    virtual void startTimer() = 0;
    virtual void stopTimer() = 0;
    virtual void timerTick() = 0;
};

#endif // PRUTIMER_H
