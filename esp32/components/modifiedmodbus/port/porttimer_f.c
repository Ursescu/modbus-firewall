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
/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb_f.h"
#include "mbport.h"
#include "driver/timer.h"
#include "sdkconfig.h"
#include "port_serial_firewall.h"

#ifdef CONFIG_FMB_TIMER_PORT_ENABLED

#define MB_US50_FREQ     (20000)  // 20kHz 1/20000 = 50mks
#define MB_DISCR_TIME_US (50)     // 50uS = one discreet for timer

#define MB_TIMER_PRESCALLER  ((TIMER_BASE_CLK / MB_US50_FREQ) - 1);
#define MB_TIMER_SCALE       (TIMER_BASE_CLK / TIMER_DIVIDER)                       // convert counter value to seconds
#define MB_TIMER_DIVIDER     ((TIMER_BASE_CLK / 1000000UL) * MB_DISCR_TIME_US - 1)  // divider for 50uS
#define MB_TIMER_WITH_RELOAD (1)

static const USHORT usTimerIndexInput = CONFIG_FMB_TIMER_INDEX;       // Modbus Timer index used by firewall input
static const USHORT usTimerIndexOutput = CONFIG_FMB_TIMER_INDEX + 1;  // Modbus Timer index used by firewall output
static const USHORT usTimerGroupIndex = CONFIG_FMB_TIMER_GROUP;       // Modbus Timer group index used by stack

static timg_dev_t *MB_TG[2] = {&TIMERG0, &TIMERG1};

/* ----------------------- Start implementation -----------------------------*/
static void IRAM_ATTR vTimerGroupIsrInput(void *param) {
    // Retrieve the interrupt status and the counter value
    // from the timer that reported the interrupt
    uint32_t intr_status = MB_TG[usTimerGroupIndex]->int_st_timers.val;
    if (intr_status & BIT(usTimerIndexInput)) {
        MB_TG[usTimerGroupIndex]->int_clr_timers.val |= BIT(usTimerIndexInput);
        (void)pxMBFirewallInputPortCBTimerExpired();  // Timer callback function
        MB_TG[usTimerGroupIndex]->hw_timer[usTimerIndexInput].config.alarm_en = TIMER_ALARM_EN;
    }
}

static void IRAM_ATTR vTimerGroupIsrOutput(void *param) {
    // Retrieve the interrupt status and the counter value
    // from the timer that reported the interrupt
    uint32_t intr_status = MB_TG[usTimerGroupIndex]->int_st_timers.val;
    if (intr_status & BIT(usTimerIndexOutput)) {
        MB_TG[usTimerGroupIndex]->int_clr_timers.val |= BIT(usTimerIndexOutput);
        (void)pxMBFirewallOutputPortCBTimerExpired();  // Timer callback function
        MB_TG[usTimerGroupIndex]->hw_timer[usTimerIndexOutput].config.alarm_en = TIMER_ALARM_EN;
    }
}
#endif

BOOL xMBFirewallPortTimersInit(USHORT usTim1Timerout50usInput, USHORT usTim1Timerout50usOutput) {
#ifdef CONFIG_FMB_TIMER_PORT_ENABLED
    MB_PORT_CHECK((usTim1Timerout50usInput > 0), FALSE,
                  "Modbus timeout discreet is incorrect.");

    MB_PORT_CHECK((usTim1Timerout50usOutput > 0), FALSE,
                  "Modbus timeout discreet is incorrect.");

    esp_err_t xErr;
    timer_config_t configInput, configOutput;

    configInput.alarm_en = TIMER_ALARM_EN;
    configInput.auto_reload = MB_TIMER_WITH_RELOAD;
    configInput.counter_dir = TIMER_COUNT_UP;
    configInput.divider = MB_TIMER_PRESCALLER;
    configInput.intr_type = TIMER_INTR_LEVEL;
    configInput.counter_en = TIMER_PAUSE;

    configOutput.alarm_en = TIMER_ALARM_EN;
    configOutput.auto_reload = MB_TIMER_WITH_RELOAD;
    configOutput.counter_dir = TIMER_COUNT_UP;
    configOutput.divider = MB_TIMER_PRESCALLER;
    configOutput.intr_type = TIMER_INTR_LEVEL;
    configOutput.counter_en = TIMER_PAUSE;

    // Configure timer
    xErr = timer_init(usTimerGroupIndex, usTimerIndexInput, &configInput);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "timer init failure, timer_init() returned (0x%x).", (uint32_t)xErr);
    // Stop timer counter
    xErr = timer_pause(usTimerGroupIndex, usTimerIndexInput);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "stop timer failure, timer_pause() returned (0x%x).", (uint32_t)xErr);
    // Reset counter value
    xErr = timer_set_counter_value(usTimerGroupIndex, usTimerIndexInput, 0x00000000ULL);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "timer set value failure, timer_set_counter_value() returned (0x%x).",
                  (uint32_t)xErr);
    // wait3T5_us = 35 * 11 * 100000 / baud; // the 3.5T symbol time for baudrate
    // Set alarm value for usTim1Timerout50us * 50uS
    xErr = timer_set_alarm_value(usTimerGroupIndex, usTimerIndexInput, (uint32_t)(usTim1Timerout50usInput));
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "failure to set alarm failure, timer_set_alarm_value() returned (0x%x).",
                  (uint32_t)xErr);
    // Register ISR for timer
    xErr = timer_isr_register(usTimerGroupIndex, usTimerIndexInput, vTimerGroupIsrInput, NULL, ESP_INTR_FLAG_IRAM, NULL);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "timer set value failure, timer_isr_register() returned (0x%x).",
                  (uint32_t)xErr);

    // Configure timer
    xErr = timer_init(usTimerGroupIndex, usTimerIndexOutput, &configOutput);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "timer init failure, timer_init() returned (0x%x).", (uint32_t)xErr);
    // Stop timer counter
    xErr = timer_pause(usTimerGroupIndex, usTimerIndexOutput);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "stop timer failure, timer_pause() returned (0x%x).", (uint32_t)xErr);
    // Reset counter value
    xErr = timer_set_counter_value(usTimerGroupIndex, usTimerIndexOutput, 0x00000000ULL);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "timer set value failure, timer_set_counter_value() returned (0x%x).",
                  (uint32_t)xErr);
    // wait3T5_us = 35 * 11 * 100000 / baud; // the 3.5T symbol time for baudrate
    // Set alarm value for usTim1Timerout50us * 50uS
    xErr = timer_set_alarm_value(usTimerGroupIndex, usTimerIndexOutput, (uint32_t)(usTim1Timerout50usOutput));
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "failure to set alarm failure, timer_set_alarm_value() returned (0x%x).",
                  (uint32_t)xErr);
    // Register ISR for timer
    xErr = timer_isr_register(usTimerGroupIndex, usTimerIndexOutput, vTimerGroupIsrOutput, NULL, ESP_INTR_FLAG_IRAM, NULL);
    MB_PORT_CHECK((xErr == ESP_OK), FALSE,
                  "timer set value failure, timer_isr_register() returned (0x%x).",
                  (uint32_t)xErr);
