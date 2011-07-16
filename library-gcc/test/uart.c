/*! \file uart.c \brief UART driver with buffer support. */
// *****************************************************************************
//
// File Name	: 'uart.c'
// Title		: UART driver with buffer support
// Author		: Pascal Stang - Copyright (C) 2000-2002
// Created		: 11/22/2000
// Revised		: 06/09/2003
// Version		: 1.3
// Target MCU	: ATMEL AVR Series
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
// *****************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "buffer.h"
#include "uart.h"

// UART global variables
// flag variables
volatile uint8_t   uartReadyTx;			///< uartReadyTx flag
volatile uint8_t   uartBufferedTx;		///< uartBufferedTx flag
// receive and transmit buffers
cBuffer uartRxBuffer;				///< uart receive buffer
cBuffer uartTxBuffer;				///< uart transmit buffer
unsigned short uartRxOverflow;		///< receive overflow counter

#ifndef UART_BUFFERS_EXTERNAL_RAM
	// using internal ram,
	// automatically allocate space in ram for each buffer
	static unsigned char uartRxData[UART_RX_BUFFER_SIZE];
	static unsigned char uartTxData[UART_TX_BUFFER_SIZE];
#endif

typedef void (*voidFuncPtru08)(unsigned char);
volatile static voidFuncPtru08 UartRxFunc;

// enable and initialize the uart
void uartInit(void)
{
	// initialize the buffers
	uartInitBuffers();
	// initialize user receive handler
	UartRxFunc = 0;

	// enable RxD/TxD and interrupts
	outb(UCR, BV(RXCIE)|BV(TXCIE)|BV(RXEN)|BV(TXEN));

	// set default baud rate
	uartSetBaudRate(UART_DEFAULT_BAUD_RATE);  
	// initialize states
	uartReadyTx = TRUE;
	uartBufferedTx = FALSE;
	// clear overflow count
	uartRxOverflow = 0;
	// enable interrupts
	sei();
}

// create and initialize the uart transmit and receive buffers
void uartInitBuffers(void)
{
	#ifndef UART_BUFFERS_EXTERNAL_RAM
		// initialize the UART receive buffer
		bufferInit(&uartRxBuffer, uartRxData, UART_RX_BUFFER_SIZE);
		// initialize the UART transmit buffer
		bufferInit(&uartTxBuffer, uartTxData, UART_TX_BUFFER_SIZE);
	#else
		// initialize the UART receive buffer
		bufferInit(&uartRxBuffer, (uint8_t*) UART_RX_BUFFER_ADDR, UART_RX_BUFFER_SIZE);
		// initialize the UART transmit buffer
		bufferInit(&uartTxBuffer, (uint8_t*) UART_TX_BUFFER_ADDR, UART_TX_BUFFER_SIZE);
	#endif
}

// redirects received data to a user function
void uartSetRxHandler(void (*rx_func)(unsigned char c))
{
	// set the receive interrupt to run the supplied user function
	UartRxFunc = rx_func;
}

// set the uart baud rate
void uartSetBaudRate(uint32_t baudrate)
{
	// calculate division factor for requested baud rate, and set it
	uint16_t bauddiv = ((F_CPU+(baudrate*8L))/(baudrate*16L)-1);
	outb(UBRRL, bauddiv);
	#ifdef UBRRH
	outb(UBRRH, bauddiv>>8);
	#endif
}

// returns the receive buffer structure 
cBuffer* uartGetRxBuffer(void)
{
	// return rx buffer pointer
	return &uartRxBuffer;
}

// returns the transmit buffer structure 
cBuffer* uartGetTxBuffer(void)
{
	// return tx buffer pointer
	return &uartTxBuffer;
}

// transmits a byte over the uart
void uartSendByte(uint8_t txData)
{
	// wait for the transmitter to be ready
	while(!uartReadyTx);
	// send byte
	outb(UDR, txData);
	// set ready state to FALSE
	uartReadyTx = FALSE;
}

