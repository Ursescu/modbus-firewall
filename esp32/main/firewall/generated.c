/* Auto generated, shouldn't be modified manually */
/* This file contains the firewall rules for the modbus firewall */
#include "firewall.h"

extern mb_firewall_policy_t firewall_default_policy;
extern mb_firewall_adress_t firewall_addresses[MB_FIREWALL_MAX_ADDRS];

extern mb_firewall_rule_t firewall_coil_rules[MB_FIREWALL_MAX_RULES];

extern mb_firewall_rule_t firewall_discrete_rules[MB_FIREWALL_MAX_RULES];

extern mb_firewall_rule_t firewall_input_rules[MB_FIREWALL_MAX_RULES];

extern mb_firewall_rule_t firewall_holding_rules[MB_FIREWALL_MAX_RULES];

mb_firewall_policy_t firewall_default_policy = MB_FIREWALL_BLACKLIST;

mb_firewall_adress_t firewall_addresses[MB_FIREWALL_MAX_ADDRS] = {
    10, 15,
};

/* Rules for each modbus request type */

/* Coils */
mb_firewall_rule_t firewall_coil_rules[MB_FIREWALL_MAX_RULES] = {
    {
        .type = MB_FIREWALL_RULE_FIXED,
        .policy = MB_FIREWALL_WHITELIST,
        .mode = MB_FIREWALL_REG_READ,
        .rule_fixed = {
            .reg_addr = 0x0002,
            .reg_data = 0x0000,
        },
    },
    {
        .type = MB_FIREWALL_RULE_FIXED,
        .policy = MB_FIREWALL_BLACKLIST,
        .mode = MB_FIREWALL_REG_READ,
        .rule_fixed = {
            .reg_addr = 0x0001,
            .reg_data = 0x0000,
        },
    },
    {
        .type = MB_FIREWALL_RULE_FIXED,
        .policy = MB_FIREWALL_WHITELIST,
        .mode = MB_FIREWALL_REG_WRITE,
        .rule_fixed = {
            .reg_addr = 0x0005,
            .reg_data = 0x0001,
        },
    },
};

/* Discrete */
mb_firewall_rule_t firewall_discrete_rules[MB_FIREWALL_MAX_RULES] = {

};

/* Inputs */
mb_firewall_rule_t firewall_input_rules[MB_FIREWALL_MAX_RULES] = {
    {
        .type = MB_FIREWALL_RULE_FIXED,
        .policy = MB_FIREWALL_WHITELIST,
        .mode = MB_FIREWALL_REG_READ,
        .rule_fixed = {
            .reg_addr = 0x0002,
            .reg_data = 0x0001,
        },
    },
};

/* Holdings */
mb_firewall_rule_t firewall_holding_rules[MB_FIREWALL_MAX_RULES] = {
    {
        .type = MB_FIREWALL_RULE_REG_DATA_INTERVAL,
        .policy = MB_FIREWALL_WHITELIST,
        .mode = MB_FIREWALL_REG_WRITE,
        .rule_reg_data_interval = {
            .reg_addr_start = 0x0001,
            .reg_addr_stop = 0x0010,
            .reg_data_start = 0xAAAA,
            .reg_data_stop = 0xAAFA
        },
    },
    {
        .type = MB_FIREWALL_RULE_REG_DATA_INTERVAL,
        .policy = MB_FIREWALL_BLACKLIST,
        .mode = MB_FIREWALL_REG_WRITE,
        .rule_reg_data_interval = {
            .reg_addr_start = 0x0001,
            .reg_addr_stop = 0x0010,
            .reg_data_start = 0xBBBB,
            .reg_data_stop = 0xFFFF
        },
    },
    {
        .type = MB_FIREWALL_RULE_REG_DATA_INTERVAL,
        .policy = MB_FIREWALL_WHITELIST,
        .mode = MB_FIREWALL_REG_READ,
        .rule_reg_data_interval = {
            .reg_addr_start = 0x0001,
            .reg_addr_stop = 0x0010,
            .reg_data_start = 0x0000,
            .reg_data_stop = 0x0000
        },
    },
};
