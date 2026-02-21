#include "board_led_status.h"

Pin* board_status_led = nullptr;

constexpr uint16_t ERROR_SPACER = 1500;     // time in ms before repeating the error code
constexpr uint16_t SLOW_PULSE = 1000;       // full second in ms
constexpr uint16_t FAST_PULSE = 250;        // quarter second in ms
constexpr uint16_t PULSE_SPACER = 125;      // eighth second in ms

const error_sequence error_sequences[] = { // the pattern alternates between short and long pulse times, a bit like morse code
    { CRITICAL_HAL_ERROR, {FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE}},
    { SD_CARD_HW_ERROR, {FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE}},
    { SD_CARD_MOUNT_ERROR, {FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE}},
    { SD_CARD_FILE_ERROR, {FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE}},
    { SPI_PERIPH_ERROR, {FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE}}

    // every combination has been pre-generated, grab more as needed
    // { (flashing_led_error_code)5, {FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)6, {FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)7, {FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)8, {FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)9, {FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)10,{FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)11,{FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)12,{FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)13,{FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)14,{FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)15,{FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)16,{FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)17,{FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)18,{FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)19,{FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)20,{FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)21,{FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)22,{FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)23,{FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)24,{FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)25,{FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)26,{FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)27,{FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)28,{FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)29,{FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)30,{FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)31,{FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)32,{SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)33,{SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)34,{SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)35,{SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)36,{SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)37,{SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)38,{SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)39,{SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)40,{SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)41,{SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)42,{SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)43,{SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)44,{SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)45,{SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)46,{SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)47,{SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)48,{SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)49,{SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)50,{SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)51,{SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)52,{SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)53,{SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)54,{SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)55,{SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)56,{SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)57,{SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)58,{SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)59,{SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)60,{SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)61,{SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE, SLOW_PULSE}},
    // { (flashing_led_error_code)62,{SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, FAST_PULSE}},
    // { (flashing_led_error_code)63,{SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE, SLOW_PULSE}}
};

void init_board_status_led(std::string pin_name) 
{
    board_status_led = new Pin(pin_name, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0);
    board_status_led->set(true); 
}

void flash_led_error(flashing_led_error_code error_code)       // this is to be called before error_handler. It puts the MCU into infinite loop and a flashing light error code. If for some reason it cant, then error_handler is the fallback. 
{
    board_status_led->set(false); 
    
    int error_index = -1;
    // find the right sequence from the code
    for(uint8_t i = 0; i < MAX_ERROR_CODES; i++) 
    {
        if (error_sequences[i].error_code == error_code) {
            error_index = i; 
            break;
        }
    }
    if (error_index == -1) 
    {   // really critical error, leave the status led off and let error_handler hang. 
        return;
    }

    while(1) 
    {        
        for(uint8_t i = 0; i < MAX_FLASHES_IN_SEQUENCE; i++) 
        {
            board_status_led->set(true); 
            HAL_Delay(error_sequences[error_index].sequence[i]);
            board_status_led->set(false); 
            HAL_Delay(PULSE_SPACER);
        }

        HAL_Delay(ERROR_SPACER);
    }
}

