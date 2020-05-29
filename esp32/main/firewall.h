#ifndef _FIREWALL_H
#define _FIREWALL_H

#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"

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

/* The firewall callback function that will be called in order to determine
 *  if packet shall pass or not.
 */
char mb_firewall_cb(uint8_t, uint8_t *, uint16_t);

#endif