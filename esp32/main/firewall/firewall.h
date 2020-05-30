#ifndef _MODBUS_F_DEFINES_H
#define _MODBUS_F_DEFINES_H

#include "mbfirewall.h"

/* General defines for modbus frames */

#define MB_PDU_SIZE_MAX 253 /* Maximum size of a PDU. */
#define MB_PDU_SIZE_MIN 1   /* Function Code */
#define MB_PDU_FUNC_OFF 0   /* Offset of function code in PDU. */
#define MB_PDU_DATA_OFF 1   /* Offset for response data in PDU. */

#define MB_FUNC_HANDLERS_MAX 16

#define MB_FUNC_NONE                         0
#define MB_FUNC_READ_COILS                   1
#define MB_FUNC_READ_DISCRETE_INPUTS         2
#define MB_FUNC_WRITE_SINGLE_COIL            5
#define MB_FUNC_WRITE_MULTIPLE_COILS         15
#define MB_FUNC_READ_HOLDING_REGISTER        3
#define MB_FUNC_READ_INPUT_REGISTER          4
#define MB_FUNC_WRITE_REGISTER               6
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS     16
#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS 23
#define MB_FUNC_DIAG_READ_EXCEPTION          7
#define MB_FUNC_DIAG_DIAGNOSTIC              8
#define MB_FUNC_DIAG_GET_COM_EVENT_CNT       11
#define MB_FUNC_DIAG_GET_COM_EVENT_LOG       12
#define MB_FUNC_OTHER_REPORT_SLAVEID         17
#define MB_FUNC_ERROR                        128

#define MB_FIREWALL_NO_HANDLER -1

typedef enum mb_firewall_reg_mode {
    MB_FIREWALL_REG_READ,
    MB_FIREWALL_REG_WRITE
} mb_firewall_reg_mode_t;

/* Using this to determine in which rule table too lookup for */
typedef enum mb_firewall_data_type {
    MB_FIREWALL_COIL,
    MB_FIREWALL_DISCRETE,
    MB_FIREWALL_INPUT,
    MB_FIREWALL_HOLDING
} mb_firewall_data_type_t;

/* Defines and typedefs for the firewall rule system */
#define MB_FIREWALL_MAX_ADDRS 32

#define MB_FIREWALL_MAX_RULES 64

typedef enum mb_firewall_policy {
    MB_FIREWALL_WHITELIST = 0,
    MB_FIREWALL_BLACKLIST
} mb_firewall_policy_t;

typedef uint8_t mb_firewall_adress_t;

typedef enum mb_firewall_stat {
    MB_FIREWALL_FAIL = 0,
    MB_FIREWALL_PASS
} mb_firewall_stat_t;

typedef mb_firewall_stat_t (*mb_firewall_func_handler)(uint8_t *, uint16_t);

typedef struct mb_firewall_func {
    uint8_t function_code;
    mb_firewall_func_handler handler;
} mb_firewall_func_t;

/* Typedefs for the firewall rules */

/*  
 *
 *
 */
typedef enum mb_firewall_rule_type {
    MB_FIREWALL_RULE_FIXED,
    MB_FIREWALL_RULE_REG_INTERVAL,
    MB_FIREWALL_RULE_REG_DATA_INTERVAL
} mb_firewall_rule_type_t;

/* Could've make a single rule type, the most generic one,
 * but it is easier to distinguish them using union. There is no
 * memory penality by doing this.
 */

/* Rule that allows single register and single value */
typedef struct mb_firewall_rule_fixed {
    uint16_t reg_addr;
    uint16_t reg_data;
} mb_firewall_rule_fixed_t;

/* Rule that allows range of registers and single value */
typedef struct mb_firewall_rule_reg_interval {
    uint16_t reg_addr_start;
    uint16_t reg_addr_stop;
    uint16_t reg_data;
} mb_firewall_rule_reg_interval_t;

/* Rule that allows range of both registers and values */
typedef struct mb_firewall_rule_reg_data_interval {
    uint16_t reg_addr_start;
    uint16_t reg_addr_stop;
    uint16_t reg_data_start;
    uint16_t reg_data_stop;
} mb_firewall_rule_reg_data_interval_t;

/* Generic rule data type */
typedef struct mb_firewall_rule {
    /* Rule type */
    mb_firewall_rule_type_t type;

    /* Whitelist or blacklist */
    mb_firewall_policy_t policy;

    /* Read or write */
    mb_firewall_reg_mode_t mode;

    /* Rule union type, can be one of the tree firewall rule type */
    union {
        /* Fixed type */
        mb_firewall_rule_fixed_t rule_fixed;

        /* Reg interval */
        mb_firewall_rule_reg_interval_t rule_reg_interval;

        /* Reg data interval */
        mb_firewall_rule_reg_data_interval_t rule_reg_data_interval;
    };

} mb_firewall_rule_t;

typedef enum firewall_match {
    FIREWALL_RULE_NOT_FOUND = 0,
    FIREWALL_RULE_FOUND = 1,
} firewall_match_t;

/* Coils defines */

mb_firewall_stat_t mb_firewall_read_coils(uint8_t *frame, uint16_t len);

mb_firewall_stat_t mb_firewall_write_single_coil(uint8_t *frame, uint16_t len);

mb_firewall_stat_t mb_firewall_write_multiple_coils(uint8_t *frame, uint16_t len);

/* Discrete inputs defines */
mb_firewall_stat_t mb_firewall_read_discrete_inputs(uint8_t *frame, uint16_t len);

/* Input registers defines */
mb_firewall_stat_t mb_firewall_read_input_registers(uint8_t *frame, uint16_t len);

/* Holding registers defines */
mb_firewall_stat_t mb_firewall_read_registers(uint8_t *frame, uint16_t len);

mb_firewall_stat_t mb_firewall_write_single_register(uint8_t *frame, uint16_t len);

mb_firewall_stat_t mb_firewall_write_multiple_registers(uint8_t *frame, uint16_t len);

mb_firewall_stat_t mb_firewall_read_write_multiple_registers(uint8_t *frame, uint16_t len);

/* Find and apply rule function */
mb_firewall_stat_t firewall_find_rule(uint8_t *, uint16_t, uint16_t, mb_firewall_reg_mode_t, mb_firewall_data_type_t);

#endif