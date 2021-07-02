/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2015, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430FR5x9x Demo - eUSCI_A3 UART echo at 9600 baud using BRCLK = 8MHz
//
//  Description: This demo echoes back characters received via a PC serial port.
//  SMCLK/ DCO is used as a clock source and the device is put in LPM3
//  The auto-clock enable feature is used by the eUSCI and SMCLK is turned off
//  when the UART is idle and turned on when a receive edge is detected.
//  Note that level shifter hardware is needed to shift between RS232 and MSP
//  voltage levels.
//
//  The example code shows proper initialization of registers
//  and interrupts to receive and transmit data.
//  To test code in LPM3, disconnect the debugger.
//
//  ACLK = VLO, MCLK =  DCO = SMCLK = 8MHz
//
//                MSP430FR5994
//             -----------------
//       RST -|     P6.0/UCA3TXD|----> PC (echo)
//            |                 |
//            |                 |
//            |     P6.1/UCA3RXD|<---- PC
//            |                 |
//
//   William Goh
//   Texas Instruments Inc.
//   October 2015
//   Built with IAR Embedded Workbench V6.30 & Code Composer Studio V6.1
//******************************************************************************
/* _q15 refer to: https://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/DSPLib/latest/exports/html/index.html
 * float16 to byte and reverse: https://stackoverflow.com/questions/25013956/float-16-bits-to-byte
 */
#include <msp430.h>
#include "lenet.h"
//#include "model.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

//#define CS_8MHZ             0
//#define CS_16MHZ            1
/* Weight matrices:
 * n_weight0_1[1][6][5][5] = 150
 * n_weight2_3[6][16][5][5] = 2400
 * weight4_5[512] 512
 * weight5_6[10][256] = 2560
 * total weights = 5622
 * */
#define MATRIX_SIZE         5622
#define PACKET_SIZE         236

short int i,percent,j,predict;
uint8 left;

image *test_data={0};
uint8 *test_label={0};

char packet[PACKET_SIZE];
int len = 0;
// decoding format: the number of sections, (index + indicator)(bits), length(bits)
// this information will be updated when the IoT device receives update notification
char format[3] = {7, 11, 4};

void DecodeUpdate()
{
    uint16_t max_index = ceil((float)MATRIX_SIZE / format[0]);
    uint8_t section = 0; // section number
    uint16_t index = 0; // index number
    uint16_t length = 0; // data length
    uint16_t cur = 0; // current data point position within a block
    int i = 0;

    while (i < len)
    {
        if (length == 0)
        {
            // check indicator bit
            // indicator: 0 section; 1 block
            if(packet[i] / (uint8_t)pow(2, 7) == 0)
            {
                // indicator bit is 0
                section = packet[i];
                i++;
            }
            else
            {
                uint16_t block = (uint16_t) (packet[i] << 8) | (uint16_t) (packet[i+1] & 0x00ff);
                // remove indicator bit = 1
                index = (uint16_t)(block << 1) >> format[2] + 1;
                length = (uint16_t)(block << format[1]) >> format[1];
                i += 2;
            }
        }
        else
        {
            int16_t point = (int16_t) (packet[i] << 8) | (int16_t) (packet[i+1] & 0x00ff);
            uint16_t pos = max_index * section + index + cur;
            Update(pos, point);

            if (cur == length - 1)
            {
                cur = 0;
                length = 0;
            }
            else
            {
                cur++;
            }
            i += 2;
        }
    }
}

void TestFirstImage()
{

    left = 7;
    predict = Predict(test_data[0], 10);

    if(left == predict)
    {
        printf("Shauo! prediction milse. Ha ha!");
    }
}


void ConfigClocks(void)
{
#if CS_8MHZ
    //Ensure that there are 0 wait states enabled
    FRCTL0 = FRCTLPW | NWAITS_0;


    // Clock System Setup
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_0;                     // Set DCO to 1MHz

    // Set SMCLK = MCLK = DCO, ACLK = VLOCLK
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;

    // Per Errata CS12 set divider to 4 before changing frequency to
    // prevent out of spec operation from overshoot transient
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;   // Set all corresponding clk sources to divide by 4 for errata
    CSCTL1 = DCOFSEL_6;                     // Set DCO to 8MHz

    // Delay by ~10us to let DCO settle. 60 cycles = 20 cycles buffer + (10us / (1/4MHz))
    __delay_cycles(60);

    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set MCLK  to 8MHz, SMCLK to 1MHz

    CSCTL4 = HFXTOFF + LFXTOFF;             // Cancel external clocks
    CSCTL5 &= ~(HFXTOFFG + LFXTOFFG);

    CSCTL0_H = 0;                           // Lock CS Registers

#endif

#if CS_16MHZ
    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    // Clock System Setup
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_0;                     // Set DCO to 1MHz

    // Set SMCLK = MCLK = DCO, ACLK = VLOCLK
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;

    // Per Errata CS12 set divider to 4 before changing frequency to
    // prevent out of spec operation from overshoot transient
    CSCTL3 = DIVA__4 | DIVS__4 | DIVM__4;   // Set all corresponding clk sources to divide by 4 for errata
    CSCTL1 = DCOFSEL_4 | DCORSEL;           // Set DCO to 16MHz

    // Delay by ~10us to let DCO settle. 60 cycles = 20 cycles buffer + (10us / (1/4MHz))
    __delay_cycles(60);

    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set MCLK to 16MHz, SMCLK to 1MHz

    CSCTL4 = HFXTOFF + LFXTOFF;             // Cancel external clocks
    CSCTL5 &= ~(HFXTOFFG + LFXTOFFG);

    CSCTL0_H = 0;                           // Lock CS Registers
#endif
}

