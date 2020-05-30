
#include "firewall.h"

/* Allow to see verbose output logging */
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char *TAG = "MB_FIREWALL";

// |  1B  |   2B  |    2B   |  REST  |
// | ADDR | FCODE |  SADDR  |  DATA  |

/* External firewall rules and default policy */
extern mb_firewall_policy_t firewall_default_policy;

extern mb_firewall_adress_t firewall_addresses[MB_FIREWALL_MAX_ADDRS];

extern mb_firewall_rule_t firewall_coil_rules[MB_FIREWALL_MAX_RULES];

extern mb_firewall_rule_t firewall_discrete_rules[MB_FIREWALL_MAX_RULES];

extern mb_firewall_rule_t firewall_input_rules[MB_FIREWALL_MAX_RULES];

extern mb_firewall_rule_t firewall_holding_rules[MB_FIREWALL_MAX_RULES];

/* Pass everything */
static inline mb_firewall_stat_t mb_firewall_pass(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "pass handler");
    return MB_FIREWALL_PASS;
}

/* Fail everything */
static inline mb_firewall_stat_t mb_firewall_fail(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "fail hanlder");
    return MB_FIREWALL_FAIL;
}

mb_firewall_func_t mb_firewall_function_handlers[MB_FUNC_HANDLERS_MAX] = {
    {MB_FUNC_OTHER_REPORT_SLAVEID, mb_firewall_pass},
    {MB_FUNC_READ_INPUT_REGISTER, mb_firewall_read_input_registers},
    {MB_FUNC_READ_HOLDING_REGISTER, mb_firewall_read_registers},
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, mb_firewall_write_multiple_registers},
    {MB_FUNC_WRITE_REGISTER, mb_firewall_write_single_register},
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, mb_firewall_read_write_multiple_registers},
    {MB_FUNC_READ_COILS, mb_firewall_read_coils},
    {MB_FUNC_WRITE_SINGLE_COIL, mb_firewall_write_single_coil},
    {MB_FUNC_WRITE_MULTIPLE_COILS, mb_firewall_write_multiple_coils},
    {MB_FUNC_READ_DISCRETE_INPUTS, mb_firewall_read_discrete_inputs},
};

static mb_firewall_stat_t mb_firewall_address_handler(uint8_t addr) {
    ESP_LOGI(TAG, "address handler");
    uint8_t addr_index;
    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;

    for (addr_index = 0; addr_index < MB_FIREWALL_MAX_ADDRS; addr_index++) {
        if (firewall_addresses[addr_index] == addr) {
            found = FIREWALL_RULE_FOUND;
            break;
        }
    }

    switch (firewall_default_policy) {
        case MB_FIREWALL_BLACKLIST:
            return found == FIREWALL_RULE_FOUND ? MB_FIREWALL_FAIL : MB_FIREWALL_PASS;
            break;
        case MB_FIREWALL_WHITELIST:
            return found == FIREWALL_RULE_FOUND ? MB_FIREWALL_PASS : MB_FIREWALL_FAIL;
            break;
        default:
            ESP_LOGE(TAG, "uknown value for the firewall type %hhu", (uint8_t)firewall_default_policy);
            return MB_FIREWALL_FAIL;
    }
}

