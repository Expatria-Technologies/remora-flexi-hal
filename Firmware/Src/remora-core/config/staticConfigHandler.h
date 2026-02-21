#ifndef STATIC_CONFIG_HANDLER_H
#define STATIC_CONFIG_HANDLER_H

#include <string>


class Remora; //forward declaration

class StaticConfigHandler {
private:

	Remora* remoraInstance;
	uint8_t loadConfiguration();


public:
	StaticConfigHandler(Remora* _remora);
	void updateThreadFreq();
};
#endif
