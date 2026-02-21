# Remora-STM32F4xx-PIO
Port of Remora for STM32F4xx family of MCUs, using new Remora-Core abstraction.

# Todos / Status
- JSON config loading - Working - no known issues
- Ethernet Comms - in progress:    
    - Working, no issues experienced yet
- SPI Comms - in progress:
    - Able to sync with LinuxCNC Component at full speed with an RPi5
    - Working but see issue below about follower errors during rapids. 
- UART support - Working on both UART2 and UART3 peripherals, can be set in PlatformIO.ini for your build target. 
- SDIO - Working - Tested on F446ZE and F446RE, able to load config from micro SD card for SPI builds.
- Modules - in progress:
    - Blinky: Working - no known issues.
    - Stepgen: Working - no known issues.
    - Digital IO: Working - no known issues.
    - Analog Ins: Working - no known issues. 
    - Hardware PWM: Working - no known issues.
    - Software PWM: To be ported in a future revision.
    - Software encoders: Working - no known issues
    - QEI: Working as expected - however speed testing have not yet tested for speed
- Linker scripts
    - F446RE - Working - no known issues.
    - F446ZE - Working - no known issues.
    - Octopus - I don't have one of these boards, but I can upload the BTT bootloader onto an STM32 and upload and run firmware successfully
    - Fysect Spider - Will endeavour to set up a build option for this. 
- Adhoc todos: 
    - Set up status LED as definable in platformIO.ini
    
# Build instructions
- In a new directory:
    - git clone https://github.com/ben-jacobson/Remora-STM32F4xx-PIO .
- Move to the src/remora-core directory
    - cd src/remora-core
    - git clone https://github.com/ben-jacobson/remora-core .
- Various in-dev features are available in branches for our fork of this repo
    - git checkout *[feature_branch]*
- Use Platform IO to select the build target and build/upload/debug.
    - If you want to use the debugger to step through code, set *default_envs* in platformio.ini to the build target of your choice 

# For communication via Wiznet W5500 module
- SCK: PA_5
- MISO: PA_6
- MOSI: PA_7
- WIZ_RST: configured in Platformio.ini. Default is PB_5
- SPI_CS: configured in Platformio.ini. Default is PA_4

# For communication via Raspberry Pi SPI
- SCK: PA_5
- MISO: PA_6
- MOSI: PA_7
- SPI_CS: configured in Platformio.ini. Default is PA_4

# UART connections
- UART_TX: 
    - F446RE: PA_2
    - F446ZE: PD_8
- UART_RX: - Firmware doesn't make use of RX, but left for completeness and future expansion
    - F446RE: PA_3
    - F446ZE: PD_9

# SD card reader connections
- SDIO_D0: PC_8
- SDIO_D1: PC_9   
- SDIO_D2: PC_10  
- SDIO_D3: PC_11  
- SDIO_CLK: PC_12  
- SDIO_CMD: PD_2  

# Quadrature Encoder Interface (QEI)
Remora has a dedicated hardware quadrature encoder module useful for high speed applications such as spindles or very high resolution encoders. Please note that this uses specific hardwired pins and two channels of one of the timer interfaces which may interfere with other features such as PWM outputs or analog inputs. These pins will not be effected if you do not include QEI in your config.txt 
- CHA: PC_6
- CHB: PC_7
- Z/Index: PA_8

# Allocation of Step Generators, IO and PWM
Please refer to the Remora documentation to configure GPIO to perform various functions like stepgen, digital IO and PWM: https://remora-docs.readthedocs.io/en/latest/configuration/configuration.html
Example config.txt files can be found in the LinuxCNC_Configs folder. 

# Analog Inputs
You may use the ADCs to read values from analog pins, this is useful for speed and feed override potentiomters. Have tested this only on PA_2 and PA_3 but theoretically all ADC enabled pins should work, provided they don't clash with other peripherals. It may allow for more than two, this is untested.

