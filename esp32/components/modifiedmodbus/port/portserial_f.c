/* Copyright 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * FreeModbus Libary: ESP32 Port Demo Application
 * Copyright (C) 2010 Christian Walter <cwalter@embedded-solutions.at>
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * IF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: $Id: portother.c,v 1.1 2010/06/06 13:07:20 wolti Exp $
 */
#include "port.h"
#include "driver/uart.h"
#include "freertos/queue.h" // for queue support
#include "soc/uart_periph.h"
#include "driver/gpio.h"
#include "esp_log.h"        // for esp_log
#include "esp_err.h"        // for ESP_ERROR_CHECK macro

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_f.h"
#include "mbport.h"
#include "sdkconfig.h"              // for KConfig options
#include "port_serial_firewall.h"

// Definitions of UART default pin numbers
#define MB_UART_INPUT_RXD   (CONFIG_MB_FIREWALL_UART_INPUT_RXD)
#define MB_UART_INPUT_TXD   (CONFIG_MB_FIREWALL_UART_INPUT_TXD)
#define MB_UART_INPUT_RTS   (CONFIG_MB_FIREWALL_UART_INPUT_RTS)

#define MB_UART_OUTPUT_RXD   (CONFIG_MB_FIREWALL_UART_OUTPUT_RXD)
#define MB_UART_OUTPUT_TXD   (CONFIG_MB_FIREWALL_UART_OUTPUT_TXD)
#define MB_UART_OUTPUT_RTS   (CONFIG_MB_FIREWALL_UART_OUTPUT_RTS)

#define MB_BAUD_RATE_DEFAULT        (115200)
#define MB_QUEUE_LENGTH             (CONFIG_FMB_QUEUE_LENGTH)

#define MB_SERIAL_TASK_PRIO         (CONFIG_FMB_SERIAL_TASK_PRIO)
#define MB_SERIAL_TASK_STACK_SIZE   (CONFIG_FMB_SERIAL_TASK_STACK_SIZE)
#define MB_SERIAL_TOUT              (3) // 3.5*8 = 28 ticks, TOUT=3 -> ~24..33 ticks

// Set buffer size for transmission
#define MB_SERIAL_BUF_SIZE          (CONFIG_FMB_SERIAL_BUF_SIZE)

// Note: This code uses mixed coding standard from legacy IDF code and used freemodbus stack

// A queue to handle UART event.
static QueueHandle_t xMbFirewallUartQueueInput;
static QueueHandle_t xMbFirewallUartQueueOutput;
static TaskHandle_t  xMbFirewallTaskHandleInput;
static TaskHandle_t  xMbFirewallTaskHandleOutput;

static const CHAR *TAG = "MB_FIREWALL_SERIAL";

// The UART hardware port numbers for input and output
static UCHAR ucUartNumberInput = UART_NUM_MAX - 1;
static UCHAR ucUartNumberOutput = UART_NUM_MAX - 2;

static BOOL bRxStateEnabledInput = FALSE; // Receiver enabled flag
static BOOL bTxStateEnabledInput = FALSE; // Transmitter enabled flag

static BOOL bRxStateEnabledOutput = FALSE; // Receiver enabled flag
static BOOL bTxStateEnabledOutput = FALSE; // Transmitter enabled flag

static UCHAR ucBufferInput[MB_SERIAL_BUF_SIZE]; // Temporary buffer to transfer received data to modbus stack
static UCHAR ucBufferOutput[MB_SERIAL_BUF_SIZE]; // Temporary buffer to transfer received data to modbus stack
static USHORT uiRxBufferPosInput = 0;    // position in the receiver buffer
static USHORT uiRxBufferPosOutput = 0;    // position in the receiver buffer

