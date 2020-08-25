# cc2640r2_init Overview

Test Texas Instrument CC2640R2 launchpad for future research purpose.


## Hardware Requirements

 -  [ LAUNCHXL-CC2640R2](https://www.ti.com/tool/LAUNCHXL-CC2640R2)
 - Computer

## Software Requirements

 - Windows OS
 - [CCS](https://www.ti.com/tool/CCSTUDIO)
 - [CC2640R2 SDK](https://www.ti.com/tool/SIMPLELINK-CC2640R2-SDK)

## Installation

 - [Install CCS](https://www.ti.com/tool/download/CCSTUDIO)
 - [Install CC2640R2 SDK](https://www.ti.com/tool/download/SIMPLELINK-CC2640R2-SDK)
 - Setup XDCTool version to match up with SDK codes
 - Open CCS
 - Select Windows -> Preferences
 - Select Code Composer Studio -> Products
 - Rediscover Installed Products, make sure XDCtools 3.51.3.28 is installed
 
## Import and Run

 - Open Resource Explorer
  - Import SimpleLink CC2640R2 SDK - v:4.20.00.04/TI Drivers/uartecho project
  - Clone this repository into local computer
  - Replace uartecho/uartecho.c in CCS with uartecho.c in this repository
  - Connect CC2640R2 to computer
  - Debug and run uartecho project in CCS
  - In main.py change serial port number as device shown on your local computer
  - Run main.py

## Features

 - serial_port.py display all available serial ports on the computer
 - Output (device_unixtime, led0_state, led1_state) from CC2640R2 to UART
 - Store output data into file based database
 - Write and read data from internal flash

