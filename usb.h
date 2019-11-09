/*
** USB CDC (ACM) - USB Communication Device Class (Abstract Control Model)
**
** Written by Tomas Stenlund, Stenlund Open Source Group 2019
**
** Exports functions to handle USB CDC ADM.
**
*/
#pragma once

#define USB_EOF (-1)

void usb_send(const void *data, uint32_t len);
void usb_readln(void *data, uint32_t len);
void usb_init(void);
void usb_wait_for_connection (void);
int usb_getch(void);
