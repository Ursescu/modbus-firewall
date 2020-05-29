/*
 * Ursescu Ionut
 * Modbus firewall entry point.
 */

#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_log.h"
#include "mbcontroller.h"
#include "firewall.h"

#define MB_PORT_NUM_IN (UART_NUM_MAX - 1)   // Number of UART port used for Modbus IN connection
#define MB_PORT_NUM_OUT (UART_NUM_MAX - 2)  // Number of UART port used for Modbus OUT connection
#define MB_DEV_SPEED (115200)               // The communication speed of the UART


static const char *TAG = "MAIN";

void app_main() {
    /* Firewall handler */
    void *handler = NULL;

    ESP_ERROR_CHECK(mbc_firewall_init(MB_PORT_SERIAL_FIREWALL, &handler));

    /* Cannot make it generic, different from modbus slave and master */
    mb_firewall_comm_info_t firewall_comm_info = {
        .mode_input = MB_MODE_RTU,       /* Modbus communication mode INPUT */
        .mode_output = MB_MODE_RTU,      /* Modbus communication mode OUTPUT */
        .port_input = MB_PORT_NUM_IN,    /* Modbus communication port (UART) INPUT number */
        .port_output = MB_PORT_NUM_OUT,  /* Modbus communication port (UART) OUTPUT number */
        .baudrate_input = MB_DEV_SPEED,  /* Modbus baudrate INTPUT */
        .baudrate_output = MB_DEV_SPEED, /* Modbus baudrate OUTPUT */
        .parity_input = MB_PARITY_NONE,  /* Modbus UART parity settings INPUT */
        .parity_output = MB_PARITY_NONE, /* Modbus UART parity settings OUTPUT */
        .packet_handler = &mb_firewall_cb, /* Modbus firewall filter function */
    };

    /* Setting up the firewall info */
    ESP_ERROR_CHECK(mbc_firewall_setup(&firewall_comm_info));

    /* Setup the ports */
    ESP_ERROR_CHECK(uart_set_pin(MB_PORT_NUM_IN, CONFIG_MB_FIREWALL_IN_UART_TXD,
                                 CONFIG_MB_FIREWALL_IN_UART_RXD, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(uart_set_pin(MB_PORT_NUM_OUT, CONFIG_MB_FIREWALL_OUT_UART_TXD,
                                 CONFIG_MB_FIREWALL_OUT_UART_RXD, UART_PIN_NO_CHANGE,
                                 UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(mbc_firewall_start());
    ESP_LOGI(TAG, "Firewall started");

    // ESP_ERROR_CHECK(mbc_firewall_destroy());

    // ESP_LOGI(TAG, "Firewall distroyed\n");
}