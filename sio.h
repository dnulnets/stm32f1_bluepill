/*
** SIO - Serial IO using the USB CDC ACM device
**
** Written by Tomas Stenlund, Stenlund Open Source Group 2019
**
** Exports functions to handle serial io.
**
*/
#pragma once

void sio_write(char *buf);
void sio_readln(char *buf, int size);

