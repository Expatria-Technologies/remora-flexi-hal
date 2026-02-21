#include "pruTimer.h"
#include "timerInterrupt.h"

pruTimer::~pruTimer() = default;

void pruTimer::setOwner(pruThread* owner) {
    timerOwnerPtr = owner;
}

void pruTimer::setFrequency(uint32_t freq) {
    if (timerRunning) {
        stopTimer();
        frequency = freq;
        configTimer();
        startTimer();
    } else {
        frequency = freq;
    }
}

uint32_t pruTimer::getFrequency() const {
    return frequency;
}
