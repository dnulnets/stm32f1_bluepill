/*
** SIO - Serial IO, uses USB
**
** Written by Tomas Stenlund, Stenlund Open Source Group
**
** Contains simple functions for reading and writing to serial USB device. It is not thread
** safe.
**
*/

/* Standard C library */
#include <stdint.h>
#include <string.h>

/* My own libraries */
#include "usb.h"
#include "sio.h"

/* HEX array */
static char *hex = "0123456789ABCDEF";

/* Conversion buffer */
#define SIO_BUFFER_SIZE 32
static char sio_buffer[SIO_BUFFER_SIZE];

void sio_write(char *buf)
{
    usb_send  (buf, strlen(buf));
}

void sio_readln(char *buf, int size)
{
    usb_readln (buf, size);
}

int sio_getch(void)
{
    int ch = usb_getch();
    if (ch == USB_EOF)
        ch = SIO_EOF;
    return ch;
}

static void _sio_write_hex8 (uint8_t h, int ix)
{
    sio_buffer[ix] = hex[(h & 0xf0)>>4];
    sio_buffer[ix+1] = hex[h & 0x0f];
}

void sio_write_hex8 (uint8_t h)
{
    _sio_write_hex8 (h, 0);
    usb_send (sio_buffer, 2);
}

void sio_write_hex16 (uint16_t h)
{
    _sio_write_hex8 ((h>>8)&0xff, 0);
    _sio_write_hex8 (h&0xff, 2);
    usb_send (sio_buffer, 4);
}

void sio_write_hex32 (uint32_t h)
{
    _sio_write_hex8 ((h>>24)&0xff, 0);
    _sio_write_hex8 ((h>>16)&0xff, 2);
    _sio_write_hex8 ((h>>8)&0xff, 4);
    _sio_write_hex8 (h&0xff, 6);
    usb_send (sio_buffer, 8);
}

void sio_write_dec8 (uint8_t h)
{
    sio_write_dec32 (h);
}

void sio_write_dec16 (uint16_t h)
{
    sio_write_dec32 (h);
}

void sio_write_dec32 (uint32_t h)
{
    uint8_t n = SIO_BUFFER_SIZE;

    while (h>0) {
        n--;
        sio_buffer[n] = (char)(48 + (h % 10));
        h = h / 10;
    }

    if (n == SIO_BUFFER_SIZE-1) {
        n--;
        sio_buffer[n] = '0';
    }

    usb_send (sio_buffer+n, SIO_BUFFER_SIZE-n);
}

