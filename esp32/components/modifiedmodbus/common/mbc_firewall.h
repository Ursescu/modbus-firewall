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
#define MB_INST_MIN_SIZE                    (2) // The minimal size of Modbus registers area in bytes
#define MB_INST_MAX_SIZE                    (65535 * 2) // The maximum size of Modbus area in bytes

#define MB_CONTROLLER_NOTIFY_QUEUE_SIZE     (CONFIG_FMB_CONTROLLER_NOTIFY_QUEUE_SIZE) // Number of messages in parameter notification queue
#define MB_CONTROLLER_NOTIFY_TIMEOUT        (pdMS_TO_TICKS(CONFIG_FMB_CONTROLLER_NOTIFY_TIMEOUT)) // notification timeout

#define MB_FIREWALL_TAG "MB_CONTROLLER_FIREWALL"

#define MB_FIREWALL_CHECK(a, ret_val, str, ...) \
    if (!(a)) { \
        ESP_LOGE(MB_FIREWALL_TAG, "%s(%u): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        return (ret_val); \
    }

#define MB_FIREWALL_ASSERT(con) do { \
        if (!(con)) { ESP_LOGE(MB_FIREWALL_TAG, "assert errno:%d, errno_str: !(%s)", errno, strerror(errno)); assert(0 && #con); } \
    } while (0)

/**
 * @brief Device communication parameters for master
 */
typedef struct {
    mb_mode_type_t mode;                    /*!< Modbus communication mode */
    uint8_t slave_addr;                     /*!< Slave address field */
    uart_port_t port;                       /*!< Modbus communication port (UART) number */
    uint32_t baudrate;                      /*!< Modbus baudrate */
    uart_parity_t parity;                   /*!< Modbus UART parity settings */
} mb_firewall_comm_info_t;

/**
 * @brief Modbus controller handler structure
 */
typedef struct {
    mb_port_type_t port_type;                           /*!< port type */
    mb_communication_info_t mbs_comm;                   /*!< communication info */
    TaskHandle_t mbs_task_handle;                       /*!< task handle */
    EventGroupHandle_t mbs_event_group;                 /*!< controller event group */
    QueueHandle_t mbs_notification_queue_handle;        /*!< controller notification queue */
    // mb_register_area_descriptor_t mbs_area_descriptors[MB_PARAM_COUNT]; /*!< register area descriptors */
} mb_firewall_options_t;

// typedef mb_event_group_t (*iface_check_event)(mb_event_group_t);          /*!< Interface method check_event */
// typedef esp_err_t (*iface_get_param_info)(mb_param_info_t*, uint32_t);    /*!< Interface method get_param_info */
// typedef esp_err_t (*iface_set_descriptor)(mb_register_area_descriptor_t); /*!< Interface method set_descriptor */

/**
 * @brief Request mode for parameter to use in data dictionary
 */
typedef struct
{
    mb_firewall_options_t opts;                                    /*!< Modbus firewall options */

    // Functional pointers to internal static functions of the implementation (public interface methods)
    iface_init init;                        /*!< Interface method init */
    iface_destroy destroy;                  /*!< Interface method destroy */
    iface_setup setup;                      /*!< Interface method setup */
    iface_start start;                      /*!< Interface method start */

    // iface_check_event check_event;          /*!< Interface method check_event */
    // iface_get_param_info get_param_info;    /*!< Interface method get_param_info */
    // iface_set_descriptor set_descriptor;    /*!< Interface method set_descriptor */

    // // Modbus register calback function pointers
    // reg_discrete_cb slave_reg_cb_discrete;  /*!< Stack callback discrete rw method */
    // reg_input_cb slave_reg_cb_input;        /*!< Stack callback input rw method */
    // reg_holding_cb slave_reg_cb_holding;    /*!< Stack callback holding rw method */
    // reg_coils_cb slave_reg_cb_coils;        /*!< Stack callback coils rw method */
} mb_firewall_interface_t;


#endif