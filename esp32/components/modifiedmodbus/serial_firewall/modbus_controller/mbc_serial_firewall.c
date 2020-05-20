#include <sys/time.h>             // for calculation of time stamp in milliseconds
#include "esp_log.h"              // for log_write
#include "mb.h"                   // for mb types definition
#include "mbutils.h"              // for mbutils functions definition for stack callback
#include "sdkconfig.h"            // for KConfig values
#include "esp_modbus_common.h"    // for common defines
#include "esp_modbus_firewall.h"  // for public firewall interface types
#include "mbc_slave.h"            // for private slave interface types
#include "mbc_serial_firewall.h"  // for serial firewall implementation definitions
#include "port_serial_firewall.h"

esp_err_t mbc_serial_firewall_create(mb_port_type_t port) {
    return ESP_OK;
}