/* Searching through the generated rules */
mb_firewall_stat_t firewall_find_rule(uint8_t *reg_buffer, uint16_t reg_addr, uint16_t coil_count, mb_firewall_reg_mode_t mode, mb_firewall_data_type_t data_type) {
    ESP_LOGI(TAG, "find rule: reg 0x%04X, count %hu, mode (%s)", reg_addr, coil_count, mode == MB_FIREWALL_REG_READ ? "R" : "W");
    uint16_t reg_idx, rule_idx;

    mb_firewall_policy_t policy_history = MB_FIREWALL_BLACKLIST;

    mb_firewall_rule_t *rule_table = NULL;

    switch (data_type) {
        case MB_FIREWALL_COIL:
            rule_table = firewall_coil_rules;
            break;
        case MB_FIREWALL_DISCRETE:
            rule_table = firewall_discrete_rules;
            break;
        case MB_FIREWALL_INPUT:
            rule_table = firewall_input_rules;
            break;
        case MB_FIREWALL_HOLDING:
            rule_table = firewall_holding_rules;
            break;
        default:
            return MB_FIREWALL_FAIL;
            break;
    }

    for (reg_idx = 0; reg_idx < coil_count; reg_idx++) {
        mb_firewall_policy_t rule_policy;
        firewall_match_t match = FIREWALL_RULE_NOT_FOUND;
        uint16_t data = 0;

        if (mode == MB_FIREWALL_REG_WRITE) {
            switch (data_type) {
                case MB_FIREWALL_COIL:
                    data = (reg_buffer[reg_idx / 8] & (1 << (reg_idx % 8)));

                    /* Convert it to boolean */
                    data = !!data;
                    break;
                case MB_FIREWALL_HOLDING:
                    data = reg_buffer[reg_idx * 2] << 8 | reg_buffer[reg_idx * 2 + 1];
                    break;
                default:
                    /* Nothing here */
                    break;
            }
        }

        for (rule_idx = 0; rule_idx < MB_FIREWALL_MAX_RULES; rule_idx++) {
            mb_firewall_rule_t temp_rule = rule_table[rule_idx];

            if (temp_rule.mode != mode)
                continue;

            switch (temp_rule.type) {
                case MB_FIREWALL_RULE_FIXED:
                    /* code */
                    /* Check the reg address */
                    if (temp_rule.rule_fixed.reg_addr == reg_addr) {
                        /* If write rule, check the data */
                        if (mode == MB_FIREWALL_REG_WRITE) {
                            if (temp_rule.rule_fixed.reg_data != data) {
                                /* Couldn't match data*/
                                break;
                            }
                        }
                        match = FIREWALL_RULE_FOUND;
                    }
                    break;
                case MB_FIREWALL_RULE_REG_INTERVAL:
                    if (temp_rule.rule_reg_interval.reg_addr_start <= reg_addr &&
                        temp_rule.rule_reg_interval.reg_addr_stop >= reg_addr) {
                        /* If write rule, check the data */
                        if (mode == MB_FIREWALL_REG_WRITE) {
                            if (temp_rule.rule_reg_interval.reg_data != data) {
                                /* Couldn't match data*/
                                break;
                            }
                        }
                        match = FIREWALL_RULE_FOUND;
                    }
                    break;
                case MB_FIREWALL_RULE_REG_DATA_INTERVAL:
                    if (temp_rule.rule_reg_data_interval.reg_addr_start <= reg_addr &&
                        temp_rule.rule_reg_data_interval.reg_addr_stop >= reg_addr) {
                        /* If write rule, check the data */
                        if (mode == MB_FIREWALL_REG_WRITE) {
                            if (temp_rule.rule_reg_data_interval.reg_data_start > data ||
                                temp_rule.rule_reg_data_interval.reg_data_stop < data) {
                                /* Couldn't match data*/
                                break;
                            }
                        }
                        match = FIREWALL_RULE_FOUND;
                    }
                    break;
                default:
                    /* Unknown rule type */
                    break;
            }

            /* Break the rule searching, allready found one rule that matched it */
            if (match) {
                rule_policy = temp_rule.policy;
                ESP_LOGI(TAG, "found rule (%s): reg_addr 0x%04X, table offset %hu", rule_policy == MB_FIREWALL_WHITELIST ? "WHITE" : "BLACK", reg_addr, rule_idx);
                break;
            }
        }

        /* If this is the first rule, we must init the policy history */
        policy_history = reg_idx == 0 ? (match == 1 ? rule_policy : firewall_default_policy) : policy_history;

        if (!match) {
            rule_policy = firewall_default_policy;
            ESP_LOGI(TAG, "not found rule: using default policy (%s), reg_addr 0x%04X", rule_policy == MB_FIREWALL_WHITELIST ? "WHITE" : "BLACK", reg_addr);
        }

        if (rule_policy != policy_history) {
            /* Have multiple rules with different policies, need to drop the packet */
            return MB_FIREWALL_FAIL;
        }

        /* Advanced the reg_addr */
        reg_addr += 1;
    }

    /* If the overall policy is whitelist, then pass the packet, otherwise drop */
    switch (policy_history) {
        case MB_FIREWALL_WHITELIST:
            ESP_LOGI(TAG, "packet allowed");
            return MB_FIREWALL_PASS;
        default:
            return MB_FIREWALL_FAIL;
    }
}

/* Firewall callback function implementation, based on liniar lookup tables
 * Implementation based on Waterfall design. Step by step checking.
 */
char mb_firewall_cb(unsigned char addr, unsigned char *frame, unsigned short len) {
    uint8_t function_code = frame[MB_PDU_FUNC_OFF];
    mb_firewall_stat_t status = MB_FIREWALL_PASS;

    uint8_t index;
    int8_t handler_index = MB_FIREWALL_NO_HANDLER;

    ESP_LOGI(TAG, "packet received: addr 0x%02X, len %hu, fcode 0x%02X, firewall mode %s",
             addr, len, function_code, firewall_default_policy == MB_FIREWALL_WHITELIST ? "W" : "B");

    /* Destination address check */
    status = mb_firewall_address_handler(addr);
    if (!status) {
        ESP_LOGI(TAG, "address not allowed 0x%02X", addr);
        return MB_FIREWALL_FAIL;
    }

    /* Search the handler for the requested code */
    for (index = 0; index < MB_FUNC_HANDLERS_MAX; index++) {
        if (mb_firewall_function_handlers[index].function_code == function_code) {
            handler_index = index;
            break;
        }
    }

    if (handler_index == MB_FIREWALL_NO_HANDLER) {
        ESP_LOGI(TAG, "function code unknown 0x%02X", function_code);
        return MB_FIREWALL_FAIL;
    }

    /* Call the function handler for requested code */
    status = mb_firewall_function_handlers[handler_index].handler(frame, len);
    if (!status)
        return MB_FIREWALL_FAIL;

    return MB_FIREWALL_PASS;
}