void ConfigGPIO(void)
{
    // Configure ports for low power
    PADIR = 0xFFFF;  PAOUT = 0;
    PBDIR = 0xFFFF;  PBOUT = 0;
    PCDIR = 0xFFFF;  PCOUT = 0;
    PDDIR = 0xFFFF;  PDOUT = 0;
    PJDIR = 0xFFFF;  PJOUT = 0;
    P1DIR = 0xFF;  P1OUT = 0;
    P2DIR = 0xFF;  P2OUT = 0;
    P3DIR = 0xFF;  P3OUT = 0;
    P4DIR = 0xFF;  P4OUT = 0;
    P5DIR = 0xFF;  P5OUT = 0;
    P6DIR = 0xFF;  P6OUT = 0;
    P7DIR = 0xFF;  P7OUT = 0;
    P8DIR = 0xFF;  P8OUT = 0;
}

void ConfigUART(void)
{
    // Configure GPIO
    P6SEL1 &= ~(BIT0 | BIT1);
    P6SEL0 |= (BIT0 | BIT1);                // USCI_A3 UART operation

    // Startup clock system with max DCO setting ~8MHz
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;           // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers
    CSCTL0_H = 0;                           // Lock CS registers

    // Configure USCI_A3 for UART mode
    UCA3CTLW0 = UCSWRST;                    // Put eUSCI in reset
    UCA3CTLW0 |= UCSSEL__SMCLK;             // CLK = SMCLK
    // Baud Rate calculation
    // 8000000/(16*9600) = 52.083
    // Fractional portion = 0.083
    // User's Guide Table 21-4: UCBRSx = 0x04
    // UCBRFx = int ( (52.083-52)*16) = 1
    UCA3BRW = 52;                           // 8000000/16/9600
    UCA3MCTLW |= UCOS16 | UCBRF_1 | 0x4900;
    UCA3CTLW0 &= ~UCSWRST;                  // Initialize eUSCI
    UCA3IE |= UCRXIE;                       // Enable USCI_A3 RX interrupt

    _BIS_SR(GIE); // Activate interrupts previously enabled
    //__bis_SR_register(LPM3_bits | GIE);     // Enter LPM3, interrupts enabled
}

void ConfigLED(void)
{
    // Configure LED GPIO
    P1OUT &= ~BIT0;                         // Clear P1.0 output latch for a defined power-on state
    P1OUT &= ~BIT1;                         // Clear P1.1 output latch for a defined power-on state
    P1DIR |= BIT0;                          // Set P1.0 to output direction
    P1DIR |= BIT1;                          // Set P1.1 to output direction
}

void Test(void)
{
    // case 1(hex): 00,80,02,00,01,00,02
    // n_weight0_1[0][0][0][0] == 1 && n_weight0_1[0][0][0][1] == 2
    // case 2(hex): 03,A8,A1,00,09
    // weight5_6[0][0] = 9
    int res = Check();
    if (res == 1)
    {
        P1OUT ^= BIT1;                      // Toggle LED
    }
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;               // Stop Watchdog

    PM5CTL0 &= ~LOCKLPM5;
    ConfigGPIO();
    ConfigClocks();
    ConfigUART();
    ConfigLED();     // Configure LED for testing

    while(1)
    {
        TestFirstImage(); // about 4s
        __delay_cycles(100000000); //100000000 = 6s
    }

    __no_operation();                       // For debugger
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=EUSCI_A3_VECTOR
__interrupt void USCI_A3_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(EUSCI_A3_VECTOR))) USCI_A3_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(UCA3IV, USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
            while(!(UCA3IFG&UCTXIFG));
            char temp = UCA3RXBUF;
            if (temp == '{')
            {
                len = 0;

            }
            else if (temp == '}')
            {
                DecodeUpdate();
                Test();
            }
            else
            {
                packet[len++] = temp;
            }
            //UCA3TXBUF = UCA3RXBUF;
            __no_operation();
            break;
        case USCI_UART_UCTXIFG: break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
        default: break;
    }
}