// gets a single byte from the uart receive buffer (getchar-style)
int uartGetByte(void)
{
	uint8_t c;
	if(uartReceiveByte(&c))
		return c;
	else
		return -1;
}

// gets a byte (if available) from the uart receive buffer
uint8_t uartReceiveByte(uint8_t* rxData)
{
	// make sure we have a receive buffer
	if(uartRxBuffer.size)
	{
		// make sure we have data
		if(uartRxBuffer.datalength)
		{
			// get byte from beginning of buffer
			*rxData = bufferGetFromFront(&uartRxBuffer);
			return TRUE;
		}
		else
		{
			// no data
			return FALSE;
		}
	}
	else
	{
		// no buffer
		return FALSE;
	}
}

// flush all data out of the receive buffer
void uartFlushReceiveBuffer(void)
{
	// flush all data from receive buffer
	//bufferFlush(&uartRxBuffer);
	// same effect as above
	uartRxBuffer.datalength = 0;
}

// return true if uart receive buffer is empty
uint8_t uartReceiveBufferIsEmpty(void)
{
	if(uartRxBuffer.datalength == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// add byte to end of uart Tx buffer
uint8_t uartAddToTxBuffer(uint8_t data)
{
	// add data byte to the end of the tx buffer
	return bufferAddToEnd(&uartTxBuffer, data);
}

// start transmission of the current uart Tx buffer contents
void uartSendTxBuffer(void)
{
	// turn on buffered transmit
	uartBufferedTx = TRUE;
	// send the first byte to get things going by interrupts
	uartSendByte(bufferGetFromFront(&uartTxBuffer));
}

// transmit nBytes from buffer out the uart
uint8_t uartSendBuffer(char *buffer, uint16_t nBytes)
{
	register uint8_t first;
	register uint16_t i;

	// check if there's space (and that we have any bytes to send at all)
	if((uartTxBuffer.datalength + nBytes < uartTxBuffer.size) && nBytes)
	{
		// grab first character
		first = *buffer++;
		// copy user buffer to uart transmit buffer
		for(i = 0; i < nBytes-1; i++)
		{
			// put data bytes at end of buffer
			bufferAddToEnd(&uartTxBuffer, *buffer++);
		}

		// send the first byte to get things going by interrupts
		uartBufferedTx = TRUE;
		uartSendByte(first);
		// return success
		return TRUE;
	}
	else
	{
		// return failure
		return FALSE;
	}
}

#include <string.h>

uint8_t uartSendString(char *buffer)
{
	if (!buffer) return FALSE;
	return uartSendBuffer(buffer, strlen(buffer));
}


// UART Transmit Complete Interrupt Handler
UART_INTERRUPT_HANDLER(SIG_UART_TRANS)
{
	// check if buffered tx is enabled
	if(uartBufferedTx)
	{
		// check if there's data left in the buffer
		if(uartTxBuffer.datalength)
		{
			// send byte from top of buffer
			outb(UDR, bufferGetFromFront(&uartTxBuffer));
		}
		else
		{
			// no data left
			uartBufferedTx = FALSE;
			// return to ready state
			uartReadyTx = TRUE;
		}
	}
	else
	{
		// we're using single-byte tx mode
		// indicate transmit complete, back to ready
		uartReadyTx = TRUE;
	}
}

// UART Receive Complete Interrupt Handler
UART_INTERRUPT_HANDLER(SIG_UART_RECV)
{
	uint8_t c;
	
	// get received char
	c = inb(UDR);

	// if there's a user function to handle this receive event
	if(UartRxFunc)
	{
		// call it and pass the received data
		UartRxFunc(c);
	}
	else
	{
		// otherwise do default processing
		// put received char in buffer
		// check if there's space
		if( !bufferAddToEnd(&uartRxBuffer, c) )
		{
			// no space in buffer
			// count overflow
			uartRxOverflow++;
		}
	}
}