# Hardware PWM
Hardware PWM is available on a wide variety of pins depending on your hardware target. When setting up your config.txt file, you must choose a PWM enabled pin from the list provided. Specific STM32 Timers and Channels will been allocated by the driver automatically. Some important details about this: 
- PWM pins can be set to variable or fixed period. Config.txt documentation can be found here https://remora-docs.readthedocs.io/en/latest/configuration/Setup-Config-File.html#pwm
- You may set up more than one PWM pin on the same timer (TIMx) as each pin is assigned a different channel, however sharing TIMx's has some interesting nuances you must be aware of:
    - The period setting is shared across that TIMx so changing any of these will effect all four channels. Since set point variables are persistent between sessions, the initial period setting will not be clearly defined unless you set them yourself in LinuxCNC.  
    - Due to above, setting some PWM pins on the same TIMx with variable period and others with fixed period somewhat defeats its own purpose. The PRU will ignore changes to the fixed pin but changes to any variable period will change all channels on that TIMx whether they are variable or not. Do note that we've been careful to force recalculation of the duty cycle on period change so this may become an immediate problem unless you have a fixed period in your used case.
    - Note that some of the channels below are marked with an N at the end. These can be configured individually like a normal PWM pin or can be paired up as inverted copies of their non-N counterparts. This is great for driving differential signals, e.g. if you configure output on PA_9 (TIM1_CH2) and PB_0 (TIM1_CH2N), PB_0 will mirror PA_9 with flipped polarity. Note that when used in pairing mode, the duty cycle is locked to the non-N pin meaning you won't be able to alter the duty cycle of any N pins independantly, you must alter the duty cycle of the non-N pin for control. Variable period works fine for these and they stay in sync.
- If you don't specify a fixed period in your config.txt, or if your LinuxCNC intialises this as zero, the default will become 200us unless overwritten by the set point variable in LinuxCNC
- Without a linuxCNC config controlling duty cycle via set point variable, the PWM will automatically starts as soon as you come out of eStop which could be dangerous. You will need to configure LinuxCNC to stop and start on the conditions you want. For example, please take caution to initialise zero pulse width for 0-10v Spindle control. HAL config can be found here: https://remora-docs.readthedocs.io/en/latest/software/hal-examples.html#pwm-to-0-10v-spindle-control-simple
- How many PWM pins available will be limited by your remora-eth-3.0 component, which defaults to 6. You can raise this limit by changing both remora-eth-3.h file and configuration.h file, but be careful that it fits within the data allocated for RX packets. You will need 1 variable for each fixed period PWM pin, or 2 for variable duty PWM.
- All PWM timers are either 16 or 32 bits wide depending on which TIMx is used. Either is more than enough for very fine control over duty cycle.
- Be very careful not to clash with other peripherals such as SPI or UART, this will result in undefined behaviour. 

PWM compatible pins for smaller F446xx target are:
| Pin   | Timer | Channel | Notes                    | Tested?     |
|-------|-------|---------|--------------------------|-------------|
| PA_8  | TIM1  | CH1     | Will clash if using QEI  | Working     |
| PA_9  | TIM1  | CH2     | Will clash if using QEI  | Working     |
| PA_10 | TIM1  | CH3     |                          | Working     |
| PA_11 | TIM1  | CH4     |                          | Working     |
| PB_0  | TIM1  | CH2N    | Inverted PA_9            | Working     |
| PB_1  | TIM1  | CH3N    | Inverted PA_10           | Working     |
| PB_6  | TIM4  | CH1     |                          | Working     |
| PB_7  | TIM4  | CH2     |                          | Working     |
| PB_13 | TIM1  | CH1N    | Inverted PA_8            | Working     |
| PB_14 | TIM1  | CH2N    | Inverted PA_9            | Working     |
| PB_15 | TIM1  | CH3N    | Inverted PA_10           | Working     |

