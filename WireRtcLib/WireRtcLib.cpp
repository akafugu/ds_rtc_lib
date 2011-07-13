/*
 * Wire RTC Library: DS1307 and DS3231 driver library
 * (C) 2011 Akafugu
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

/*
 * DS1307 register map
 *  
 *  00h-06h: seconds, minutes, hours, day-of-week, date, month, year (all in BCD)
 *     bit 7 of seconds enables/disables clock
 *     bit 6 of hours toggles 12/24h mode (1 for 12h, 0 for 24h)
 *       when 12h mode is selected bit 5 is high for PM, low for AM
 *  07h: control
 *      bit7: OUT
 *      bit6: 0
 *      bit5: 0
 *      bit4: SQWE
 *      bit3: 0
 *      bit2: 0
 *      bit1: RS0
 *      bit0: RS1
 *  08h-3fh: 56 bytes of SRAM
 *
 * DS3231 register map
 *
 *  00h-06h: seconds, minutes, hours, day-of-week, date, month, year (all in BCD)
 *     bit 7 of seconds enables/disables clock
 *
 */

#include <avr/io.h>

#define TRUE 1
#define FALSE 0

#include "WireRtcLib.h"

#define RTC_ADDR 0x68 // I2C address
#define CH_BIT 7 // clock halt bit

// *16
// >>4
uint8_t WireRtcLib::dec2bcd(uint8_t d)
{
  return ((d/10 * 16) + (d % 10));
}

uint8_t WireRtcLib::bcd2dec(uint8_t b)
{
  return ((b/16 * 10) + (b % 16));
}

uint8_t WireRtcLib::read_byte(uint8_t offset)
{
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(offset);
	Wire.endTransmission();

	Wire.requestFrom(RTC_ADDR, 1);
	return Wire.receive();
}

void WireRtcLib::write_byte(uint8_t b, uint8_t offset)
{
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(offset);
	Wire.send(b);
	Wire.endTransmission();
}

WireRtcLib::WireRtcLib()
: m_is_ds1307(false)
, m_is_ds3231(false)
{}

void WireRtcLib::begin()
{
	// Attempt autodetection:
	// 1) Read and save temperature register
	// 2) Write a value to temperature register
	// 3) Read back the value
	//   equal to the one written: DS1307, write back saved value and return
	//   different from written:   DS3231
	
	uint8_t temp1 = read_byte(0x11);
	uint8_t temp2 = read_byte(0x12);
	
	write_byte(0xee, 0x11);
	write_byte(0xdd, 0x12);
	
	if (read_byte(0x11) == 0xee && read_byte(0x12) == 0xdd) {
		m_is_ds1307 = true;
		// restore values
		write_byte(temp1, 0x11);
		write_byte(temp2, 0x12);
	}
	else {
		m_is_ds3231 = true;
	}
}

// Autodetection
bool WireRtcLib::isDS1307(void) { return m_is_ds1307; }
bool WireRtcLib::isDS3231(void) { return m_is_ds3231; }

// Autodetection override
void WireRtcLib::setDS1307(void) { m_is_ds1307 = true;   m_is_ds3231 = false; }
void WireRtcLib::setDS3231(void) { m_is_ds1307 = false;  m_is_ds3231 = true;  }

WireRtcLib::tm* WireRtcLib::getTime(void)
{
	uint8_t rtc[9];

	// read 7 bytes starting from register 0
	// sec, min, hour, day-of-week, date, month, year
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(0x0);
	Wire.endTransmission();
	
	Wire.requestFrom(RTC_ADDR, 7);
	
	for(uint8_t i=0; i<7; i++) {
		rtc[i] = Wire.receive();
	}
	
	Wire.endTransmission();

	// Clear clock halt bit from read data
	rtc[0] &= ~(_BV(CH_BIT)); // clear bit

	m_tm.sec  = bcd2dec(rtc[0]);
	m_tm.min  = bcd2dec(rtc[1]);
	m_tm.hour = bcd2dec(rtc[2]);
	m_tm.mday = bcd2dec(rtc[4]);
	m_tm.mon  = bcd2dec(rtc[5]); // returns 1-12
	m_tm.year = bcd2dec(rtc[6]); // year 0-99
	m_tm.wday = bcd2dec(rtc[3]); // returns 1-7
	
	if (m_tm.hour == 0) {
		m_tm.twelveHour = 0;
		m_tm.am = 1;
	}
	else if (m_tm.hour < 12) {
		m_tm.twelveHour = m_tm.hour;
		m_tm.am = 1;
	}
	else {
		m_tm.twelveHour = m_tm.hour - 12;
		m_tm.am = 0;
	}
	
	return &m_tm;
}

