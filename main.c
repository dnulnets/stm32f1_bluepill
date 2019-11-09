/*
** Main - USB Communication Device Example
**
** Written by Tomas Stenlund, Stenlund Open Source Group, 2019
**
** Example code to demonstrate serial io with a USB CDC ACM.
**
*/

/* libopencm3 libs */
#include <libopencm3/stm32/rcc.h>

/* My own libs */
#include "delay.h"
#include "usb.h"
#include "sio.h"

/*
** The main
*/
int main(void)
{
    char buf[100];

    /* Set up system clocks and counters */
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
    rcc_periph_clock_enable(RCC_GPIOA);

    /* Init system tick counter */
    delay_init();

    /* Init the USB */
    usb_init ();

    /* Wait for connection */
    usb_wait_for_connection();
    sio_write ("STM32F Started\r\n");

    /* Simple loop */
    uint16_t n = 0;
    while (true) {
        sio_write ("Enter something : ");
        sio_readln (buf, 100);
        sio_write ("\r\nWe got '");
        sio_write (buf);
        sio_write ("'\r\n");
        sio_write_hex16 (n);
        sio_write ("\r\n");
        n++;
    }
}
