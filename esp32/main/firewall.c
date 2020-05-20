/*
 * Ursescu Ionut
 * Modbus firewall entry point.
 */

#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_log.h"

const char* TAG = "MODBUS_FIREWALL";

void app_main() {
    /* Set esp log level */

    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    ESP_LOGI(TAG, "Firewall started\n");

    printf("Hello there here is the firewall main app.\n");

    ESP_LOGI(TAG, "Firewall distroyed\n");
}