
#include "moduleList.h"
#include "moduleFactory.h"


// Create module based on thread and type
std::shared_ptr<Module> ModuleFactory::createModule(const char* _tname,
                                   const char* _mtype,
                                   const JsonVariant config,
                                   Remora* instance) {
    if (strcmp(_tname, "Base") == 0) {
        if (strcmp(_mtype, "Stepgen") == 0) {
            return Stepgen::create(config, instance);
        } else if (strcmp(_mtype, "Encoder") == 0) {
            return SoftEncoder::create(config, instance);    
        }        
    } else if (strcmp(_tname, "Servo") == 0) {
        if (strcmp(_mtype, "Blink") == 0) {
            return Blink::create(config, instance);
        } else if (strcmp(_mtype, "Reset Pin") == 0) {
            return ResetPin::create(config, instance);
        } else if (strcmp(_mtype, "Digital Pin") == 0) {
            return DigitalPin::create(config, instance);
        } else if (strcmp(_mtype, "Sigma Delta") == 0) {
            return SigmaDelta::create(config, instance);
        } else if (strcmp(_mtype, "Temperature") == 0) {
            return Temperature::create(config, instance);
        } else if (strcmp(_mtype, "PWM") == 0) {
            return PWM::create(config, instance); 
        } else if (strcmp(_mtype, "Analog Pin") == 0) {
            return AnalogPin::create(config, instance);                      
        } else if (strcmp(_mtype, "QEI") == 0) {
            return QEI::create(config, instance);             
        }
    } else if (strcmp(_tname, "On load") == 0) {
    	if (strcmp(_mtype, "TMC2208") == 0) {
   	        return TMC2208::create(config, instance);
   	    } else if (strcmp(_mtype, "TMC2209") == 0) {
    		return TMC2209::create(config, instance);
    	} else if (strcmp(_mtype, "TMC5160") == 0) {
    		return TMC5160::create(config, instance);
    	}
    } else {
        printf("Error: Unknown thread type '%s' or module type '%s'\n", _tname, _mtype);
    }

    return nullptr;
}

// Static instance accessor
ModuleFactory* ModuleFactory::getInstance() {
    static ModuleFactory* instance = new ModuleFactory();
    return instance;
}
