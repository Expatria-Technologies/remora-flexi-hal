#ifndef BOARD_LED_STATUS_H
#define BOARD_LED_STATUS_H

#include "pin/pin.h"

#define MAX_FLASHES_IN_SEQUENCE 6
#define MAX_ERROR_CODES 64      // 2 ^ 6

// We have a set of flashing light error code sequences to indicate specific problems. 
typedef enum {
    CRITICAL_HAL_ERROR,
    SD_CARD_HW_ERROR,
    SD_CARD_MOUNT_ERROR,
    SD_CARD_FILE_ERROR,
    SPI_PERIPH_ERROR    // more to be added as we go. 
} flashing_led_error_code;

typedef struct {
    flashing_led_error_code error_code;
    uint16_t sequence[MAX_FLASHES_IN_SEQUENCE];
} error_sequence;

void init_board_status_led(std::string pin_name);
void flash_led_error(flashing_led_error_code error_code);

#endif

