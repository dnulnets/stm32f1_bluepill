/*
** SIO - Serial IO, uses USB
**
** Written by Tomas Stenlund, Stenlund Open Source Group
**
** Contains simple functions for reading and writing to serial USB device.
**
*/

/* Standard C library */
#include <stdint.h>
#include <string.h>

/* My own libraries */
#include "usb.h"
#include "sio.h"

void sio_write(char *buf)
{
    usb_send  (buf, strlen(buf));
}

void sio_readln(char *buf, int size)
{
    usb_readln (buf, size);
}
