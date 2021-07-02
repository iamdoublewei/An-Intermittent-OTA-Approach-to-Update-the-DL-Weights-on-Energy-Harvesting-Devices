/*
 * Copyright (c) 2015-2019, Texas Instruments Incorporated
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
 */

/*
 *  ======== uartecho.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/NVS.h>
#include <ti/sysbios/hal/Seconds.h>

/* Example/Board Header files */
#include "Board.h"

/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON0.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    /* Clear the GPIO interrupt and toggle an LED */
    // GPIO_toggle(Board_GPIO_LED0);
    unsigned int ledPinValue = GPIO_read(Board_GPIO_LED0);
    GPIO_write(Board_GPIO_LED0, (ledPinValue == Board_GPIO_LED_ON) ? Board_GPIO_LED_OFF : Board_GPIO_LED_ON);
}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on Board_GPIO_BUTTON1.
 *  This may not be used for all boards.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    /* Clear the GPIO interrupt and toggle an LED */
    //GPIO_toggle(Board_GPIO_LED1);
    unsigned int ledPinValue = GPIO_read(Board_GPIO_LED1);
    GPIO_write(Board_GPIO_LED1, (ledPinValue == Board_GPIO_LED_ON) ? Board_GPIO_LED_OFF : Board_GPIO_LED_ON);
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    // UART params
    UART_Handle uart;
    UART_Params uartParams;
    // NVS params
    NVS_Handle nvsHandle;
    NVS_Attrs regionAttrs;
    NVS_Params nvsParams;
    // led signals
    unsigned int ledPinValue0;
    unsigned int ledPinValue1;
    // unix time
    UInt32 t;

    /* Call driver init functions */
    GPIO_init();
    UART_init();
    NVS_init();

    /* Configure the LED and button pins */
    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(Board_GPIO_LED1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(Board_GPIO_BUTTON0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);
    GPIO_setConfig(Board_GPIO_BUTTON1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);
    GPIO_write(Board_GPIO_LED1, Board_GPIO_LED_ON);

    /* install Button callback */
    GPIO_setCallback(Board_GPIO_BUTTON0, gpioButtonFxn0);
    GPIO_setCallback(Board_GPIO_BUTTON1, gpioButtonFxn1);

    /* Enable interrupts */
    GPIO_enableInt(Board_GPIO_BUTTON0);
    GPIO_enableInt(Board_GPIO_BUTTON1);

    /* Create a UART with data processing off. */
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 115200;

    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }

    NVS_Params_init(&nvsParams);
    nvsHandle = NVS_open(Board_NVSINTERNAL, &nvsParams);

    if (nvsHandle == NULL) {
        /* NVS_open() failed. */
        while (1);
    }

    /*
     * This will populate a NVS_Attrs structure with properties specific
     * to a NVS_Handle such as region base address, region size,
     * and sector size.
     */
    NVS_getAttrs(nvsHandle, &regionAttrs);

    /* set to today’s date in seconds since Jan 1, 1970 */
    Seconds_set(1597757956); // refer to https://www.epochconverter.com/

    /* Loop forever echoing */
    while (1) {
        /* retrieve current time relative to Jan 1, 1970 */
        t = Seconds_get();
        // output format: epoch_time, led0_state, led1_state
        // epoch time always 10 digits
        char data[] = {'0','0','0','0','0', '0', '0', '0', '0', '0', ',', '0', ',', '0'};
        int i;
        for (i=0; i<10; i++){
            data[9 - i] = t % 10 + '0';
            t /= 10;
        }

        // read leds
        ledPinValue0 = GPIO_read(Board_GPIO_LED0);
        ledPinValue1 = GPIO_read(Board_GPIO_LED1);
        data[11] = ledPinValue0 ? '1' : '0';
        data[13] = ledPinValue1 ? '1' : '0';

        // write and read from flash
        NVS_write(nvsHandle, 0, (void *) data, sizeof(data),
            NVS_WRITE_ERASE | NVS_WRITE_POST_VERIFY);
        // Buffer placed in RAM to hold bytes read from non-volatile storage.
        char buffer[14];
        NVS_read(nvsHandle, 0, (void *) buffer, sizeof(buffer));

        // write to serial
        UART_write(uart, buffer, sizeof(buffer));

        sleep(1);
    }
}
