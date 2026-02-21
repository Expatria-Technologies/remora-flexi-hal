#include <cstdio>
#include <algorithm>
#include "pruThread.h"
#include "../modules/module.h"

pruThread::pruThread(const std::string& _name)
    : threadName(_name)
{}

void pruThread::setTimer(std::unique_ptr<pruTimer> _timer)
{
	timerPtr = std::move(_timer);
    timerPtr->setOwner(this);
}

bool pruThread::executeModules()
{
    for (const auto& module : modules) if (module) module->runModule();
    if (hasModulesPost) for (const auto& module : modulesPost) if (module) module->runModulePost();
    return true;
}

bool pruThread::registerModule(std::shared_ptr<Module> module)
{
    if (!module) return false;
    modules.push_back(module);
    return true;
}

bool pruThread::registerModulePost(std::shared_ptr<Module> module)
{
    if (!module) return false;
    hasModulesPost = true;
    modulesPost.push_back(module);
    return true;
}

bool pruThread::unregisterModule(std::shared_ptr<Module> module)
{
    if (!module) return false;

    auto iter = std::remove_if(modules.begin(), modules.end(),
        [&module](const std::shared_ptr<Module>& mod) {
            return mod == module;
        });
    modules.erase(iter, modules.end());

    auto postIter = std::remove_if(modulesPost.begin(), modulesPost.end(),
        [&module](const std::shared_ptr<Module>& mod) {
            return mod == module;
        });
    modulesPost.erase(postIter, modulesPost.end());

    hasModulesPost = !modulesPost.empty();

    return true;
}

bool pruThread::startThread()
{
    if (isRunning()) return true;
    setThreadRunning(true);
    setThreadPaused(false);
	timerPtr->configTimer();
    timerPtr->startTimer();
    return true;
}

void pruThread::stopThread()
{
    setThreadRunning(false);
    setThreadPaused(false);
    timerPtr->stopTimer();
}

bool pruThread::update()
{
    if (!isRunning() || isPaused()) return true;
    return executeModules();
}

void pruThread::pauseThread() { setThreadPaused(true); }
void pruThread::resumeThread() { setThreadPaused(false); }
const std::string& pruThread::getName() const { return threadName; }
uint32_t pruThread::getFrequency() const { return timerPtr ? timerPtr->getFrequency() : 0; }
