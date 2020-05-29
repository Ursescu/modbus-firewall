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

/* Defines and typedefs for the firewall rule system */
#define MB_FIREWALL_MAX_ADDRS 32

typedef enum mb_firewall_mode {
    FIREWALL_WHITELIST = 0,
    FIREWALL_BLACKLIST
} mb_firewall_mode_t;

typedef uint8_t mb_firewall_adress;

typedef enum mb_firewall_stat {
    FIREWALL_FAIL = 0,
    FIREWALL_PASS
} mb_firewall_stat_t;

typedef mb_firewall_stat_t (*mb_firewall_func_handler)(uint8_t *, uint16_t);

typedef struct mb_firewall_func {
    uint8_t mb_function_code;
    mb_firewall_func_handler handler;
} mb_firewall_func_t;

typedef struct mb_firewall_rule {
} mb_firewall_rule_t;



/* Coils defines */

mb_firewall_stat_t mb_firewall_read_coils(uint8_t *frame, uint16_t len);

mb_firewall_stat_t mb_firewall_write_single_coil(uint8_t *frame, uint16_t len);

mb_firewall_stat_t mb_firewall_write_multiple_coils(uint8_t *frame, uint16_t len);

/* Discrete inputs defines */

#endif