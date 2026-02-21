#ifndef PLATFORM_CONFIGURATION_H
#define PLATFORM_CONFIGURATION_H

#include "stm32f4xx.h"

#include <cstdint>

/*
There are numerous build targets within the F4 family supported by this codebase.
The bootloader location, program start location, flash memory locations can all be configured to work with the target via the linkerscript.
This code below pulls this in from the linker script nomimated in Platform IO to ensure Remora behaves correctly.

Please see the linker script and the platformio.ini file for more information about your specific build target. 
*/

extern "C" {
    extern const uint8_t _ls_json_upload_start;
    extern const uint8_t _ls_json_upload_end;
    extern const uint8_t _ls_json_storage_start;
    extern const uint8_t _ls_json_storage_end;
    extern const uint8_t _ls_json_upload_sector;
    extern const uint8_t _ls_json_storage_sector;     
}

namespace Platform_Config {
    const std::uintptr_t JSON_upload_start_address  = reinterpret_cast<std::uintptr_t>(&_ls_json_upload_start);
    const std::uintptr_t JSON_upload_end_address    = reinterpret_cast<std::uintptr_t>(&_ls_json_upload_end);

    const std::uintptr_t JSON_storage_start_address = reinterpret_cast<std::uintptr_t>(&_ls_json_storage_start);
    const std::uintptr_t JSON_storage_end_address   = reinterpret_cast<std::uintptr_t>(&_ls_json_storage_end);

    const uint32_t JSON_Config_Upload_Sector        = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(&_ls_json_upload_sector));      // this is a bit finnicky beacuse the linker scripts store numeric addresses only, not values
    const uint32_t JSON_Config_Storage_Sector       = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(&_ls_json_storage_sector));     // also the need for 32 bit integer here to store a value from 0-8 is deliberate....
}

#endif