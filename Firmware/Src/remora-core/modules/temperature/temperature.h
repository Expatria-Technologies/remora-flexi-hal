#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <cstdint>
#include <string>

#include "../../remora.h"
#include "../../modules/module.h"
#include "../../sensors/tempSensor.h"
#include "../../sensors/thermistor/thermistor.h"

class Temperature : public Module
{
  private:
    
    volatile float* ptrFeedback;       	   // pointer where to put the feedback
    std::string sensorType;       // temperature sensor type
    std::string pinSensor;	             // physical pins connections

    float temperaturePV;

    // thermistor parameters
    float beta;
    float r0;
		float t0;

  public:

    Temperature(volatile float&, int32_t, int32_t, std::string, std::string, float, int, int);  // Thermistor type constructor
    static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
    
    TempSensor* Sensor;

    virtual void update(void);           // Module default interface
    virtual void slowUpdate(void);
};


#endif
