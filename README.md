# Remora Flexi-HAL

This is a port of the excellent Remora firmware for the Flexi-HAL.  
https://github.com/Expatria-Technologies/Flexi-HAL

Different in this fork from the 'standard' Remora is the lack of a config file. The board has a well defined set of inputs and outputs, typically with a single function. Within the LinuxCNC component the 'pins' are named intuitively to attempt to make HAL configuration easier. Pins are defined in board_config.h and the modules loaded in main.cpp in an attempt to keep the modules unmodified for easier maintenance as they are developed upstream.

Integration with the UF2 bootloader that ships on the Flexi-HAL is still a WIP; at this point the image must be installed via DFU or via the serial bootloader and will overwrite the UF2 bootloader. A script is provided in the Tools directory to do this from the Pi via the Pi header, including resetting the STM32 and managing the boot0 pin. This script can be used to re-install the UF2 bootloader as well if desired. stm32flash will need to be installed on the Pi (it is pre-installed in the Flexi-Pi image).

The uf2 bootloader can be found here if you want to restore it:

https://github.com/Expatria-Technologies/tinyuf2/releases

Firmware must be built with Mbed Studio with the 'FLEXIHAL' target, and the 'Release' configuration must be selected. A pre-compiled binary is provided in the FirmwareBin directory for convenience. 


## Flexi-Pi 

Included under 'releases' is a Pi 4 image which built on the official LinuxCNC image. It is pre-configured with all of the required dependencies. The base configuration runs Probe Basic, and it functions as expected apart from the ATC tab, which currently does nothing. Unzip it and flash it to an SD card using dd, balenaEtcher, or the tool of your choice. The filesystem will be automatically resized on first boot to fill the entirety of the SD card. 

The minimum SD card size is 8GB, though a 16GB or larger card is recommended. A U3 High Endurance card from a reputable manufacturer is the preferred option, such as those available from Sandisk. Slower cards will result in a noticeable reduction in performance. 

**Running the updater built into the Pi wizard will currently result in the image not booting after updating. Skip the updates until this can be corrected.** 

## Flexi-HAL Configuration
Jumpers need to be in place in the marked locations on the Flexi-HAL for the Pi to communicate with the MCU for firmware flashing:

<img src="/Images/Jumper_locations.png" width="500">

These are shipped in place by default from the Expatria shop, but if they have been moved or removed they will need to be replaced. 


## Reference config
A reference config is included in the [LinuxCNC/ConfigSamples/flexi-hal](./LinuxCNC/ConfigSamples/flexi-hal) directory. **This will need to be edited for your specific machine.** 

There are VFD options in the reference config for either vfdmod (configured for a Durapulse GS10 here), and hy_vfd (for Huanyang VFDs). If using hy_vfd, the vfdmod items will need to be commented out, and the hy_vfd items uncomments in both [remora-flexi.hal](./LinuxCNC/ConfigSamples/flexi-hal/remora-flexi.hal) and [spindle_load.hal](./LinuxCNC/ConfigSamples/flexi-hal/spindle_load.hal). 

Machine travels, limits, and other configuration is done in [remora-flexi.ini](./LinuxCNC/ConfigSamples/flexi-hal/remora-flexi.ini). 
 
---
## Remora
Remora is a free, opensource LinuxCNC component and Programmable Realtime Unit (PRU) firmware to allow LPC17xx and STM32F4 base controller boards to be used in conjuction with a Raspberry Pi to implement a LinuxCNC based CNC controller.

Having a low cost and accessable hardware platform for LinuxCNC is important if we want to use LinuxCNC for 3D printing for example. Having a controller box the size of the printer itself makes no sense in this applicatoin. A SoC based single board computer is ideal in this application. Although developed for 3D Printing, Remora (and LinuxCNC) is highly flexible and configurable for other CNC applications.

Remora has been in use amd development since 2017. Starting on Raspberry Pi 3B and 3B+ eventhough at the time it was percieved that the Raspberry Pi was not a viable hardware for LinuxCNC.

With the release of the RPi 4 the LinuxCNC community now supports the hardware, with LinuxCNC and Preempt-RT Kernel packages now available from the LinuxCNC repository. This now greatly simplifies the build of a Raspberry Pi based CNC controller.