void vMBFirewallPortSerialEnable(BOOL bRxEnableInput, BOOL bTxEnableInput,
                                BOOL bRxEnableOutput, BOOL bTxEnableOutput)
{
    // This function can be called from xMBRTUTransmitFSM() of different task
    if (bRxEnableInput) {

        bRxStateEnabledInput = TRUE;
        vTaskResume(xMbFirewallTaskHandleInput); // Resume receiver task INPUT
    } else {
        vTaskSuspend(xMbFirewallTaskHandleInput); // Block receiver task
        bRxStateEnabledInput = FALSE;
    }

    if (bRxEnableOutput) {

        bRxStateEnabledOutput = TRUE;
        vTaskResume(xMbFirewallTaskHandleOutput); // Resume receiver task OUTPUT
    }
    else {
        vTaskSuspend(xMbFirewallTaskHandleOutput); // Block receiver task
        bRxStateEnabledOutput = FALSE;
    }
    if (bTxEnableInput) {
        bTxStateEnabledInput = TRUE;
    } else {
        bTxStateEnabledInput = FALSE;
    }

    if (bTxEnableOutput) {
        bTxStateEnabledOutput = TRUE;
    } else {
        bTxStateEnabledOutput = FALSE;
    }
}

static void vMBFirewallInputPortSerialRxPoll(size_t xEventSize)
{
    USHORT usLength;

    if (bRxStateEnabledInput) {
        if (xEventSize > 0) {
            xEventSize = (xEventSize > MB_SERIAL_BUF_SIZE) ?  MB_SERIAL_BUF_SIZE : xEventSize;
            uiRxBufferPosInput = ((uiRxBufferPosInput + xEventSize) >= MB_SERIAL_BUF_SIZE) ? 0 : uiRxBufferPosInput;
            // Get received packet into Rx buffer
            usLength = uart_read_bytes(ucUartNumberInput, &ucBufferInput[uiRxBufferPosInput], xEventSize, portMAX_DELAY);
            
            // TODO 
            /* MB frame callback logic need to be reimplemented */
            for(USHORT usCnt = 0; usCnt < usLength; usCnt++ ) {
                // Call the Modbus stack callback function and let it fill the buffers.
                ( void )pxMBFirewallInputFrameCBByteReceived(); // calls callback xMBRTUReceiveFSM() to execute MB state machine
            }

            // The buffer is transferred into Modbus stack and is not needed here any more
            uart_flush_input(ucUartNumberInput);
    
            // TODO
            // Send event EV_FRAME_RECEIVED to allow stack process packet
#ifndef MB_TIMER_PORT_ENABLED
            // Let the stack know that T3.5 time is expired and data is received
            (void)pxMBFirewallInputFrameCBByteReceived(); // calls callback xMBRTUTimerT35Expired();
#endif
            ESP_LOGD(TAG, "RX_T35_timeout: %d(bytes in buffer)\n", (uint32_t)usLength);
        }
    }
}

static void vMBFirewallOutputPortSerialRxPoll(size_t xEventSize)
{
    USHORT usLength;

    if (bRxStateEnabledOutput) {
        if (xEventSize > 0) {
            xEventSize = (xEventSize > MB_SERIAL_BUF_SIZE) ?  MB_SERIAL_BUF_SIZE : xEventSize;
            uiRxBufferPosOutput = ((uiRxBufferPosOutput + xEventSize) >= MB_SERIAL_BUF_SIZE) ? 0 : uiRxBufferPosOutput;
            // Get received packet into Rx buffer
            usLength = uart_read_bytes(ucUartNumberOutput, &ucBufferOutput[uiRxBufferPosOutput], xEventSize, portMAX_DELAY);
            
            // TODO 
            /* MB frame callback logic need to be reimplemented */
            // for(USHORT usCnt = 0; usCnt < usLength; usCnt++ ) {
            //     // Call the Modbus stack callback function and let it fill the buffers.
            //     ( void )pxMBFrameCBByteReceived(); // calls callback xMBRTUReceiveFSM() to execute MB state machine
            // }

            // The buffer is transferred into Modbus stack and is not needed here any more
            uart_flush_input(ucUartNumberOutput);
    
            // TODO
//             // Send event EV_FRAME_RECEIVED to allow stack process packet
// #ifndef MB_TIMER_PORT_ENABLED
//             // Let the stack know that T3.5 time is expired and data is received
//             (void)pxMBPortCBTimerExpired(); // calls callback xMBRTUTimerT35Expired();
// #endif
            ESP_LOGD(TAG, "RX_T35_timeout: %d(bytes in buffer)\n", (uint32_t)usLength);
        }
    }
}

