/*
** Main - USB Communication Device Example
**
** Written by Tomas Stenlund, Stenlund Open Source Group, 2019
**
** Example code to demonstrate serioal io with a USB CDC ACM.
**
*/

/* libopencm3 library */
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

/* My own stuff */
#include "delay.h"

/* Locl tick counter */
static volatile uint32_t tickcnt;

/*
** Init the system tick
*/
void delay_init(void)
{
	tickcnt = 0;
	systick_set_frequency(1000, rcc_ahb_frequency);
	systick_counter_enable();
	systick_interrupt_enable();
}

/*
** Wait for a number of ticks befor returning
*/
void delay_ticks(volatile uint32_t ticks)
{
	volatile uint32_t start = tickcnt;

	while (tickcnt - start < ticks)
	{
		__asm__("nop");
	}
}

/*
** Returns with the number of ticks
*/
uint32_t tickcount(void)
{
	return tickcnt;
}

/*
** The system tick interrupt handler
*/
void sys_tick_handler(void)
{
	tickcnt++;
}
