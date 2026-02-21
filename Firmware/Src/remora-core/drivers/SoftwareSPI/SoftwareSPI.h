#ifndef SOFTWARESPI_H
#define SOFTWARESPI_H

#include <cstdint>
#include <string>
#include "../../../remora-hal/pin/pin.h"

enum BitOrder { MSB_FIRST, LSB_FIRST };
enum ByteOrder { MSB_FIRST_BYTE, LSB_FIRST_BYTE };
enum SPImodes { SPI_MODE_0, SPI_MODE_1, SPI_MODE_2, SPI_MODE_3 };

class SoftwareSPI {
private:
    Pin mosi;
    Pin miso;
    Pin clk;
    Pin* cs; // Optional chip select pin (nullptr if externally handled)
    uint32_t delayTicks;
    uint8_t mode;
    bool cpol; // Clock Polarity (0 = Idle Low, 1 = Idle High)
    bool cpha; // Clock Phase (0 = Sample on first edge, 1 = Sample on second edge)
    BitOrder bitOrder;
    ByteOrder byteOrder;

public:
    // Constructor WITHOUT chip select
    SoftwareSPI(const std::string& mosiPin, const std::string& misoPin, const std::string& clkPin,
                uint8_t mode, BitOrder bitOrder = MSB_FIRST, ByteOrder byteOrder = MSB_FIRST_BYTE);

    // Constructor WITH chip select
    SoftwareSPI(const std::string& mosiPin, const std::string& misoPin, const std::string& clkPin, const std::string& csPin,
    		uint8_t mode, BitOrder bitOrder = MSB_FIRST, ByteOrder byteOrder = MSB_FIRST_BYTE);

    ~SoftwareSPI();

    void begin();
    void end();
    void setSPIMode(uint8_t mode);
    void setClockDivider(uint32_t div);
    void transfer(uint8_t* data, size_t length);
    uint8_t transfer(uint8_t value);
    void transferEmptyBytes(uint8_t n);
    void delay();
};

#endif