#endif
    return TRUE;
}

void vMBFirewallInputPortTimersEnable() {
#ifdef CONFIG_FMB_TIMER_PORT_ENABLED
    ESP_ERROR_CHECK(timer_pause(usTimerGroupIndex, usTimerIndexInput));
    ESP_ERROR_CHECK(timer_set_counter_value(usTimerGroupIndex, usTimerIndexInput, 0ULL));
    ESP_ERROR_CHECK(timer_enable_intr(usTimerGroupIndex, usTimerIndexInput));
    ESP_ERROR_CHECK(timer_start(usTimerGroupIndex, usTimerIndexInput));
#endif
}

void vMBFirewallOutputPortTimersEnable() {
#ifdef CONFIG_FMB_TIMER_PORT_ENABLED
    ESP_ERROR_CHECK(timer_pause(usTimerGroupIndex, usTimerIndexOutput));
    ESP_ERROR_CHECK(timer_set_counter_value(usTimerGroupIndex, usTimerIndexOutput, 0ULL));
    ESP_ERROR_CHECK(timer_enable_intr(usTimerGroupIndex, usTimerIndexOutput));
    ESP_ERROR_CHECK(timer_start(usTimerGroupIndex, usTimerIndexOutput));
#endif
}

void vMBFirewallInputPortTimersDisable() {
#ifdef CONFIG_FMB_TIMER_PORT_ENABLED
    ESP_ERROR_CHECK(timer_pause(usTimerGroupIndex, usTimerIndexInput));
    ESP_ERROR_CHECK(timer_set_counter_value(usTimerGroupIndex, usTimerIndexInput, 0ULL));
    // Disable timer interrupt
    ESP_ERROR_CHECK(timer_disable_intr(usTimerGroupIndex, usTimerIndexInput));
#endif
}

void vMBFirewallOutputPortTimersDisable() {
#ifdef CONFIG_FMB_TIMER_PORT_ENABLED
    ESP_ERROR_CHECK(timer_pause(usTimerGroupIndex, usTimerIndexOutput));
    ESP_ERROR_CHECK(timer_set_counter_value(usTimerGroupIndex, usTimerIndexOutput, 0ULL));
    // Disable timer interrupt
    ESP_ERROR_CHECK(timer_disable_intr(usTimerGroupIndex, usTimerIndexOutput));
#endif
}

void vMBFirewallInputPortTimerClose() {
#ifdef CONFIG_FMB_TIMER_PORT_ENABLED
    ESP_ERROR_CHECK(timer_pause(usTimerGroupIndex, usTimerIndexInput));
    ESP_ERROR_CHECK(timer_disable_intr(usTimerGroupIndex, usTimerIndexInput));
#endif
}

void vMBFirewallOutputPortTimerClose() {
#ifdef CONFIG_FMB_TIMER_PORT_ENABLED
    ESP_ERROR_CHECK(timer_pause(usTimerGroupIndex, usTimerIndexOutput));
    ESP_ERROR_CHECK(timer_disable_intr(usTimerGroupIndex, usTimerIndexOutput));
#endif
}