Additional pins for larger F4 boards like the F446ZE
| Pin   | Timer | Channel | Notes                    | Tested?     |
|-------|-------|---------|--------------------------|-------------|
| PD_12 | TIM4  | CH1     |                          | Working     |
| PD_13 | TIM4  | CH2     |                          | Untested    |
| PD_14 | TIM4  | CH3     |                          | Untested    |
| PD_15 | TIM4  | CH4     |                          | Untested    |
| PE_5  | TIM9  | CH1     |                          | Untested    |
| PE_6  | TIM9  | CH2     |                          | Untested    |
| PE_8  | TIM1  | CH1N    | Inverted PE_9            | Untested    |
| PE_9  | TIM1  | CH1     |                          | Untested    |
| PE_10 | TIM1  | CH2N    | Inverted PE_11           | Untested    |
| PE_11 | TIM1  | CH2     |                          | Untested    |
| PE_12 | TIM1  | CH3N    | Inverted PE_13           | Untested    |
| PE_13 | TIM1  | CH3     |                          | Untested    |
| PE_14 | TIM1  | CH4     |                          | Untested    |
| PF_6  | TIM10 | CH1     |                          | Untested    |
| PF_7  | TIM11 | CH1     |                          | Untested    |
| PF_8  | TIM13 | CH1     |                          | Untested    |
| PF_9  | TIM14 | CH1     |                          | Untested    |
 
# Boards
- Nucleo F446RE: In development
- Nucleo F446ZE: In development
- Octopus 446: To be tested

------------------------------------------

# Installation instructions
You may build the firmware from source using PlatformIO. On initial release, precompiled binaries will be made available in this repo enabling upload via an STLink. 

This firmware uses the Remora-eth-0.3.0 ethernet component avaialable in the LinuxCNC/Components folder.

Compile the component using halcompile
```
sudo halcompile --install remora-eth-3.0.c
```

Configs are loaded via tftpy, on release a config.txt file will be made available which you can upload via an upload_config.py script.
```
pip3 install tftpy # If not using virtualenv you may get an error about breaking system packages, use the --break-system-packages flag if needed
python3 upload_config.py NucleoF411RE-Config.txt
```

Refer to the remora documents for more information
https://remora-docs.readthedocs.io/en/latest/firmware/ethernet-config.html

Board will not start until ethernet connection is established. 

Credits to Scotta and Cakeslob and others that worked on Remora. Additional credit to Expatria Technologies and Terje IO. 

# Known issues and assorted notes
- Using the SPI version of this firmware does work and have tested with a Raspberry Pi 5 with 8Gb of RAM. However, am seeing following errors during rapid movements. Cannot be sure if this is something to do with our SPI code, maybe the RPi isn't up to the task or could also be an issue with the SPI component. The workaround is to either lower max velocity of rapid moves, or raise the ferror value. Unsure if this is indicative of a bigger problem, more testing is required. The Ethernet config has been tested on a full sized PC, but a good test could be to connect via Ethernet from an RPi4 or 5 to see if the issue can be replicated there too. 
- When using the SPI comms interface, the EXTI4 is not truly configurable despite it being settable in platformio.ini. Some handlers in irqHandler.h have this hard coded in as GPIO_4, changing this may break the comms interface. Use of EXT4 (PA_4 CS line) with other SPI2/3 is yet to be tested.
- Noticed that STMHal makes heavy use of lock objects, not sure if keeping generic HAL Handlers as class members is going to work long term. See the Hardware PWM HAL code for more info on limitations, ideally each should be broken out as global objects so that the class can allocate shared resources. This issue may also creep up later with shared SPI and other handlers later on down the track
- Lost packet detection in the W5500 Networking drivers only checks if new packets are loaded before handed over to the PRU. This works for now but could be improved later with a status check of Remora. Have not noticed any issues with lost packets, just thinking ahead for example if there was ever a need to run the base thread faster than the recommended 40Khz. 
