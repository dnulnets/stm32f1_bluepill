/*
** SIO - Serial IO using the USB CDC ACM device
**
** Written by Tomas Stenlund, Stenlund Open Source Group 2019
**
** Exports functions to handle serial io.
**
*/
#pragma once

#define SIO_EOF (-1)

/* Writing functions */
void sio_write(char *buf);
void sio_write_hex8 (uint8_t h);
void sio_write_hex16 (uint16_t h);
void sio_write_hex32 (uint32_t h);
void sio_write_dec8 (uint8_t h);
void sio_write_dec16 (uint16_t h);
void sio_write_dec32 (uint32_t h);

/* Reading functions */
void sio_readln(char *buf, int size);
int sio_getch(void);
