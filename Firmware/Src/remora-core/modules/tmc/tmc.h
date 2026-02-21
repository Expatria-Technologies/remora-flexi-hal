#ifndef TMCMODULE_H
#define TMCMODULE_H

#include <cstdint>
#include <string>

#include "../../remora.h"
#include "../../modules/module.h"
#include "../../drivers/TMCStepper/TMCStepper.h"
#include "../../remoraStatus.h"

class TMC : public Module, public std::enable_shared_from_this<TMC>
{
protected:

	Remora* 	instance;
	float       Rsense;

public:

	TMC(Remora* _instance, float _Rsense) : instance(_instance), Rsense(_Rsense) {}

	virtual void update(void) = 0;           // Module default interface
	virtual void configure(void) = 0;

    std::shared_ptr<TMC> getShared() {
        return shared_from_this();
    }
};


class TMC2208 : public TMC
{
protected:

	std::string rxtxPin;     // default to half duplex
	uint16_t    mA;
	uint16_t    microsteps;
	bool        stealth;

	std::unique_ptr<TMC2208Stepper> driver;

public:

	TMC2208(std::string, float, uint16_t, uint16_t, bool, Remora*);
	static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
	~TMC2208() = default;

    void update(void) override;
    void configure(void) override;
};


class TMC2209 : public TMC
{
protected:

	std::string rxtxPin;     // default to half duplex
	uint8_t     addr;
	uint16_t    mA;
	uint16_t    microsteps;
	bool        stealth;
	uint16_t    stall;

	std::unique_ptr<TMC2209Stepper> driver;

public:

	TMC2209(std::string, float, uint8_t, uint16_t, uint16_t, bool, uint16_t, Remora*);
	static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
	~TMC2209() = default;

    void update(void) override;
    void configure(void) override;
};

class TMC5160 : public TMC
{
protected:

	std::string pinCS;
	std::string pinMOSI;
	std::string pinMISO;
	std::string pinSCK;
	uint8_t     addr;
	uint16_t    mA;
	uint16_t    microsteps;
	bool        stealth;
	uint16_t    stall;

	std::unique_ptr<TMC5160Stepper> driver;

public:

	TMC5160(std::string, std::string, std::string, std::string, float, uint8_t, uint16_t, uint16_t, bool, uint16_t, Remora*);
	static std::shared_ptr<Module> create(const JsonObject& config, Remora* instance);
	~TMC5160() = default;

    void update(void) override;
    void configure(void) override;
};

#endif
