/*
 * Ursescu Ionut
 * Modbus firewall entry point.
 */

#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "mbcontroller.h"

#define MB_PORT_NUM_IN (2)     // Number of UART port used for Modbus IN connection
#define MB_PORT_NUM_OUT (3)    // Number of UART port used for Modbus OUT connection
#define MB_DEV_SPEED (115200)  // The communication speed of the UART

static const char* TAG = "MODBUS_FIREWALL";

void app_main() {
    /* Set esp log level */
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    printf("Hello there here is the firewall main app.\n");

    /* Firewall handler */
    void *handler = NULL;
    mb_port_type_t port_type = MB_PORT_SERIAL_SLAVE;

    ESP_ERROR_CHECK(mbc_firewall_init(port_type, &handler));
    ESP_LOGI(TAG, "Firewall started\n");

    void *com_info = NULL;

    ESP_ERROR_CHECK(mbc_firewall_setup(com_info));


    ESP_ERROR_CHECK(mbc_firewall_start());


    ESP_ERROR_CHECK(mbc_firewall_destroy());


    ESP_LOGI(TAG, "Firewall distroyed\n");
}