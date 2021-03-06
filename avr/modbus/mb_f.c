/* 
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id: mb.c,v 1.28 2010/06/06 13:54:40 wolti Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_f.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbfunc.h"
#include "mbport.h"

#if MB_FIREWALL_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_FIREWALL_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_FIREWALL_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 0
#endif

/* ----------------------- Static variables ---------------------------------*/

static eMBMode  eMBCurrentMode;

static enum
{
    STATE_ENABLED,
    STATE_DISABLED,
    STATE_NOT_INITIALIZED
} eMBState = STATE_NOT_INITIALIZED;

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 */
static peMBFrameSend peMBFirewallInputFrameSendCur;
static peMBFrameSend peMBFirewallOutputFrameSendCur;

static pvMBFrameStart pvMBFirewallFrameStartCur;
static pvMBFrameStop pvMBFirewallFrameStopCur;

static peMBFrameReceive peMBFirewallInputFrameReceiveCur;
static peMBFrameReceive peMBFirewallOutputFrameReceiveCur;

static pvMBFrameClose pvMBFrameCloseCur;

/* Callback functions required by the porting layer. They are called when
 * an external event has happend which includes a timeout or the reception
 * or transmission of a character.
 */
BOOL( *pxMBFirewallInputFrameCBByteReceived ) ( void );
BOOL( *pxMBFirewallOutputFrameCBByteReceived ) ( void );

BOOL( *pxMBFirewallInputFrameCBTransmitterEmpty ) ( void );
BOOL( *pxMBFirewallOutputFrameCBTransmitterEmpty ) ( void );

BOOL( *pxMBFirewallInputPortCBTimerExpired ) ( void );
BOOL( *pxMBFirewallOutputPortCBTimerExpired ) ( void );

#if MB_FIREWALL_RTU_ENABLED
static xMBFirewallSerialPacketHandler xMBFirewallPacketHandlerCur;
#endif

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBFirewallInit( eMBMode eMode, UCHAR ucPortInput, ULONG ulBaudRateInput, eMBParity eParityInput,
                 UCHAR ucPortOutput, ULONG ulBaudRateOutput, eMBParity eParityOutput, xMBFirewallPacketHandler xPacketHandler)
{
    eMBErrorCode    eStatus = MB_ENOERR;

    /* Get mode */
    switch ( eMode )
    {
#if MB_FIREWALL_RTU_ENABLED > 0
    case MB_RTU:
        pvMBFirewallFrameStartCur = eMBFirewallRTUStart;
        pvMBFirewallFrameStopCur = eMBFirewallRTUStop;

        peMBFirewallInputFrameSendCur = eMBFirewallInputRTUSend;
        peMBFirewallOutputFrameSendCur = eMBFirewallOutputRTUSend;

        peMBFirewallInputFrameReceiveCur = eMBFirewallInputRTUReceive;
        peMBFirewallOutputFrameReceiveCur = eMBFirewallOutputRTUReceive;

        pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;

        pxMBFirewallInputFrameCBByteReceived = xMBFirewallInputRTUReceiveFSM;
        pxMBFirewallOutputFrameCBByteReceived = xMBFirewallOutputRTUReceiveFSM;

        pxMBFirewallInputFrameCBTransmitterEmpty = xMBFirewallInputRTUTransmitFSM;
        pxMBFirewallOutputFrameCBTransmitterEmpty = xMBFirewallOutputRTUTransmitFSM;
    
        pxMBFirewallInputPortCBTimerExpired = xMBFirewallInputRTUTimerT35Expired;
        pxMBFirewallOutputPortCBTimerExpired = xMBFirewallOutputRTUTimerT35Expired;

        xMBFirewallPacketHandlerCur = xPacketHandler;

        eStatus = eMBFirewallRTUInit(ucPortInput, ulBaudRateInput, eParityInput, ucPortOutput, ulBaudRateOutput, eParityOutput);
        break;
#endif
#if MB_FIREWALL_ASCII_ENABLED > 0
    case MB_ASCII:
        pvMBFirewallFrameStartCur = eMBASCIIStart;
        pvMBFirewallFrameStopCur = eMBASCIIStop;
        peMBFrameSendCur = eMBASCIISend;
        peMBFrameReceiveCur = eMBASCIIReceive;
        pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
        pxMBFrameCBByteReceived = xMBASCIIReceiveFSM;
        pxMBFrameCBTransmitterEmpty = xMBASCIITransmitFSM;
        pxMBPortCBTimerExpired = xMBASCIITimerT1SExpired;

        eStatus = eMBASCIIInit( ucMBAddress, ucPort, ulBaudRate, eParity );
        break;
#endif
    default:
        eStatus = MB_EINVAL;
    }

    if( eStatus == MB_ENOERR )
    {
        if( !xMBFirewallPortEventInit(  ) )
        {
            /* port dependent event module initalization failed. */
            eStatus = MB_EPORTERR;
        }
        else
        {
            eMBCurrentMode = eMode;
            eMBState = STATE_DISABLED;
        }
    }

    return eStatus;
}

