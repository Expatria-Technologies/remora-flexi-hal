#include "commsInterface.h"

CommsInterface::CommsInterface() {
    // Constructor implementation
}

void CommsInterface::init(){}
void CommsInterface::start(){}
void CommsInterface::tasks(){}

uint8_t CommsInterface::read_byte(void) { return 0; }
uint8_t CommsInterface::write_byte(uint8_t byte) { return 0; }
void CommsInterface::DMA_write(uint8_t *data, uint16_t len) {}
void CommsInterface::DMA_read(uint8_t *data, uint16_t len) {}
void CommsInterface::flag_new_data(void) {}

