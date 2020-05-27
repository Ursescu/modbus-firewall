/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006-2018 Christian Walter <cwalter@embedded-solutions.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _MB_PORT_H
#define _MB_PORT_H

#include "mbconfig.h"

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

/* ----------------------- Type definitions ---------------------------------*/

typedef enum
{
    EV_READY,                   /*!< Startup finished. */
    EV_FRAME_RECEIVED,          /*!< Frame received. */
    EV_EXECUTE,                 /*!< Execute function. */
    EV_FRAME_SENT               /*!< Frame sent. */
} eMBEventType;


#if MB_FIREWALL_RTU_ENABLED  || MB_FIREWALL_ASCII_ENABLED
typedef enum
{
    EV_F_READY = 0x01,                       /*!< Startup finished. */
    EV_F_INPUT_FRAME_RECEIVED = 0x02,              /*!< Frame received. */
    EV_F_INPUT_EXECUTE = 0x04,                 /*!< Execute function. */
    EV_F_INPUT_FRAME_SENT = 0x08,               /*!< Frame sent. */
    EV_F_OUTPUT_FRAME_RECEIVED = 0x10,          /*!< Frame received. */
    EV_F_OUTPUT_EXECUTE = 0x12,                 /*!< Execute function. */
    EV_F_OUTPUT_FRAME_SENT = 0x14               /*!< Frame sent. */

} eMBFirewallEventType;

#endif 
/*! \ingroup modbus
 * \brief Parity used for characters in serial mode.
 *
 * The parity which should be applied to the characters sent over the serial
 * link. Please note that this values are actually passed to the porting
 * layer and therefore not all parity modes might be available.
 */
typedef enum
{
    MB_PAR_NONE,                /*!< No parity. */
    MB_PAR_ODD,                 /*!< Odd parity. */
    MB_PAR_EVEN                 /*!< Even parity. */
} eMBParity;

/* ----------------------- Supporting functions -----------------------------*/
BOOL            xMBPortEventInit( void );

BOOL            xMBPortEventPost( eMBEventType eEvent );

BOOL            xMBPortEventGet(  /*@out@ */ eMBEventType * eEvent );

#if MB_FIREWALL_RTU_ENABLED || MB_FIREWALL_ASCII_ENABLED
BOOL            xMBFirewallPortEventInit( void );

BOOL            xMBFirewallPortEventPost( eMBFirewallEventType eEvent );

BOOL            xMBFirewallPortEventGet( eMBFirewallEventType * eEvent );
#endif

/* ----------------------- Serial port functions ----------------------------*/

BOOL            xMBPortSerialInit( UCHAR ucPort, ULONG ulBaudRate,
                                   UCHAR ucDataBits, eMBParity eParity );

void            vMBPortClose( void );

void            xMBPortSerialClose( void );

void            vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable );

BOOL            xMBPortSerialGetByte( CHAR * pucByte );

BOOL            xMBPortSerialPutByte( CHAR ucByte );


#if MB_FIREWALL_RTU_ENABLED  || MB_FIREWALL_ASCII_ENABLED

BOOL            xMBFirewallPortSerialInit( UCHAR ucPortInput, ULONG ulBaudRateInput,
                                   UCHAR ucDataBitsInput, eMBParity eParityInput,
                                   UCHAR ucPortOutput, ULONG ulBaudRateOutput,
                                   UCHAR ucDataBitsOutput, eMBParity eParityOutput);

void            vMBFirewallPortClose( void );

void            xMBFirewallPortSerialClose( void );

void            vMBFirewallInputPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable);

void            vMBFirewallOutputPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable);

BOOL            xMBFirewallInputPortSerialGetByte( CHAR * pucByte );

BOOL            xMBFirewallOutputPortSerialGetByte( CHAR * pucByte );

BOOL            xMBFirewallInputPortSerialPutByte( CHAR ucByte );

BOOL            xMBFirewallOutputPortSerialPutByte( CHAR ucByte );

#endif

/* ----------------------- Timers functions ---------------------------------*/
BOOL            xMBPortTimersInit( USHORT usTimeOut50us );

void            xMBPortTimersClose( void );

void            vMBPortTimersEnable( void );

void            vMBPortTimersDisable( void );

void            vMBPortTimersDelay( USHORT usTimeOutMS );

#if MB_FIREWALL_RTU_ENABLED || MB_FIREWALL_ASCII_ENABLED
BOOL            xMBFirewallPortTimersInit( USHORT usTimeOut50usInput,  USHORT usTimeOut50usOutput );

void            xMBFirewallInputPortTimersClose( void );

void            xMBFirewallOutputPortTimersClose( void );

void            vMBFirewallInputPortTimersEnable( void );

void            vMBFirewallOutputPortTimersEnable( void );

void            vMBFirewallInputPortTimersDisable( void );

void            vMBFirewallOutputPortTimersDisable( void );

#endif 

/* ----------------------- Callback for the protocol stack ------------------*/

/*!
 * \brief Callback function for the porting layer when a new byte is
 *   available.
 *
 * Depending upon the mode this callback function is used by the RTU or
 * ASCII transmission layers. In any case a call to xMBPortSerialGetByte()
 * must immediately return a new character.
 *
 * \return <code>TRUE</code> if a event was posted to the queue because
 *   a new byte was received. The port implementation should wake up the
 *   tasks which are currently blocked on the eventqueue.
 */
extern          BOOL( *pxMBFrameCBByteReceived ) ( void );

extern          BOOL( *pxMBFrameCBTransmitterEmpty ) ( void );

extern          BOOL( *pxMBPortCBTimerExpired ) ( void );

#if MB_FIREWALL_RTU_ENABLED || MB_FIREWALL_ASCII_ENABLED
extern          BOOL( *pxMBFirewallInputFrameCBByteReceived ) ( void );
extern          BOOL( *pxMBFirewallOutputFrameCBByteReceived ) ( void );

extern          BOOL( *pxMBFirewallInputFrameCBTransmitterEmpty ) ( void );
extern          BOOL( *pxMBFirewallOutputFrameCBTransmitterEmpty ) ( void );

extern          BOOL( *pxMBFirewallInputPortCBTimerExpired ) ( void );
extern          BOOL( *pxMBFirewallOutputPortCBTimerExpired ) ( void );
#endif

/* ----------------------- TCP port functions -------------------------------*/
BOOL            xMBTCPPortInit( USHORT usTCPPort );

void            vMBTCPPortClose( void );

void            vMBTCPPortDisable( void );

BOOL            xMBTCPPortGetRequest( UCHAR **ppucMBTCPFrame, USHORT * usTCPLength );

BOOL            xMBTCPPortSendResponse( const UCHAR *pucMBTCPFrame, USHORT usTCPLength );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
