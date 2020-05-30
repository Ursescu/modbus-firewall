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
 * File: $Id: mbrtu.c,v 1.18 2007/09/12 10:15:56 wolti Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_f.h"
#include "mbrtu.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN 4   /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX 256 /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC 2   /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF 0   /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF  1   /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/
typedef enum {
    STATE_I_RX_INIT,  /*!< Receiver is in initial state. */
    STATE_I_RX_IDLE,  /*!< Receiver is in idle state. */
    STATE_I_RX_RCV,   /*!< Frame is beeing received. */
    STATE_I_RX_ERROR, /*!< If the frame is invalid. */
    STATE_O_RX_INIT,
    STATE_O_RX_IDLE,
    STATE_O_RX_RCV,
    STATE_O_RX_ERROR
} eMBRcvState;

typedef enum {
    STATE_I_TX_IDLE, /*!< Transmitter is in idle state. */
    STATE_I_TX_XMIT, /*!< Transmitter is in transfer state. */
    STATE_O_TX_IDLE, /*!< Transmitter is in transfer state. */
    STATE_O_TX_XMIT  /*!< Transmitter is in transfer state. */
} eMBSndState;

/* ----------------------- Static variables ---------------------------------*/
static volatile eMBSndState eSndStateInput;
static volatile eMBRcvState eRcvStateInput;

static volatile eMBSndState eSndStateOutput;
static volatile eMBRcvState eRcvStateOutput;

volatile UCHAR ucRTUBufInput[MB_SER_PDU_SIZE_MAX];

volatile UCHAR ucRTUBufOutput[MB_SER_PDU_SIZE_MAX];

static volatile UCHAR *pucSndBufferCurInput;
static volatile UCHAR *pucSndBufferCurOutput;

static volatile USHORT usSndBufferCountInput;
static volatile USHORT usSndBufferCountOutput;

static volatile USHORT usRcvBufferPosInput;
static volatile USHORT usRcvBufferPosOutput;

