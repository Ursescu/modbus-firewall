/*
 * FreeModbus Libary: ATMega168 Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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

/* ----------------------- AVR includes -------------------------------------*/
#include <avr/io.h>
#include <avr/interrupt.h>

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_f.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_TIMER_PRESCALER      ( 1024UL )
#define MB_TIMER_TICKS          ( F_CPU / MB_TIMER_PRESCALER )
#define MB_50US_TICKS           ( 20000UL )

/* ----------------------- Static variables ---------------------------------*/
static USHORT   usTimerOCRADelta;
static USHORT   usTimerOCRBDelta;

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBFirewallPortTimersInit( USHORT usTim1Timerout50usInput, USHORT usTim1Timerout50usOutput )
{
    /* Calculate overflow counter an OCR values for Timer1. */
    usTimerOCRADelta =
        ( MB_TIMER_TICKS * usTim1Timerout50usInput ) / ( MB_50US_TICKS );
    usTimerOCRBDelta = 
        ( MB_TIMER_TICKS * usTim1Timerout50usOutput ) / ( MB_50US_TICKS );

    TCCR1A = 0x00;
    TCCR1B = 0x00;
    TCCR1C = 0x00;

    TCCR0A = 0x00;
    TCCR1B = 0x00;
    TCCR1C = 0x00;


    vMBFirewallInputPortTimersDisable(  );
    vMBFirewallOutputPortTimersDisable(  );

    return TRUE;
}


inline void
vMBFirewallInputPortTimersEnable(  )
{
    TCNT1 = 0x0000;
    if( usTimerOCRADelta > 0 )
    {
        TIMSK1 |= _BV( OCIE1A );
        OCR1A = usTimerOCRADelta;
    }

    TCCR1B |= _BV( CS12 ) | _BV( CS10 );
}


inline void
vMBFirewallOutputPortTimersEnable(  )
{
    TCNT0 = 0x0000;
    if( usTimerOCRBDelta > 0 )
    {
        TIMSK0 |= _BV( OCIE0A );
        OCR0A = usTimerOCRADelta;
    }

    TCCR0B |= _BV( CS02 ) | _BV( CS00 );
}


inline void
vMBFirewallInputPortTimersDisable(  )
{
    /* Disable the timer. */
    TCCR1B &= ~( _BV( CS12 ) | _BV( CS10 ) );
    /* Disable the output compare interrupts for channel A/B. */
    TIMSK1 &= ~( _BV( OCIE1A ) );
    /* Clear output compare flags for channel A/B. */
    TIFR1 |= _BV( OCF1A ) ;
}

inline void
vMBFirewallOutputPortTimersDisable(  )
{
    /* Disable the timer. */
    TCCR0B &= ~( _BV( CS02 ) | _BV( CS00 ) );
    /* Disable the output compare interrupts for channel A/B. */
    TIMSK0 &= ~( _BV( OCIE0A ) );
    /* Clear output compare flags for channel A/B. */
    TIFR0 |= _BV( OCF0A ) ;
}


ISR( TIMER1_COMPA_vect )
{
    ( void )pxMBFirewallInputPortCBTimerExpired(  );
}

ISR( TIMER0_COMPA_vect )
{
    ( void )pxMBFirewallOutputPortCBTimerExpired(  );
}

