Server: ios app SensorTag
Hardware Config: CC2640R2F received bluetooth info from server, send info through UART to MSP430FR5994, MSP430FR6989 provide power. MSP430FR5994 need to power up by cable first, then can use power provided by MSP430FR6989 board.

PIN Configurations:
CC2640R2F: DIO2 To MSP430FR5994: P6.0
	   DIO3			 P6.1

CC2640R2F
Environment: CCS 9.3.0
Based Example: ble5_project_zero_cc2640r2lp_app
MSP430FR5994
Environment: CCS 10.1.1
Based Example: uart example