BOOL xMBFirewallInputPortSerialTxPoll()
{
    BOOL bStatus = FALSE;
    USHORT usCount = 0;
    BOOL bNeedPoll = FALSE;

    if( bTxStateEnabledInput ) {
        // Continue while all response bytes put in buffer or out of buffer
        // TODO - rewrite logic here
        while((bNeedPoll == FALSE) && (usCount++ < MB_SERIAL_BUF_SIZE)) {
            // Calls the modbus stack callback function to let it fill the UART transmit buffer.
            bNeedPoll = pxMBFirewallInputFrameCBTransmitterEmpty( ); // calls callback xMBRTUTransmitFSM();
        }
        ESP_LOGD(TAG, "MB_TX_buffer sent: (%d) bytes\n", (uint16_t)usCount);
        bStatus = TRUE;
    }
    return bStatus;
}

BOOL xMBFirewallOutputPortSerialTxPoll()
{
    BOOL bStatus = FALSE;
    USHORT usCount = 0;
    BOOL bNeedPoll = FALSE;

    if( bTxStateEnabledOutput ) {
        // Continue while all response bytes put in buffer or out of buffer
        // TODO - rewrite logic here
        while((bNeedPoll == FALSE) && (usCount++ < MB_SERIAL_BUF_SIZE)) {
            // Calls the modbus stack callback function to let it fill the UART transmit buffer.
            // bNeedPoll = pxMBFrameCBTransmitterEmpty( ); // calls callback xMBRTUTransmitFSM();
        }
        ESP_LOGD(TAG, "MB_TX_buffer sent: (%d) bytes\n", (uint16_t)usCount);
        bStatus = TRUE;
    }
    return bStatus;
}

