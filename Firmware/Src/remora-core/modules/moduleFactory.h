#ifndef MODULE_FACTORY_H
#define MODULE_FACTORY_H

#include <algorithm>
#include <functional>
#include <memory>
#include <string>

#include "../remora.h"
#include "module.h"
#include "../config/jsonConfigHandler.h"

class ModuleFactory {
private:

    // Private constructor for singleton
    ModuleFactory() {};

public:
    static ModuleFactory* getInstance();
    // Create module based on thread and type
    std::shared_ptr<Module> createModule(const char*, const char*, const JsonVariant, Remora*);

};

#endif
