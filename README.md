
# Overview

- Purpose: This project is used for paper experiment
- Paper: [An Intermittent OTA Approach to Update the DL Weights on Energy Harvesting Devices](https://ieeexplore.ieee.org/abstract/document/9806295)
- Published Conference: ISQED 2022
- Description: The energy harvesting IoT device receives Bluetooth Low Energy (BLE) over-the-air (OTA) update packages from edge server, and then decode and update the optimized LeNet-5 machine learning model implemented in the IoT device.


## Hardware Requirements

 -  [CC2640R2F](https://www.ti.com/product/CC2640R2F) - BLE communication
 -  [MSP430FR5994](https://www.ti.com/product/MSP430FR5994) - LeNet-5 Machine Learning Inference
 -  Raspberry Pi 4 - Edge Server
 -  SIGLENT SDG1032X - Energy Harvesting Power Supply Simulator

## Software Requirements

 -  Windows OS
 -  [CCS](https://www.ti.com/tool/CCSTUDIO)
 
## Contents

-  ./Implementation/CC2640R2F - BLE module in CC2640R2F TI board to receive BLE update packages from edge server, and then send the update packages from UART communication to MSP430FR5994 which runs the LeNet-5 machine learning inference.
- ./Implementation/MSP430FR5994 - LeNet-5 inference module in MSP430FR5994 TI board, running periodical inference tasks. This deice is able to run the machine learning inference tasks and model updates intermittently. 
-  ./Measurements - Energy consumption measurements of some particular tasks.
- ./Testing - Basic function testing for different tasks.
- ./Data - data and references for this project.