#if MB_FIREWALL_TCP_ENABLED > 0
eMBErrorCode
eMBTCPInit( USHORT ucTCPPort )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( ( eStatus = eMBTCPDoInit( ucTCPPort ) ) != MB_ENOERR )
    {
        eMBState = STATE_DISABLED;
    }
    else if( !xMBPortEventInit(  ) )
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
        pvMBFirewallFrameStartCur = eMBTCPStart;
        pvMBFirewallFrameStopCur = eMBTCPStop;
        peMBFrameReceiveCur = eMBTCPReceive;
        peMBFrameSendCur = eMBTCPSend;
        pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
        ucMBAddress = MB_TCP_PSEUDO_ADDRESS;
        eMBCurrentMode = MB_TCP;
        eMBState = STATE_DISABLED;
    }
    return eStatus;
}
#endif


eMBErrorCode
eMBFirewallClose( void )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState == STATE_DISABLED )
    {
        if( pvMBFrameCloseCur != NULL )
        {
            pvMBFrameCloseCur(  );
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBFirewallEnable( void )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( eMBState == STATE_DISABLED )
    {
        /* Activate the protocol stack. */
        pvMBFirewallFrameStartCur(  );
        eMBState = STATE_ENABLED;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBFirewallDisable( void )
{
    eMBErrorCode    eStatus;

    if( eMBState == STATE_ENABLED )
    {
        pvMBFirewallFrameStopCur(  );
        eMBState = STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    else if( eMBState == STATE_DISABLED )
    {
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBFirewallPoll( void )
{
    static UCHAR   *ucMBFrame;
    static UCHAR    ucRcvAddress;
    static USHORT   usLength;
    static eMBException eException;

    int             i;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBFirewallEventType    eEvent;
    BOOL    xPacketPass;

    /* Check if the protocol stack is ready. */
    if( eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if( xMBFirewallPortEventGet( &eEvent ) == TRUE )
    {
        switch ( eEvent )
        {
        case EV_F_READY:
            break;

        case EV_F_INPUT_FRAME_RECEIVED:
            eStatus = peMBFirewallInputFrameReceiveCur( &ucRcvAddress, &ucMBFrame, &usLength );
            
            if( eStatus == MB_ENOERR )
            {
                
                /* Process the received frame */
                ( void )xMBFirewallPortEventPost( EV_F_INPUT_EXECUTE );
            }
            break;

        case EV_F_INPUT_EXECUTE:
            /* Call callback with the frame */
            xPacketPass = xMBFirewallPacketHandlerCur(ucRcvAddress, ucMBFrame, usLength);

            if (xPacketPass == TRUE) {
                eStatus = peMBFirewallOutputFrameSendCur( ucRcvAddress, ucMBFrame, usLength );
            }
            else {
                /* Packet blocked, it will wait for timeout */
                eStatus = MB_EINVAL;
            }

            break;

        case EV_F_OUTPUT_FRAME_RECEIVED:
            eStatus = peMBFirewallOutputFrameReceiveCur( &ucRcvAddress, &ucMBFrame, &usLength );
            if( eStatus == MB_ENOERR )
            {
                /* Process the received frame */
                ( void )xMBFirewallPortEventPost( EV_F_OUTPUT_EXECUTE );
            }
            break;
            break;

        case EV_F_OUTPUT_EXECUTE:
            eStatus = peMBFirewallInputFrameSendCur( ucRcvAddress, ucMBFrame, usLength );
            break;

        case EV_F_INPUT_FRAME_SENT:
            break;

        case EV_F_OUTPUT_FRAME_SENT:
            break;
        }
    }
    return MB_ENOERR;
}