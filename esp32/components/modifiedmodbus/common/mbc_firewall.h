#ifndef _MB_CONTROLLER_FIREWALL_H
#define _MB_CONTROLLER_FIREWALL_H

#include "freertos/FreeRTOS.h"      // for task creation and queue access
#include "freertos/task.h"          // for task api access
#include "freertos/event_groups.h"  // for event groups
#include "driver/uart.h"            // for UART types
#include "errno.h"                  // for errno
#include "esp_log.h"                // for log write
#include "string.h"                 // for strerror()
#include "esp_modbus_common.h"      // for common types
#include "esp_modbus_firewall.h"    // for public firewall types
#include "esp_modbus_callbacks.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_INST_MIN_SIZE (2)          // The minimal size of Modbus registers area in bytes
#define MB_INST_MAX_SIZE (65535 * 2)  // The maximum size of Modbus area in bytes

#define MB_CONTROLLER_NOTIFY_QUEUE_SIZE (CONFIG_FMB_CONTROLLER_NOTIFY_QUEUE_SIZE)              // Number of messages in parameter notification queue
#define MB_CONTROLLER_NOTIFY_TIMEOUT    (pdMS_TO_TICKS(CONFIG_FMB_CONTROLLER_NOTIFY_TIMEOUT))  // notification timeout

#define MB_FIREWALL_TAG "MB_CONTROLLER_FIREWALL"

#define MB_FIREWALL_CHECK(a, ret_val, str, ...)                                           \
    if (!(a)) {                                                                           \
        ESP_LOGE(MB_FIREWALL_TAG, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        return (ret_val);                                                                 \
    }

#define MB_FIREWALL_ASSERT(con)                                                                     \
    do {                                                                                            \
        if (!(con)) {                                                                               \
            ESP_LOGE(MB_FIREWALL_TAG, "assert errno:%d, errno_str: !(%s)", errno, strerror(errno)); \
            assert(0 && #con);                                                                      \
        }                                                                                           \
    } while (0)

/**
 * @brief Modbus controller handler structure
 */
typedef struct {
    mb_port_type_t port_type;                    /*!< port type */
    mb_communication_info_t mbf_comm_input;      /*!< communication info IN*/
    mb_communication_info_t mbf_comm_output;     /*!< communication info OUT */
    TaskHandle_t mbf_task_handle;                /*!< task handle */
    EventGroupHandle_t mbf_event_group;          /*!< controller event group */
    QueueHandle_t mbf_notification_queue_handle; /*!< controller notification queue */
    mb_firewall_rule_function_t mbf_packet_handler;
} mb_firewall_options_t;

/**
 * @brief Request mode for parameter to use in data dictionary
 */
typedef struct
{
    mb_firewall_options_t opts; /*!< Modbus firewall options */

    // Functional pointers to internal static functions of the implementation (public interface methods)
    iface_init init;       /*!< Interface method init */
    iface_destroy destroy; /*!< Interface method destroy */
    iface_setup setup;     /*!< Interface method setup */
    iface_start start;     /*!< Interface method start */

} mb_firewall_interface_t;

#endif