void WireRtcLib::getTime_s(uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	uint8_t rtc[9];

	// read 7 bytes starting from register 0
	// sec, min, hour, day-of-week, date, month, year
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(0x0);
	Wire.endTransmission();
	
	Wire.requestFrom(RTC_ADDR, 7);
	
	for(uint8_t i=0; i<7; i++) {
		rtc[i] = Wire.receive();
	}
	
	Wire.endTransmission();
	
	if (sec)  *sec =  bcd2dec(rtc[0]);
	if (min)  *min =  bcd2dec(rtc[1]);
	if (hour) *hour = bcd2dec(rtc[2]);
}

void WireRtcLib::setTime(WireRtcLib::tm* tm)
{
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(0x0);

	// clock halt bit is 7th bit of seconds: this is always cleared to start the clock
	Wire.send(dec2bcd(tm->sec)); // seconds
	Wire.send(dec2bcd(tm->min)); // minutes
	Wire.send(dec2bcd(tm->hour)); // hours
	Wire.send(dec2bcd(tm->wday)); // day of week
	Wire.send(dec2bcd(tm->mday)); // day
	Wire.send(dec2bcd(tm->mon)); // month
	Wire.send(dec2bcd(tm->year)); // year
	
	Wire.endTransmission();
}

void WireRtcLib::setTime_s(uint8_t hour, uint8_t min, uint8_t sec)
{
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(0x0);

	// clock halt bit is 7th bit of seconds: this is always cleared to start the clock
	Wire.send(dec2bcd(sec)); // seconds
	Wire.send(dec2bcd(min)); // minutes
	Wire.send(dec2bcd(hour)); // hours
	
	Wire.endTransmission();
}

// halt/start the clock
// 7th bit of register 0 (second register)
// 0 = clock is running
// 1 = clock is not running
void WireRtcLib::runClock(bool run)
{
  if (m_is_ds3231) return;
  
  uint8_t b = read_byte(0x0);

  if (run)
    b &= ~(_BV(CH_BIT)); // clear bit
  else
    b |= _BV(CH_BIT); // set bit
    
    write_byte(b, 0x0);
}

bool WireRtcLib::isClockRunning(void)
{
  if (m_is_ds3231) return true;
  
  uint8_t b = read_byte(0x0);

  if (b & _BV(CH_BIT)) return false;
  return true;
}

void WireRtcLib::getTemp(int8_t* i, uint8_t* f)
{
	uint8_t msb, lsb;
	
	*i = 0;
	*f = 0;
	
	if (m_is_ds1307) return; // only valid on DS3231
	
	Wire.beginTransmission(RTC_ADDR);
	// temp registers are 0x11 and 0x12
	Wire.send(0x11);
	Wire.endTransmission();

	Wire.requestFrom(RTC_ADDR, 2);

	if (Wire.available()) {
		msb = Wire.receive(); // integer part (in twos complement)
		lsb = Wire.receive(); // fraction part
		
		// integer part in entire byte
		*i = msb;
		// fractional part in top two bits (increments of 0.25)
		*f = (lsb >> 6) * 25;
		
		// float value can be read like so:
		// float temp = ((((short)msb << 8) | (short)lsb) >> 6) / 4.0f;
	}
}

void WireRtcLib::forceTempConversion(uint8_t block)
{
	if (m_is_ds1307) return; // only valid on DS3231

	// read control register (0x0E)
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(0x0E);
	Wire.endTransmission();

	Wire.requestFrom(RTC_ADDR, 1);
	uint8_t ctrl = Wire.receive();

	ctrl |= 0b00100000; // Set CONV bit

	// write new control register value
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(0x0E);
	Wire.send(ctrl);
	Wire.endTransmission();

	if (!block) return;
	
	// Temp conversion is ready when control register becomes 0
	do {
		// Block until CONV is 0
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x0E);
		Wire.endTransmission();
		Wire.requestFrom(RTC_ADDR, 1);
	} while ((Wire.receive() & 0b00100000) != 0);
}

#define DS1307_SRAM_ADDR 0x08

// SRAM: 56 bytes from address 0x08 to 0x3f (DS1307-only)
void WireRtcLib::getSram(uint8_t* data)
{
	// cannot receive 56 bytes in one go, because of the Wire library buffer limit
	// so just receive one at a time for simplicity
  	for(int i=0;i<56;i++)
		data[i] = getSramByte(i);
}

void WireRtcLib::setSram(uint8_t *data)
{
	// cannot send 56 bytes in one go, because of the Wire library buffer limit
	// so just send one at a time for simplicity
  	for(int i=0;i<56;i++)
		setSramByte(data[i], i);
}

