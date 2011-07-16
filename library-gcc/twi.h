/*
  TwoWire.h - TWI/I2C library for Arduino & Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef TwoWire_h
#define TwoWire_h

#include <inttypes.h>

#define BUFFER_LENGTH 32

void twi_init_master(void);
void twi_init_slave(uint8_t);
void twi_begin_transmission(uint8_t);
uint8_t twi_end_transmission(void);
uint8_t twi_request_from(uint8_t, uint8_t);
void twi_send_byte(uint8_t);
void twi_send(uint8_t*, uint8_t);
void twi_send_char(char*);
uint8_t twi_available(void);
uint8_t twi_receive(void);
void twi_set_on_receive( void (*)(int) );
void twi_set_on_request( void (*)(void) );

#endif
