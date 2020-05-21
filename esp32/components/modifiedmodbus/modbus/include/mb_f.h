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
 * File: $Id: mb.h,v 1.17 2006/12/07 22:10:34 wolti Exp $
 */

#ifndef _MB_F_H
#define _MB_F_H

#include "port.h"

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

#include "mbport.h"
#include "mbproto.h"

#define MB_FIREWALL_TCP_PORT_USE_DEFAULT 0   

/* ----------------------- Type definitions ---------------------------------*/

#if !(defined (_MB_M_H) || defined (_MB_H))


typedef enum {
    MB_RTU,   /*!< RTU transmission mode. */
    MB_ASCII, /*!< ASCII transmission mode. */
    MB_TCP    /*!< TCP mode. */
} eMBMode;


typedef enum {
    MB_REG_READ, /*!< Read register values and pass to protocol stack. */
    MB_REG_WRITE /*!< Update register values. */
} eMBRegisterMode;

typedef enum {
    MB_ENOERR,                  /*!< no error. */
    MB_ENOREG,                  /*!< illegal register address. */
    MB_EINVAL,                  /*!< illegal argument. */
    MB_EPORTERR,                /*!< porting layer error. */
    MB_ENORES,                  /*!< insufficient resources. */
    MB_EIO,                     /*!< I/O error. */
    MB_EILLSTATE,               /*!< protocol stack in illegal state. */
    MB_ETIMEDOUT                /*!< timeout error occurred. */
} eMBErrorCode;

#endif

eMBErrorCode    eMBFirewallInit( eMBMode eMode,
                         UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity );

eMBErrorCode    eMBFirewallTCPInit( USHORT usTCPPort );

eMBErrorCode    eMBFirewallClose( void );

eMBErrorCode    eMBFirewallEnable( void );

eMBErrorCode    eMBFirewallDisable( void );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif
