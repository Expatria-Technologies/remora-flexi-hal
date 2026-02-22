
#include "staticConfigHandler.h"
#include "../remora.h"
#include "remora-hal/board_config.h"
#include "crc32.h"

StaticConfigHandler::StaticConfigHandler(Remora* _remora) :
	remoraInstance(_remora)
{
	uint8_t status = loadConfiguration();
    remoraInstance->setStatus(status);
    if (status != 0x00) {
        return;
    }
    updateThreadFreq();
}

uint8_t StaticConfigHandler::loadConfiguration() {

   printf("loading static config\n");

}

void StaticConfigHandler::updateThreadFreq() {

    printf("Updating thread frequency - Setting BASE thread frequency to %lu\n", baseFreq);
    remoraInstance->setBaseFreq(baseFreq);

    printf("Updating thread frequency - Setting SERVO thread frequency to %lu\n", servoFreq);
    remoraInstance->setServoFreq(servoFreq);

    printf("BASE thread frequency set to: %lu\n", remoraInstance->getBaseFreq());
    printf("SERVO thread frequency set to: %lu\n", remoraInstance->getServoFreq());

}
