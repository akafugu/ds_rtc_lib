/*
 * DS RTC Library: DS1307 and DS3231 driver library
 * (C) 2011 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

#include <stdio.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "../twi.h"
#include "../rtc.h"

#include "uart.h"

#define LED_BIT PB5
#define LED_DDR DDRB
#define LED_PORT PORTB
#define LED_HIGH LED_PORT |= _BV(LED_BIT)
#define LED_HIGH LED_PORT |= _BV(LED_BIT)
#define LED_LOW  LED_PORT &= ~(_BV(LED_BIT))

void read_rtc(void)
{
	if(rtc_is_ds3231()) {
		int8_t t;
		uint8_t f;
		ds3231_get_temp_int(&t, &f);
	}
	else {
		uint8_t hour, min, sec;
		rtc_get_time_s(&hour, &min, &sec);	
	}
}

void main(void) __attribute__ ((noreturn));

void main(void)
{
	struct tm* t = NULL;
	char buf[32];
	uint8_t hour, min, sec;

	uartInit();
	uartSetBaudRate(9600);
	uartSendString("DS RTC Library Test\n");

	LED_DDR  |= _BV(LED_BIT); // indicator led

	for (int i = 0; i < 5; i++) {
		LED_HIGH;
		_delay_ms(100);
		LED_LOW;
		_delay_ms(100);
	}

	uartSendString("Before Init\n");
	twi_init_master();
	rtc_init();
	rtc_set_time_s(12, 0, 50);

	uartSendString("After Init\n");
	if (rtc_is_ds1307())
		uartSendString("DS1307\n");
	else
		uartSendString("DS3231\n");

	rtc_set_alarm_s(12, 1, 0);
	
	rtc_get_alarm_s(&hour, &min, &sec);

	sprintf(buf, "Alarm is set -%d:%d:%d-\n", hour, min, sec);
	uartSendString(buf);	
	uartSendString("---\n");
	uartSendString("---\n");
	uartSendString("---\n");

	while (1) {
		t = rtc_get_time();

		sprintf(buf, "%d:%d:%d\n", t->hour, t->min, t->sec);
		uartSendString(buf);
		uartSendString("---\n");

		if (rtc_check_alarm())
			uartSendString("ALARM!\n");

	   	_delay_ms(500);
	}
}
