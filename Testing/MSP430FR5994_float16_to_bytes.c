#include <msp430.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

// received update message
char msg[20] = {0};
// current position of received update message
// -1 indicate empty and no message coming
// 0 indicate empty and message coming
// received update message buffer only do replacement,
// not going to empty after use, only set index to indicate state
int len = -1;

/* _q15 refer to: https://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/DSPLib/latest/exports/html/index.html
 * float16 to byte and reverse: https://stackoverflow.com/questions/25013956/float-16-bits-to-byte */
void decode()
{
    // n_weight0_1[INPUT][LAYER1][LENGTH_KERNEL][LENGTH_KERNEL]
    // n_weight2_3[LAYER2][LAYER3][LENGTH_KERNEL][LENGTH_KERNEL]
    // n_weight5_6[LAYER5 * LENGTH_FEATURE5 * LENGTH_FEATURE5][OUTPUT]
    // weight4_5[2][LAYER5][LENGTH_KERNEL][LENGTH_KERNEL]
    // weight4_5_1[2][LAYER5][LENGTH_KERNEL][LENGTH_KERNEL]
    // weight4_5_2[2][LAYER5][LENGTH_KERNEL][LENGTH_KERNEL]
    // weight4_5_3[2][LAYER5][LENGTH_KERNEL][LENGTH_KERNEL]
    // weight4_5_4[2][LAYER5][LENGTH_KERNEL][LENGTH_KERNEL]
    // weight4_5_5[2][LAYER5][LENGTH_KERNEL][LENGTH_KERNEL]
    // weight4_5_6[2][LAYER5][LENGTH_KERNEL][LENGTH_KERNEL]
    // weight4_5_7[2][LAYER5][LENGTH_KERNEL][LENGTH_KERNEL]
    // n_bias0_1[LAYER1]
    // n_bias2_3[LAYER3]
    // n_bias4_5[120]
    // n_bias5_6[OUTPUT]
    // f_input[INPUT][LENGTH_FEATURE0][LENGTH_FEATURE0]
    // f_output[OUTPUT]
    // f_layer1[LAYER1][LENGTH_FEATURE1][LENGTH_FEATURE1]
    // f_layer2[LAYER2][LENGTH_FEATURE2][LENGTH_FEATURE2]
    // f_layer3[LAYER3][LENGTH_FEATURE3][LENGTH_FEATURE3]
    // f_layer4[LAYER4][LENGTH_FEATURE4][LENGTH_FEATURE4]
    // f_layer5[LAYER5][LENGTH_FEATURE5][LENGTH_FEATURE5]

    // update weight0_1[1][6][5][5]
    //P1OUT ^= BIT0;                      // Toggle Red LED
    //P1OUT ^= BIT1;                      // Toggle Green LED

    int i;
    for (i=0; i<len; i+=2)
    {
        int16_t weight = (int16_t) (msg[i] << 8) | (int16_t) (msg[i+1] & 0x00ff);
        if (converted == 4045)
        {
            P1OUT ^= BIT1;
        }
    }
}

int main(void)
{
     WDTCTL = WDTPW | WDTHOLD;               // Stop Watchdog

    // Configure GPIO
    P6SEL1 &= ~(BIT0 | BIT1);
    P6SEL0 |= (BIT0 | BIT1);                // USCI_A3 UART operation
    // Configure LED GPIO
    P1OUT &= ~BIT0;                         // Clear P1.0 output latch for a defined power-on state
    P1OUT &= ~BIT1;                         // Clear P1.1 output latch for a defined power-on state
    P1DIR |= BIT0;                          // Set P1.0 to output direction
    P1DIR |= BIT1;                          // Set P1.1 to output direction

    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

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


    int16_t value = 4045; //4045
    int8_t b1 = (int8_t) ((value >> 8) & 0x00ff);
    int8_t b2 = (int8_t) (value & 0x00ff);

    if (b1 == (int8_t)(15))
    {
        P1OUT ^= BIT0;
    }
    if (b2 == (int8_t)(-51))
    {
        P1OUT ^= BIT1;
    }
    int16_t converted = (int16_t) (b1 << 8) | (int16_t) (b2 & 0x00ff);
//    if (value == converted)
//    {
//        P1OUT ^= BIT0;                        // Toggle Red LED
//        P1OUT ^= BIT1;                      // Toggle Red LED
//
//    }
//    if (converted == 4045)
//    {
//        P1OUT ^= BIT1;
//    }
    _BIS_SR(GIE); // Activate interrupts previously enabled
    //__ z bis_SR_register(LPM3_bits | GIE);     // Enter LPM3, interrupts enabled


    while (1)
    {
        __no_operation();                       // For debugger
    }


}

/* Message transfer within the system from CC2640R2F to MSP430FR5994 using UART.
 * '{'(without single quotation) represent start,
 * '}'(without single quotation) represent end.
 */
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
            // assume '{' and '}' only exists at the beginning and the end
            if (UCA3RXBUF == '{')
            {
                len = 0;
            }
            else if (UCA3RXBUF == '}')
            {
                decode();
            }
            else
            {
                msg[len++] = UCA3RXBUF;
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

