# Remora Flexi-HAL

This is a port of the excellent [Remora](https://github.com/scottalford75/Remora) firmware for the [Flexi-HAL](https://github.com/Expatria-Technologies/Flexi-HAL). 

Using this firmware with LinuxCNC requires a Raspberry Pi 4 or 5 (Pi 5 is recommended). A pre-configured Pi image is avilable in the [Releases](https://github.com/Expatria-Technologies/Flexi-Pi/releases) section of the [Flexi-Pi](https://github.com/Expatria-Technologies/Flexi-Pi) repository, where you will also find some setup notes in the [README](https://github.com/Expatria-Technologies/Flexi-Pi/blob/master/README_cdn.md).

Different in this fork from the 'standard' Remora is the lack of a config file. The board has a well defined set of inputs and outputs, typically with a single function. Within the LinuxCNC component the 'pins' are named intuitively to attempt to make HAL configuration easier. Pins are defined in board_config.h and the modules loaded in main.cpp in an attempt to keep the modules unmodified for easier maintenance as they are developed upstream.

Integration with the UF2 bootloader that ships on the Flexi-HAL is still a WIP; at this point the image must be installed via DFU or via the serial bootloader and will overwrite the UF2 bootloader. A script is provided in the Tools directory to do this from the Pi via the Pi header, including resetting the STM32 and managing the boot0 pin. This script can be used to re-install the UF2 bootloader as well if desired. stm32flash will need to be installed on the Pi (it is pre-installed in the Flexi-Pi image).

The uf2 bootloader can be found here if you want to restore it:

https://github.com/Expatria-Technologies/tinyuf2/releases

Firmware must be built with Mbed Studio with the 'FLEXIHAL' target, and the 'Release' configuration must be selected. A pre-compiled binary is provided in the FirmwareBin directory for convenience. 


## Changes

<details open>
  <summary>2024-11-08</summary>
  
  * The default UI has been migrated from probe_basic to qtdragon_hd. 
  * Inverted 'not' pins have been added to the digital inputs to simplify HAL configuration. The format is '[InputName].not'. They are visible in halshow for setup/debugging your configuration.
  * Pi 5 support has been added to the Flexi component via [rp1lib](https://github.com/scottalford75/rp1lib)
  * Both the firmware and component have been updated to pass raw counts between the Remora firmware and the component via SPI rather than the DDS value. 
  * The deadband implementation has been reworked in the component; deadband is no longer added to the error, and instead zeros error and skips calculating velocity command updates while within the deadband. This allows for higher FF1 gains without hunting, and makes it possible to more easily tune the following error to the expected amount within 1 servo period. **If you had previously tuned your gains using the previous firmware, you will need to tweak them with the new component.**
  * The reference config has been updated for our new Bookworm based Pi images, which have different mapping of the UARTs for the Pi 4. 
  * Scripts have been added for flashing the Flexi-HAL with the Pi 5. The Pi images will have the correct variant pre-installed.
</details>




## Flexi-Pi

The Flexi-Pi images have been migrated to a seperate repository to make them more maintainable. You can find them here: https://github.com/Expatria-Technologies/Flexi-Pi




## Flexi-HAL Configuration
Jumpers need to be in place in the marked locations on the Flexi-HAL for the Pi to communicate with the MCU for firmware flashing:

<img src="/Images/Jumper_locations.png" width="500">

These are shipped in place by default from the Expatria shop, but if they have been moved or removed they will need to be replaced. 


## Reference config
A reference config is included in the [LinuxCNC/ConfigSamples/flexi-hal](./LinuxCNC/ConfigSamples/flexi-hal) directory. **This will need to be edited for your specific machine.** 

There are VFD options in the reference config for either vfdmod (configured for a Durapulse GS10 here), and hy_vfd (for Huanyang VFDs). If using hy_vfd, the vfdmod items will need to be commented out, and the hy_vfd items uncommented in [remora-flexi.hal](./LinuxCNC/ConfigSamples/flexi-hal/remora-flexi.hal) and [qtdragon_hd](./LinuxCNC/ConfigSamples/flexi-hal/qtdragon_hd.hal).

Machine travels, limits, and other configuration is done in [remora-flexi.ini](./LinuxCNC/ConfigSamples/flexi-hal/remora-flexi.ini).

Additional reference configs are posted here:   
https://github.com/Expatria-Technologies/linuxcnc_configs
