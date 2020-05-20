#ifndef _MODBUS_SERIAL_CONTROLLER_FIREWALL
#define _MODBUS_SERIAL_CONTROLLER_FIREWALL

#include <stdint.h>                 // for standard int types definition
#include <stddef.h>                 // for NULL and std defines
#include "esp_modbus_common.h"      // for common defines

/* ----------------------- Defines ------------------------------------------*/
#define MB_CONTROLLER_NOTIFY_QUEUE_SIZE     (CONFIG_FMB_CONTROLLER_NOTIFY_QUEUE_SIZE) // Number of messages in parameter notification queue
#define MB_CONTROLLER_NOTIFY_TIMEOUT        (pdMS_TO_TICKS(CONFIG_FMB_CONTROLLER_NOTIFY_TIMEOUT)) // notification timeout

/*
 * @brief Initialize Modbus controller and stack
 *
 * @param[out] handler handler(pointer) to master data structure
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_NO_MEM Parameter error
 */
esp_err_t mbc_serial_firewall_create(mb_port_type_t port_type, void** handler);

#endif // _MODBUS_SERIAL_CONTROLLER_SLAVE