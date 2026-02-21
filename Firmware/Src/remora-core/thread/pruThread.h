#ifndef PRUTHREAD_H
#define PRUTHREAD_H

#include <vector>
#include <memory>
#include <string>
#include <atomic>

#include "pruTimer.h"

class Module;


class pruThread
{
private:
    std::string threadName;
    std::unique_ptr<pruTimer> timerPtr;
    std::atomic<bool> threadRunning{false};
    std::atomic<bool> threadPaused{false};
    std::vector<std::shared_ptr<Module>> modules;
    std::vector<std::shared_ptr<Module>> modulesPost;
    bool hasModulesPost{false};

    void setThreadRunning(bool val) { threadRunning.store(val, std::memory_order_release); }
    void setThreadPaused(bool val) { threadPaused.store(val, std::memory_order_release); }
    bool executeModules();

public:
    pruThread(const std::string& _name);
    void setTimer(std::unique_ptr<pruTimer> _timer);
    bool registerModule(std::shared_ptr<Module> module);
    bool registerModulePost(std::shared_ptr<Module> module);
    bool unregisterModule(std::shared_ptr<Module> module);
    bool isRunning() const { return threadRunning.load(std::memory_order_acquire); }
    bool isPaused() const { return threadPaused.load(std::memory_order_acquire); }
    bool startThread(void);
    void stopThread(void);
    bool update();
    void pauseThread();
    void resumeThread();
    const std::string& getName() const;
    uint32_t getFrequency() const;
};

#endif