/* Input uart task */
static void vUartTaskInput(void *pvParameters)
{
    uart_event_t xEvent;
    for(;;) {
        if (xQueueReceive(xMbFirewallUartQueueInput, (void*)&xEvent, portMAX_DELAY) == pdTRUE) {
            ESP_LOGD(TAG, "MB_uart[%d] event:", ucUartNumberInput);
            //vMBPortTimersEnable();
            switch(xEvent.type) {
                //Event of UART receving data
                case UART_DATA:
                    ESP_LOGD(TAG,"Receive data, len: %d", xEvent.size);
                    // Read received data and send it to modbus stack
                    vMBFirewallInputPortSerialRxPoll(xEvent.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGD(TAG, "hw fifo overflow\n");
                    xQueueReset(xMbFirewallUartQueueInput);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGD(TAG, "ring buffer full\n");
                    xQueueReset(xMbFirewallUartQueueInput);
                    uart_flush_input(ucUartNumberInput);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGD(TAG, "uart rx break\n");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGD(TAG, "uart parity error\n");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGD(TAG, "uart frame error\n");
                    break;
                default:
                    ESP_LOGD(TAG, "uart event type: %d\n", xEvent.type);
                    break;
            }
        }
    }
    vTaskDelete(NULL);
}

/* Output uart task */
static void vUartTaskOutput(void *pvParameters)
{
    uart_event_t xEvent;
    for(;;) {
        if (xQueueReceive(xMbFirewallUartQueueOutput, (void*)&xEvent, portMAX_DELAY) == pdTRUE) {
            ESP_LOGD(TAG, "MB_uart[%d] event:", ucUartNumberOutput);
            //vMBPortTimersEnable();
            switch(xEvent.type) {
                //Event of UART receving data
                case UART_DATA:
                    ESP_LOGD(TAG,"Receive data, len: %d", xEvent.size);
                    // Read received data and send it to modbus stack
                    vMBFirewallOutputPortSerialRxPoll(xEvent.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGD(TAG, "hw fifo overflow\n");
                    xQueueReset(xMbFirewallUartQueueOutput);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGD(TAG, "ring buffer full\n");
                    xQueueReset(xMbFirewallUartQueueOutput);
                    uart_flush_input(ucUartNumberOutput);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGD(TAG, "uart rx break\n");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGD(TAG, "uart parity error\n");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGD(TAG, "uart frame error\n");
                    break;
                default:
                    ESP_LOGD(TAG, "uart event type: %d\n", xEvent.type);
                    break;
            }
        }
    }
    vTaskDelete(NULL);
}


BOOL xMBFirewallPortSerialInit(UCHAR ucPORTInput, ULONG ulBaudRateInput,
                        UCHAR ucDataBitsInput, eMBParity eParityInput,
                        UCHAR ucPORTOutput, ULONG ulBaudRateOutput,
                        UCHAR ucDataBitsOutput, eMBParity eParityOutput)
{
    esp_err_t xErr = ESP_OK;
    MB_PORT_CHECK((eParityInput <= MB_PAR_EVEN), FALSE, "mb serial set parity for input failed.");
    MB_PORT_CHECK((eParityOutput <= MB_PAR_EVEN), FALSE, "mb serial set parity for output failed.");
    // Set communication port number
    ucUartNumberInput = ucPORTInput;
    ucUartNumberOutput = ucPORTOutput;
 
    // Configure serial communication parameters
    UCHAR ucParityInput = UART_PARITY_DISABLE;
    UCHAR ucDataInput = UART_DATA_8_BITS;

    UCHAR ucParityOutput = UART_PARITY_DISABLE;
    UCHAR ucDataOutput = UART_DATA_8_BITS;

    switch(eParityInput){
        case MB_PAR_NONE:
            ucParityInput = UART_PARITY_DISABLE;
            break;
        case MB_PAR_ODD:
            ucParityInput = UART_PARITY_ODD;
            break;
        case MB_PAR_EVEN:
            ucParityInput = UART_PARITY_EVEN;
            break;
    }

    switch(eParityOutput){
        case MB_PAR_NONE:
            ucParityOutput = UART_PARITY_DISABLE;
            break;
        case MB_PAR_ODD:
            ucParityOutput = UART_PARITY_ODD;
            break;
        case MB_PAR_EVEN:
            ucParityOutput = UART_PARITY_EVEN;
            break;
    }

    switch(ucDataBitsInput){
        case 5:
            ucDataInput = UART_DATA_5_BITS;
            break;
        case 6:
            ucDataInput = UART_DATA_6_BITS;
            break;
        case 7:
            ucDataInput = UART_DATA_7_BITS;
            break;
        case 8:
            ucDataInput = UART_DATA_8_BITS;
            break;
        default:
            ucDataInput = UART_DATA_8_BITS;
            break;
    }

    switch(ucDataBitsOutput){
        case 5:
            ucDataOutput = UART_DATA_5_BITS;
            break;
        case 6:
            ucDataOutput = UART_DATA_6_BITS;
            break;
        case 7:
            ucDataOutput = UART_DATA_7_BITS;
            break;
        case 8:
            ucDataOutput = UART_DATA_8_BITS;
            break;
        default:
            ucDataOutput = UART_DATA_8_BITS;
            break;
    }

    uart_config_t xUartConfigInput = {
        .baud_rate = ulBaudRateInput,
        .data_bits = ucDataInput,
        .parity = ucParityInput,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 2,
    };

    uart_config_t xUartConfigOutput = {
        .baud_rate = ulBaudRateOutput,
        .data_bits = ucDataOutput,
        .parity = ucParityOutput,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 2,
    };

    // Set UART config
    xErr = uart_param_config(ucUartNumberInput, &xUartConfigInput);
    MB_PORT_CHECK((xErr == ESP_OK),
            FALSE, "mb input config failure, uart_param_config() returned (0x%x).", (uint32_t)xErr);
    
    xErr = uart_param_config(ucUartNumberOutput, &xUartConfigOutput);
    MB_PORT_CHECK((xErr == ESP_OK),
            FALSE, "mb output config failure, uart_param_config() returned (0x%x).", (uint32_t)xErr);

    // Install UART INPUT driver, and get the queue.
    xErr = uart_driver_install(ucUartNumberInput, MB_SERIAL_BUF_SIZE, MB_SERIAL_BUF_SIZE,
            MB_QUEUE_LENGTH, &xMbFirewallUartQueueInput, 0);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
            "mb input serial driver failure, uart_driver_install() returned (0x%x).", (uint32_t)xErr);

    // Install UART OUTPUT driver, and get the queue.
    xErr = uart_driver_install(ucUartNumberOutput, MB_SERIAL_BUF_SIZE, MB_SERIAL_BUF_SIZE,
            MB_QUEUE_LENGTH, &xMbFirewallUartQueueOutput, 0);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
            "mb output serial driver failure, uart_driver_install() returned (0x%x).", (uint32_t)xErr);

#ifndef MB_TIMER_PORT_ENABLED
    // Set timeout for TOUT interrupt (T3.5 modbus time)
    xErr = uart_set_rx_timeout(ucUartNumberInput, MB_SERIAL_TOUT);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
            "mb input serial set rx timeout failure, uart_set_rx_timeout() returned (0x%x).", (uint32_t)xErr);

    xErr = uart_set_rx_timeout(ucUartNumberOutput, MB_SERIAL_TOUT);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
            "mb output serial set rx timeout failure, uart_set_rx_timeout() returned (0x%x).", (uint32_t)xErr);
