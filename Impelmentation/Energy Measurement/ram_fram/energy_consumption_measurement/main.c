#include <msp430.h> 
#include <stdint.h>

#pragma PERSISTENT(fram_buff)
uint16_t fram_buff = 0;

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	PM5CTL0 &= ~LOCKLPM5;

	//uint16_t ram_buff;
	uint32_t i;

//    __no_operation();                       // For debugger
//
//	for (i=0; i<10000000; i++) //10000000 = 59s
//	{
//	    ram_buff = i;
//	}
    __no_operation();                       // For debugger

    for (i=0; i<10000000; i++) //10000000 = 90s
    {
        fram_buff = i;
    }
    __no_operation();                       // For debugger
    __bis_SR_register(LPM4_bits);

	return 0;
}
