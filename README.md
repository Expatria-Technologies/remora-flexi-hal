
# Remora Flexi-HAL

This is a port of the excellent Remora firmware for the Flexi-HAL.  
https://github.com/Expatria-Technologies/Flexi-HAL

Different in this fork from the 'standard' Remora is the lack of a config file. The board has a well defined set of inputs and outputs, typically with a single function. Within the LinuxCNC component the 'pins' are named intuitively to attempt to make HAL configuration easier. Pins are defined in board_config.h and the modules loaded in main.cpp in an attempt to keep the modules unmodified for easier maintenance as they are developed upstream.

A Pi image will be provided at a later date pre-configured with the required components and dependencies pre-compiled and a base configuration installed.

Integration with the UF2 bootloader that ships on the Flexi-HAL is still a WIP; at this point the image must be installed via DFU or via the serial bootloader and will overwrite the UF2 bootloader. A script is provided in the Tools directory to do this from the Pi via the Pi header, including resetting the STM32 and managing the boot0 pin. This script can be used to re-install the UF2 bootloader as well if desired. stm32flash will need to be installed on the Pi.

Firmware must be built with Mbed Studio with the 'FLEXIHAL' target, and the 'Release' configuration must be selected. A pre-compiled binary is provided in the FirmwareBin directory for convienience. 




# Remora
Remora is a free, opensource LinuxCNC component and Programmable Realtime Unit (PRU) firmware to allow LPC17xx and STM32F4 base controller boards to be used in conjuction with a Raspberry Pi to implement a LinuxCNC based CNC controller.

Having a low cost and accessable hardware platform for LinuxCNC is important if we want to use LinuxCNC for 3D printing for example. Having a controller box the size of the printer itself makes no sense in this applicatoin. A SoC based single board computer is ideal in this application. Although developed for 3D Printing, Remora (and LinuxCNC) is highly flexible and configurable for other CNC applications.

Remora has been in use amd development since 2017. Starting on Raspberry Pi 3B and 3B+ eventhough at the time it was percieved that the Raspberry Pi was not a viable hardware for LinuxCNC.

With the release of the RPi 4 the LinuxCNC community now supports the hardware, with LinuxCNC and Preempt-RT Kernel packages now available from the LinuxCNC repository. This now greatly simplifies the build of a Raspberry Pi based CNC controller.