#endif

    // Create a task to handle UART events for INPUT
    BaseType_t xStatus = xTaskCreate(vUartTaskInput, "uart_queue_task_input", MB_SERIAL_TASK_STACK_SIZE,
                                        NULL, MB_SERIAL_TASK_PRIO, &xMbFirewallTaskHandleInput);
    if (xStatus != pdPASS) {
        vTaskDelete(xMbFirewallTaskHandleInput);
        // Force exit from function with failure
        MB_PORT_CHECK(FALSE, FALSE,
                "mb input stack serial task creation error. xTaskCreate() returned (0x%x).",
                (uint32_t)xStatus);
    } else {
        vTaskSuspend(xMbFirewallTaskHandleInput); // Suspend serial task while stack is not started
    }

    // Create a task to handle UART events for OUTPUT
    xStatus = xTaskCreate(vUartTaskOutput, "uart_queue_task_output", MB_SERIAL_TASK_STACK_SIZE,
                                        NULL, MB_SERIAL_TASK_PRIO, &xMbFirewallTaskHandleOutput);
    if (xStatus != pdPASS) {
        vTaskDelete(xMbFirewallTaskHandleOutput);
        // Force exit from function with failure
        MB_PORT_CHECK(FALSE, FALSE,
                "mb output stack serial task creation error. xTaskCreate() returned (0x%x).",
                (uint32_t)xStatus);
    } else {
        vTaskSuspend(xMbFirewallTaskHandleOutput); // Suspend serial task while stack is not started
    }
    uiRxBufferPosInput = 0;
    uiRxBufferPosOutput = 0;
    return TRUE;
}

void vMBFirewallPortSerialClose()
{
    (void)vTaskSuspend(xMbFirewallTaskHandleInput);
    (void)vTaskDelete(xMbFirewallTaskHandleInput);


    (void)vTaskSuspend(xMbFirewallTaskHandleOutput);
    (void)vTaskDelete(xMbFirewallTaskHandleOutput);

    ESP_ERROR_CHECK(uart_driver_delete(ucUartNumberInput));
    ESP_ERROR_CHECK(uart_driver_delete(ucUartNumberOutput));
}

BOOL xMBFirewallInputPortSerialPutByte(CHAR ucByte)
{
    // Send one byte to UART transmission buffer
    // This function is called by Modbus stack
    UCHAR ucLength = uart_write_bytes(ucUartNumberInput, &ucByte, 1);
    return (ucLength == 1);
}

BOOL xMBFirewallOutputPortSerialPutByte(CHAR ucByte)
{
    // Send one byte to UART transmission buffer
    // This function is called by Modbus stack
    UCHAR ucLength = uart_write_bytes(ucUartNumberOutput, &ucByte, 1);
    return (ucLength == 1);
}

// Get one byte from intermediate input RX buffer
BOOL xMBFirewallInputPortSerialGetByte(CHAR* pucByte)
{
    assert(pucByte != NULL);
    MB_PORT_CHECK((uiRxBufferPosInput < MB_SERIAL_BUF_SIZE),
            FALSE, "mb stack serial get byte failure.");
    *pucByte = ucBufferInput[uiRxBufferPosInput];
    uiRxBufferPosInput++;
    return TRUE;
}

// Get one byte from intermediate output RX buffer
BOOL xMBFirewallOutputPortSerialGetByte(CHAR* pucByte)
{
    assert(pucByte != NULL);
    MB_PORT_CHECK((uiRxBufferPosOutput < MB_SERIAL_BUF_SIZE),
            FALSE, "mb stack serial get byte failure.");
    *pucByte = ucBufferOutput[uiRxBufferPosOutput];
    uiRxBufferPosOutput++;
    return TRUE;
}

