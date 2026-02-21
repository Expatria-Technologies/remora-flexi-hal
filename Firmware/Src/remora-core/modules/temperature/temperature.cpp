#include "temperature.h"


std::shared_ptr<Module> Temperature::create(const JsonObject& config, Remora* instance)
{
    const char* comment = config["Comment"];
    uint32_t threadFreq = config["ThreadFreq"];
    
    printf("%s\n",comment);

    int pv = config["PV[i]"];
    const char* sensor = config["Sensor"];

    volatile float* ptrProcessVariable = &instance->getTxData()->processVariable[pv];

    if (!strcmp(sensor, "Thermistor"))
    {
        const char* pinSensor = config["Thermistor"]["Pin"];
        float beta =  config["Thermistor"]["beta"];
        int r0 = config["Thermistor"]["r0"];
        int t0 = config["Thermistor"]["t0"];

        // slow module with 1 hz update
        int updateHz = 1;
        return std::make_unique<Temperature>(*ptrProcessVariable, threadFreq, updateHz, sensor, pinSensor, beta, r0, t0);
    }
    else return nullptr;
}



Temperature::Temperature(volatile float &ptrFeedback, int32_t threadFreq, int32_t slowUpdateFreq, std::string sensorType, std::string pinSensor, float beta, int r0, int t0) :
  Module(threadFreq, slowUpdateFreq),
  ptrFeedback(&ptrFeedback),
  sensorType(sensorType),
  pinSensor(pinSensor),
	beta(beta),
	r0(r0),
	t0(t0)
{
    if (this->sensorType == "Thermistor")
    {
        printf("Creating Thermistor Tempearture measurement @ pin %s\n", this->pinSensor.c_str());
        //cout <<"Creating Thermistor Tempearture measurement @ pin " << this->pinSensor << endl;
        this->Sensor = new Thermistor(this->pinSensor, this->beta, this->r0, this->t0);
    }
    // TODO: Add more sensor types as needed

    // Take some readings to get the ADC up and running before moving on
    this->slowUpdate();
    this->slowUpdate();
}

void Temperature::update()
{
  return;
}

void Temperature::slowUpdate()
{
	this->temperaturePV = this->Sensor->getTemperature();

    // check for disconnected temperature sensor
    if (this->temperaturePV > 0)
    {
        *(this->ptrFeedback) = this->temperaturePV;
    }
    else
    {
        printf("Temperature sensor error, pin %s reading = %f\n", this->pinSensor.c_str(), this->temperaturePV);
        //cout << "Temperature sensor error, pin " << this->pinSensor << " reading = " << this->temperaturePV << endl;
        *(this->ptrFeedback) = 999;
    }

}
