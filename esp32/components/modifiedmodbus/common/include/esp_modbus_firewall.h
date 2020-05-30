#ifndef _ESP_MB_FIREWALL_INTERFACE_H
#define _ESP_MB_FIREWALL_INTERFACE_H

// Public interface header for firewall
#include <stdint.h>                 // for standard int types definition
#include <stddef.h>                 // for NULL and std defines
#include "soc/soc.h"                // for BITN definitions
#include "freertos/FreeRTOS.h"      // for task creation and queues access
#include "freertos/event_groups.h"  // for event groups
#include "esp_modbus_common.h"      // for common types

/* Need two UART ports to use firewall feature */
#define MB_UART_PORT_IN  (UART_NUM_MAX - 1)  // Default UART port number
#define MB_UART_PORT_OUT (UART_NUM_MAX - 2)  // Default UART port number

/**
 * @brief Parameter access event information type
 */
typedef struct {
    uint32_t time_stamp;   /*!< Timestamp of Modbus Event (uS)*/
    uint16_t mb_offset;    /*!< Modbus register offset */
    mb_event_group_t type; /*!< Modbus event type */
    uint8_t* address;      /*!< Modbus data storage address */
    size_t size;           /*!< Modbus event register size (number of registers)*/
} mb_firewall_info_t;

typedef char (*mb_firewall_rule_function_t)(unsigned char, unsigned char*, unsigned short);

typedef struct {
    mb_mode_type_t mode_input;   /*!< Modbus communication mode */
    mb_mode_type_t mode_output;  /*!< Modbus communication mode */
    uart_port_t port_input;      /*!< Modbus communication port (UART) INPUT number */
    uart_port_t port_output;     /*!< Modbus communication port (UART) OUTPUT number */
    uint32_t baudrate_input;     /*!< Modbus baudrate INTPUT */
    uint32_t baudrate_output;    /*!< Modbus baudrate OUTPUT */
    uart_parity_t parity_input;  /*!< Modbus UART parity settings INPUT */
    uart_parity_t parity_output; /*!< Modbus UART parity settings OUTPUT */
    mb_firewall_rule_function_t packet_handler;
} mb_firewall_comm_info_t;

/**
 * @brief Initialize Modbus controller and stack
 *
 * @param[out] handler handler(pointer) to master data structure
 * @param[in] port_type type of stack
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_NO_MEM Parameter error
 */
esp_err_t mbc_firewall_init(mb_port_type_t port_type, void** handler);

/**
 * @brief Destroy Modbus controller and stack
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_STATE Parameter error
 */
esp_err_t mbc_firewall_destroy(void);

/**
 * @brief Start Modbus communication stack
 *
 * @return
 *     - ESP_OK   Success
 *     - ESP_ERR_INVALID_ARG Modbus stack start error
 */
esp_err_t mbc_firewall_start(void);

/**
 * @brief Set Modbus communication parameters for the controller
 *
 * @param comm_info Communication parameters structure.
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_ERR_INVALID_ARG Incorrect parameter data
 */
esp_err_t mbc_firewall_setup(void* firewall_info);

#endif