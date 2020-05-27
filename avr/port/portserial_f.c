/*
 * FreeModbus Libary: ATMega168 Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *   - Initial version and ATmega168 support
 * Modfications Copyright (C) 2006 Tran Minh Hoang:
 *   - ATmega8, ATmega16, ATmega32 support
 *   - RS485 support for DS75176
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

#include <avr/io.h>
#include <avr/interrupt.h>

#include "port.h"

#define BAUD 115200
#include <util/setbaud.h>

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_f.h"
#include "mbport.h"

#define UART_BAUD_CALC(UART_BAUD_RATE, F_OSC) \
    ( ( F_OSC ) / ( ( UART_BAUD_RATE ) * 16UL ) - 1 )

void
vMBFirewallInputPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    UCSR0B |= _BV( TXEN0 );


    if( xRxEnable )
    {
        UCSR0B |= _BV( RXEN0 ) | _BV( RXCIE0 );
    }
    else
    {
        UCSR0B &= ~( _BV( RXEN0 ) | _BV( RXCIE0 ) );
    }

    if( xTxEnable )
    {
        UCSR0B |= _BV( TXEN0 ) | _BV( UDRIE0 );
    }
    else
    {
        UCSR0B &= ~( _BV( UDRIE0 ) );
    }
}


void
vMBFirewallOutputPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{

    UCSR1B |= _BV( TXEN1 );


    if( xRxEnable )
    {
        UCSR1B |= _BV( RXEN1 ) | _BV( RXCIE1 );
    }
    else
    {
        UCSR1B &= ~( _BV( RXEN1 ) | _BV( RXCIE1 ) );
    }

    if( xTxEnable )
    {
        UCSR1B |= _BV( TXEN1 ) | _BV( UDRIE1 );
    }
    else
    {
        UCSR1B &= ~( _BV( UDRIE1 ) );
    }
}

BOOL
xMBFirewallPortSerialInit( UCHAR ucPortInput, ULONG ulBaudRateInput,
                                   UCHAR ucDataBitsInput, eMBParity eParityInput,
                                   UCHAR ucPortOutput, ULONG ulBaudRateOutput,
                                   UCHAR ucDataBitsOutput, eMBParity eParityOutput)
{
    UCHAR ucUCSRC0 = 0;
    UCHAR ucUCSRC1 = 0;

    /* prevent compiler warning. */
    (void)ucPortInput;
    (void)ucPortOutput;
	
    // UBRR0 = UART_BAUD_CALC( ulBaudRateInput, F_CPU );
    // UBRR1 = UART_BAUD_CALC( ulBaudRateInput, F_CPU );

    UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

    UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;

#if USE_2X
	UCSR0A |= (1<<U2X0);
    UCSR1A |= (1<<U2X1);
#else
	UCSR0A &= ~(1 << U2X0);
    UCSR1A &= ~(1 << U2X1);
#endif

    switch ( eParityInput )
    {
        case MB_PAR_EVEN:
            ucUCSRC0 |= _BV( UPM01 );
            break;
        case MB_PAR_ODD:
            ucUCSRC0 |= _BV( UPM01 ) | _BV( UPM00 );
            break;
        case MB_PAR_NONE:
            break;
    }

    switch ( ucDataBitsInput )
    {
        case 8:
            ucUCSRC0 |= _BV( UCSZ00 ) | _BV( UCSZ01 );
            break;
        case 7:
            ucUCSRC0 |= _BV( UCSZ01 );
            break;
    }

    switch ( eParityOutput )
    {
        case MB_PAR_EVEN:
            ucUCSRC1 |= _BV( UPM11 );
            break;
        case MB_PAR_ODD:
            ucUCSRC1 |= _BV( UPM11 ) | _BV( UPM10 );
            break;
        case MB_PAR_NONE:
            break;
    }

    switch ( ucDataBitsInput )
    {
        case 8:
            ucUCSRC1 |= _BV( UCSZ10 ) | _BV( UCSZ11 );
            break;
        case 7:
            ucUCSRC1 |= _BV( UCSZ11 );
            break;
    }

    UCSR0C |= ucUCSRC0;
    UCSR1C |= ucUCSRC1;


    vMBFirewallInputPortSerialEnable( FALSE, FALSE );
    vMBFirewallOutputPortSerialEnable( FALSE, FALSE );

    return TRUE;
}

BOOL
xMBFirewallInputPortSerialPutByte( CHAR ucByte )
{
    UDR0 = ucByte;
    return TRUE;
}

BOOL
xMBFirewallInputPortSerialGetByte( CHAR * pucByte )
{
    *pucByte = UDR0;
    return TRUE;
}

BOOL
xMBFirewallOutputPortSerialPutByte( CHAR ucByte )
{
    UDR1 = ucByte;
    return TRUE;
}

BOOL
xMBFirewallOutputPortSerialGetByte( CHAR * pucByte )
{
    *pucByte = UDR1;
    return TRUE;
}

ISR( USART0_UDRE_vect )
{
    pxMBFirewallInputFrameCBTransmitterEmpty(  );
}

ISR( USART0_RX_vect )
{
    pxMBFirewallInputFrameCBByteReceived(  );
}

ISR( USART1_UDRE_vect )
{
    pxMBFirewallOutputFrameCBTransmitterEmpty(  );
}

ISR( USART1_RX_vect )
{
    pxMBFirewallOutputFrameCBByteReceived(  );
}
