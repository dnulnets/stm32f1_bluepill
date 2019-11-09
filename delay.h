/*
** Delay - Delay functions based on the system ticks
**
** Written by Tomas Stenlund, Stenlund Open Source Group 2019
**
** Exports functions to handle delays
**
*/
#pragma once

void delay_init(void);
void delay_ticks(uint32_t ticks);
uint32_t tickcount(void);
