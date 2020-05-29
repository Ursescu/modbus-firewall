#ifndef _FIREWALL_H
#define _FIREWALL_H

#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"

typedef enum mb_firewall_rule_result {
    FIREWALL_FAIL,
    FIREWALL_PASS
} mb_firewall_rule_result_t;

char firewall_cb(uint8_t, uint8_t *, uint16_t);

#endif