uint8_t WireRtcLib::getSramByte(uint8_t offset)
{
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(DS1307_SRAM_ADDR + offset);
	Wire.endTransmission();

	Wire.requestFrom(RTC_ADDR, 1);
	return Wire.receive();
}

void WireRtcLib::setSramByte(uint8_t b, uint8_t offset)
{
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(DS1307_SRAM_ADDR + offset);
	Wire.send(b);
	Wire.endTransmission();
}

void WireRtcLib::SQWEnable(bool enable)
{
	if (m_is_ds1307) {
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x07);
		Wire.endTransmission();
		
		// read control
   		Wire.requestFrom(RTC_ADDR, 1);
		uint8_t control = Wire.receive();

		if (enable)
			control |=  0b00010000; // set SQWE to 1
		else
			control &= ~0b00010000; // set SQWE to 0

		// write control back
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x07);
		Wire.send(control);
		Wire.endTransmission();

	}
	else { // DS3231
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x0E);
		Wire.endTransmission();
		
		// read control
   		Wire.requestFrom(RTC_ADDR, 1);
		uint8_t control = Wire.receive();

		if (enable) {
			control |=  0b01000000; // set BBSQW to 1
			control &= ~0b00000100; // set INTCN to 0
		}
		else {
			control &= ~0b01000000; // set BBSQW to 0
		}

		// write control back
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x0E);
		Wire.send(control);
		Wire.endTransmission();
	}
}

void WireRtcLib::SQWSetFreq(enum RTC_SQW_FREQ freq)
{
	if (m_is_ds1307) {
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x07);
		Wire.endTransmission();
		
		// read control (uses bits 0 and 1)
   		Wire.requestFrom(RTC_ADDR, 1);
		uint8_t control = Wire.receive();

		control &= ~0b00000011; // Set to 0
		control |= freq; // Set freq bitmask

		// write control back
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x07);
		Wire.send(control);
		Wire.endTransmission();

	}
	else { // DS3231
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x0E);
		Wire.endTransmission();
		
		// read control (uses bits 3 and 4)
   		Wire.requestFrom(RTC_ADDR, 1);
		uint8_t control = Wire.receive();

		control &= ~0b00011000; // Set to 0
		control |= (freq << 4); // Set freq bitmask

		// write control back
		Wire.beginTransmission(RTC_ADDR);
		Wire.send(0x0E);
		Wire.send(control);
		Wire.endTransmission();
	}
}

// DS3231 only
void WireRtcLib::Osc32kHzEnable(bool enable)
{
	if (!m_is_ds3231) return;

	Wire.beginTransmission(RTC_ADDR);
	Wire.send(0x0F);
	Wire.endTransmission();

	// read status
	Wire.requestFrom(RTC_ADDR, 1);
	uint8_t status = Wire.receive();

	if (enable)
		status |= 0b00001000; // set to 1
	else
		status &= ~0b00001000; // Set to 0

	// write status back
	Wire.beginTransmission(RTC_ADDR);
	Wire.send(0x0F);
	Wire.send(status);
	Wire.endTransmission();
}

// ALARM FUNCTIONALITY
//
// On DS1307, SRAM bytes 0 to 2 are used to store the alarm data
// On DS3232, native alarm 1 is used
//

// reset the alarm to 0:00
void WireRtcLib::resetAlarm(void)
{
	if (m_is_ds1307) {
		setSramByte(0, 0); // hour
		setSramByte(0, 1); // minute
		setSramByte(0, 2); // second
	}
	else {
		// fixme: implement
	}
}

// set the alarm to hour:min:sec
void WireRtcLib::setAlarm(uint8_t hour, uint8_t min, uint8_t sec)
{
	if (m_is_ds1307) {
		setSramByte(hour, 0); // hour
		setSramByte(min,  1); // minute
		setSramByte(sec,  2); // sec 
	}
	else {
		// fixme: implement
	}
}

// get the currently set alarm
void WireRtcLib::getAlarm(uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	if (m_is_ds1307) {
		if (hour) *hour = getSramByte(0);
		if (min)  *min  = getSramByte(1);
		if (sec)  *sec  = getSramByte(2);
	}
	else {
		// fixme: implement
	}
}

// check whether or not the alarm is going off
// must be polled more than once a second
bool WireRtcLib::checkAlarm(void)
{
	if (m_is_ds1307) {
		uint8_t hour = getSramByte(0);
		uint8_t min  = getSramByte(1);
		uint8_t sec  = getSramByte(2);

		uint8_t cur_hour, cur_min, cur_sec;
		getTime_s(&cur_hour, &cur_min, &cur_sec);
		
		if (cur_hour == hour && cur_min == min && cur_sec == sec)
			return true;
		return false;
	}
	else {
		// fixme: implement
		return false;
	}
}
