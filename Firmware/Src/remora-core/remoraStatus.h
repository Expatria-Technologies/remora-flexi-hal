#ifndef REMORASTATUS_H
#define REMORASTATUS_H

#include <cstdint>

// Status byte structure:
// Bit 7   = FATAL flag
// Bits 6-4 = ERROR SOURCE
// Bits 3-0 = ERROR CODE

enum class RemoraErrorSource : uint8_t {
    NO_ERROR      = 0x00,
    CORE          = (0x01 << 4), // 0x10
    JSON_CONFIG   = (0x02 << 4), // 0x20
    MODULE_LOADER = (0x03 << 4), // 0x30
    TMC_DRIVER    = (0x04 << 4), // 0x40
};

enum class RemoraErrorCode : uint8_t {
    NO_ERROR                  = 0x00,

    // CORE
    REMORA_CORE_ERROR         = 0x01,

    // JSON_CONFIG
    SD_MOUNT_FAILED           = 0x01,
    CONFIG_FILE_OPEN_FAILED   = 0x02,
    CONFIG_FILE_READ_FAILED   = 0x03,
    CONFIG_INVALID_INPUT      = 0x04,
    CONFIG_NO_MEMORY          = 0x05,
    CONFIG_PARSE_FAILED       = 0x06,
    CONFIG_LOADED_DEFAULT     = 0x07,

    // MODULE_LOADER
    MODULE_CREATE_FAILED      = 0x01,

    // TMC_DRIVER
    TMC_DRIVER_ERROR          = 0x01,
};

// Create status byte: Bit 7 is FATAL, 6-4 is Source, 3-0 is Code
inline uint8_t makeRemoraStatus(RemoraErrorSource source, RemoraErrorCode code, bool fatal = false) {
    uint8_t status = (static_cast<uint8_t>(source) & 0x70) | (static_cast<uint8_t>(code) & 0x0F);
    if (fatal) status |= 0x80;
    return status;
}

#endif