/* ----------------------- Start implementation -----------------------------*/
eMBErrorCode
eMBFirewallRTUInit(UCHAR ucPortInput, ULONG ulBaudRateInput, eMBParity eParityInput,
                   UCHAR ucPortOutput, ULONG ulBaudRateOutput, eMBParity eParityOutput) {
    eMBErrorCode eStatus = MB_ENOERR;
    ULONG usTimerT35_50usInput;
    ULONG usTimerT35_50usOutput;

    ENTER_CRITICAL_SECTION();

    /* Modbus RTU uses 8 Databits. */
    if (xMBFirewallPortSerialInit(ucPortInput, ulBaudRateInput, 8, eParityInput, ucPortOutput, ulBaudRateOutput, 8, eParityOutput) != TRUE) {
        eStatus = MB_EPORTERR;
    } else {
        // TODO - add timer for second UART

        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if (ulBaudRateInput > 19200) {
            usTimerT35_50usInput = 35; /* 1800us. */
        } else {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50usInput = (7UL * 220000UL) / (2UL * ulBaudRateInput);
        }

        if (ulBaudRateOutput > 19200) {
            usTimerT35_50usOutput = 35; /* 1800us. */
        } else {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50usOutput = (7UL * 220000UL) / (2UL * ulBaudRateOutput);
        }
        if (xMBFirewallPortTimersInit((USHORT)usTimerT35_50usInput, (USHORT)usTimerT35_50usOutput) != TRUE) {
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION();

    return eStatus;
}

void eMBFirewallRTUStart(void) {
    ENTER_CRITICAL_SECTION();
    /* Initially the receiver is in the state STATE_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
    eRcvStateInput = STATE_I_RX_INIT;
    eRcvStateOutput = STATE_O_RX_INIT;
    vMBFirewallInputPortSerialEnable(TRUE, FALSE);
    vMBFirewallOutputPortSerialEnable(FALSE, FALSE);
    vMBFirewallInputPortTimersEnable();
    vMBFirewallOutputPortTimersEnable();

    EXIT_CRITICAL_SECTION();
}

void eMBFirewallRTUStop(void) {
    ENTER_CRITICAL_SECTION();
    vMBFirewallInputPortSerialEnable(FALSE, FALSE);
    vMBFirewallOutputPortSerialEnable(FALSE, FALSE);
    vMBFirewallInputPortTimersDisable();
    vMBFirewallOutputPortTimersDisable();
    EXIT_CRITICAL_SECTION();
}

eMBErrorCode
eMBFirewallInputRTUReceive(UCHAR *pucRcvAddress, UCHAR **pucFrame, USHORT *pusLength) {
    eMBErrorCode eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION();
    assert(usRcvBufferPosInput < MB_SER_PDU_SIZE_MAX);

    /* Length and CRC check */
    if ((usRcvBufferPosInput >= MB_SER_PDU_SIZE_MIN) && (usMBCRC16((UCHAR *)ucRTUBufInput, usRcvBufferPosInput) == 0)) {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = ucRTUBufInput[MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = (USHORT)(usRcvBufferPosInput - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC);

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = (UCHAR *)&ucRTUBufInput[MB_SER_PDU_PDU_OFF];
    } else {
        eStatus = MB_EIO;
    }

    EXIT_CRITICAL_SECTION();
    return eStatus;
}

eMBErrorCode
eMBFirewallOutputRTUReceive(UCHAR *pucRcvAddress, UCHAR **pucFrame, USHORT *pusLength) {
    eMBErrorCode eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION();
    assert(usRcvBufferPosOutput < MB_SER_PDU_SIZE_MAX);

    /* Length and CRC check */
    if ((usRcvBufferPosOutput >= MB_SER_PDU_SIZE_MIN) && (usMBCRC16((UCHAR *)ucRTUBufOutput, usRcvBufferPosOutput) == 0)) {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = ucRTUBufOutput[MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = (USHORT)(usRcvBufferPosOutput - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC);

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = (UCHAR *)&ucRTUBufOutput[MB_SER_PDU_PDU_OFF];
    } else {
        eStatus = MB_EIO;
    }

    EXIT_CRITICAL_SECTION();
    return eStatus;
}

eMBErrorCode
eMBFirewallInputRTUSend(UCHAR ucSlaveAddress, const UCHAR *pucFrame, USHORT usLength) {
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT usCRC16;

    ENTER_CRITICAL_SECTION();

    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if (eRcvStateInput == STATE_I_RX_IDLE) {
        /* First byte before the Modbus-PDU is the slave address. */
        pucSndBufferCurInput = (UCHAR *)pucFrame - 1;
        usSndBufferCountInput = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pucSndBufferCurInput[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usSndBufferCountInput += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = usMBCRC16((UCHAR *)pucSndBufferCurInput, usSndBufferCountInput);
        ucRTUBufInput[usSndBufferCountInput++] = (UCHAR)(usCRC16 & 0xFF);
        ucRTUBufInput[usSndBufferCountInput++] = (UCHAR)(usCRC16 >> 8);

        /* Activate the transmitter. */
        eSndStateInput = STATE_I_TX_XMIT;
        vMBFirewallInputPortSerialEnable(FALSE, TRUE);
    } else {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();
    return eStatus;
}

eMBErrorCode
eMBFirewallOutputRTUSend(UCHAR ucSlaveAddress, const UCHAR *pucFrame, USHORT usLength) {
    eMBErrorCode eStatus = MB_ENOERR;
    USHORT usCRC16;

    ENTER_CRITICAL_SECTION();

    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if (eRcvStateOutput == STATE_O_RX_IDLE) {
        /* First byte before the Modbus-PDU is the slave address. */
        pucSndBufferCurOutput = (UCHAR *)pucFrame - 1;
        usSndBufferCountOutput = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        pucSndBufferCurOutput[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        usSndBufferCountOutput += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = usMBCRC16((UCHAR *)pucSndBufferCurOutput, usSndBufferCountOutput);
        ucRTUBufOutput[usSndBufferCountOutput++] = (UCHAR)(usCRC16 & 0xFF);
        ucRTUBufOutput[usSndBufferCountOutput++] = (UCHAR)(usCRC16 >> 8);

        /* Activate the transmitter. */
        eSndStateOutput = STATE_O_TX_XMIT;
        vMBFirewallOutputPortSerialEnable(FALSE, TRUE);
    } else {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION();
    return eStatus;
}

BOOL xMBFirewallInputRTUReceiveFSM(void) {
    BOOL xTaskNeedSwitch = FALSE;
    UCHAR ucByte;

    assert(eSndStateInput == STATE_I_TX_IDLE);

    /* Always read the character. */
    (void)xMBFirewallInputPortSerialGetByte((CHAR *)&ucByte);

    switch (eRcvStateInput) {
            /* If we have received a character in the init state we have to
         * wait until the frame is finished.
         */
        case STATE_I_RX_INIT:
            vMBFirewallInputPortTimersEnable();
            break;

            /* In the error state we wait until all characters in the
         * damaged frame are transmitted.
         */
        case STATE_I_RX_ERROR:
            vMBFirewallInputPortTimersEnable();
            break;

            /* In the idle state we wait for a new character. If a character
         * is received the t1.5 and t3.5 timers are started and the
         * receiver is in the state STATE_RX_RECEIVCE.
         */
        case STATE_I_RX_IDLE:
            usRcvBufferPosInput = 0;
            ucRTUBufInput[usRcvBufferPosInput++] = ucByte;
            eRcvStateInput = STATE_I_RX_RCV;

            /* Enable t3.5 timers. */
            vMBFirewallInputPortTimersEnable();
            break;

            /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
        case STATE_I_RX_RCV:
            if (usRcvBufferPosInput < MB_SER_PDU_SIZE_MAX) {
                ucRTUBufInput[usRcvBufferPosInput++] = ucByte;
            } else {
                eRcvStateInput = STATE_I_RX_ERROR;
            }
            vMBFirewallInputPortTimersEnable();
            break;
        default:
            break;
    }
    return xTaskNeedSwitch;
}

/* Receive the modbus response from output port */
BOOL xMBFirewallOutputRTUReceiveFSM(void) {
    BOOL xTaskNeedSwitch = FALSE;
    UCHAR ucByte;

    assert(eSndStateOutput == STATE_O_TX_IDLE);

    /* Always read the character. */
    (void)xMBFirewallOutputPortSerialGetByte((CHAR *)&ucByte);

    switch (eRcvStateOutput) {
            /* If we have received a character in the init state we have to
         * wait until the frame is finished.
         */
        case STATE_O_RX_INIT:
            vMBFirewallOutputPortTimersEnable();
            break;

            /* In the error state we wait until all characters in the
         * damaged frame are transmitted.
         */
        case STATE_O_RX_ERROR:
            vMBFirewallOutputPortTimersEnable();
            break;

            /* In the idle state we wait for a new character. If a character
         * is received the t1.5 and t3.5 timers are started and the
         * receiver is in the state STATE_RX_RECEIVCE.
         */
        case STATE_O_RX_IDLE:
            usRcvBufferPosOutput = 0;
            ucRTUBufOutput[usRcvBufferPosOutput++] = ucByte;
            eRcvStateOutput = STATE_O_RX_RCV;

            /* Enable t3.5 timers. */
            vMBFirewallOutputPortTimersEnable();
            break;

            /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
        case STATE_O_RX_RCV:
            if (usRcvBufferPosOutput < MB_SER_PDU_SIZE_MAX) {
                ucRTUBufOutput[usRcvBufferPosOutput++] = ucByte;
            } else {
                eRcvStateOutput = STATE_O_RX_ERROR;
            }
            vMBFirewallOutputPortTimersEnable();
            break;
        default:
            break;
    }
    return xTaskNeedSwitch;
}

BOOL xMBFirewallInputRTUTransmitFSM(void) {
    BOOL xNeedPoll = FALSE;

    assert(eRcvStateInput == STATE_I_RX_IDLE);

    switch (eSndStateInput) {
            /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
        case STATE_I_TX_IDLE:
            /* enable receiver/disable transmitter. */
            vMBFirewallInputPortSerialEnable(TRUE, FALSE);
            break;

        case STATE_I_TX_XMIT:
            /* check if we are finished. */
            if (usSndBufferCountInput != 0) {
                xMBFirewallInputPortSerialPutByte((CHAR)*pucSndBufferCurInput);
                pucSndBufferCurInput++; /* next byte in sendbuffer. */
                usSndBufferCountInput--;
            } else {
                xNeedPoll = xMBFirewallPortEventPost(EV_F_INPUT_FRAME_SENT);
                /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
                vMBFirewallInputPortSerialEnable(TRUE, FALSE);
                eSndStateInput = STATE_I_TX_IDLE;
            }
            break;
        default:
            break;
    }

    return xNeedPoll;
}

BOOL xMBFirewallOutputRTUTransmitFSM(void) {
    BOOL xNeedPoll = FALSE;

    assert(eRcvStateOutput == STATE_O_RX_IDLE);

    switch (eSndStateOutput) {
            /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
        case STATE_O_TX_IDLE:
            /* enable receiver/disable transmitter. */
            vMBFirewallOutputPortSerialEnable(TRUE, FALSE);
            break;

        case STATE_O_TX_XMIT:
            /* check if we are finished. */
            if (usSndBufferCountOutput != 0) {
                xMBFirewallOutputPortSerialPutByte((CHAR)*pucSndBufferCurOutput);
                pucSndBufferCurOutput++; /* next byte in sendbuffer. */
                usSndBufferCountOutput--;
            } else {
                xNeedPoll = xMBFirewallPortEventPost(EV_F_OUTPUT_FRAME_SENT);
                /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
                vMBFirewallOutputPortSerialEnable(TRUE, FALSE);
                eSndStateOutput = STATE_O_TX_IDLE;
            }
            break;
        default:
            break;
    }

    return xNeedPoll;
}

BOOL xMBFirewallInputRTUTimerT35Expired(void) {
    BOOL xNeedPoll = FALSE;

    switch (eRcvStateInput) {
            /* Timer t35 expired. Startup phase is finished. */
        case STATE_I_RX_INIT:
            xNeedPoll = xMBFirewallPortEventPost(EV_F_READY);
            break;

            /* A frame was received and t35 expired. Notify the listener that
         * a new frame was received. */
        case STATE_I_RX_RCV:
            xNeedPoll = xMBFirewallPortEventPost(EV_F_INPUT_FRAME_RECEIVED);
            break;

            /* An error occured while receiving the frame. */
        case STATE_I_RX_ERROR:
            break;

            /* Function called in an illegal state. */
        default:
            assert((eRcvStateInput == STATE_I_RX_INIT) ||
                   (eRcvStateInput == STATE_I_RX_RCV) || (eRcvStateInput == STATE_I_RX_ERROR));
    }

    vMBFirewallInputPortTimersDisable();
    eRcvStateInput = STATE_I_RX_IDLE;

    return xNeedPoll;
}

BOOL xMBFirewallOutputRTUTimerT35Expired(void) {
    BOOL xNeedPoll = FALSE;

    switch (eRcvStateOutput) {
            /* Timer t35 expired. Startup phase is finished. */
        case STATE_O_RX_INIT:
            xNeedPoll = xMBFirewallPortEventPost(EV_F_READY);
            break;

            /* A frame was received and t35 expired. Notify the listener that
         * a new frame was received. */
        case STATE_O_RX_RCV:
            xNeedPoll = xMBFirewallPortEventPost(EV_F_OUTPUT_FRAME_RECEIVED);
            break;

            /* An error occured while receiving the frame. */
        case STATE_O_RX_ERROR:
            break;

            /* Function called in an illegal state. */
        default:
            assert((eRcvStateOutput == STATE_O_RX_INIT) ||
                   (eRcvStateOutput == STATE_O_RX_RCV) || (eRcvStateOutput == STATE_O_RX_ERROR));
    }

    vMBFirewallOutputPortTimersDisable();
    eRcvStateOutput = STATE_O_RX_IDLE;

    return xNeedPoll;
}
