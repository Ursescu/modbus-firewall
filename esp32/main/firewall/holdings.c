#include "stdlib.h"
#include "string.h"

#include "firewall.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#define MB_PDU_FUNC_READ_ADDR_OFF   (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_READ_REGCNT_OFF (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_READ_SIZE       (4)
#define MB_PDU_FUNC_READ_REGCNT_MAX (0x007D)

#define MB_PDU_FUNC_WRITE_ADDR_OFF  (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_WRITE_VALUE_OFF (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_SIZE      (4)

#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF    (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF  (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF (MB_PDU_DATA_OFF + 4)
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF  (MB_PDU_DATA_OFF + 5)
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN    (5)
#define MB_PDU_FUNC_WRITE_MUL_REGCNT_MAX  (0x0078)

#define MB_PDU_FUNC_READWRITE_READ_ADDR_OFF    (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF  (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF   (MB_PDU_DATA_OFF + 4)
#define MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF (MB_PDU_DATA_OFF + 6)
#define MB_PDU_FUNC_READWRITE_BYTECNT_OFF      (MB_PDU_DATA_OFF + 8)
#define MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF (MB_PDU_DATA_OFF + 9)
#define MB_PDU_FUNC_READWRITE_SIZE_MIN         (9)

static const char *TAG = "MB_FIREWALL_HOLDINGS";

extern mb_firewall_mode_t firewall_type;

/* Searching through the generated rules for the holdings */
static firewall_match_t firewall_find_holding_rule(uint8_t *reg_buffer, uint16_t reg_addr, uint16_t holding_count, mb_firewall_reg_mode_t mode) {
    ESP_LOGI(TAG, "find holding rule: reg 0x%04X, count %hu, mode %s", reg_addr, holding_count, mode == FIREWALL_REG_READ ? "R" : "W");

    return FIREWALL_RULE_NOT_FOUND;
}

mb_firewall_stat_t mb_firewall_write_single_register(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "write single holding register handler");

    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;

    uint16_t reg_addr;

    if (len == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);
        reg_addr++;

        found = firewall_find_holding_rule(&frame[MB_PDU_FUNC_WRITE_VALUE_OFF],
                                           reg_addr, 1, FIREWALL_REG_WRITE);
    } else {
        /* Can't be a valid request because the length is incorrect. */
        return FIREWALL_FAIL;
    }
    switch (firewall_type) {
        case FIREWALL_BLACKLIST:
            return found == FIREWALL_RULE_FOUND ? FIREWALL_FAIL : FIREWALL_PASS;
            break;
        case FIREWALL_WHITELIST:
            return found == FIREWALL_RULE_FOUND ? FIREWALL_PASS : FIREWALL_FAIL;
            break;
        default:
            ESP_LOGE(TAG, "uknown value for the firewall type %hhu", (uint8_t)firewall_type);
            return FIREWALL_FAIL;
    }

    return FIREWALL_FAIL;
}

mb_firewall_stat_t mb_firewall_write_multiple_registers(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "write mutiple holding registers handler");

    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;

    uint16_t reg_addr;
    uint16_t reg_count;
    uint8_t reg_byte_count;

    if (len >= (MB_PDU_FUNC_WRITE_MUL_SIZE_MIN + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]);
        reg_addr++;

        reg_count = (uint16_t)(frame[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF] << 8);
        reg_count |= (uint16_t)(frame[MB_PDU_FUNC_WRITE_MUL_REGCNT_OFF + 1]);

        reg_byte_count = frame[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];

        if ((reg_count >= 1) &&
            (reg_count <= MB_PDU_FUNC_WRITE_MUL_REGCNT_MAX) &&
            (reg_byte_count == (uint8_t)(2 * reg_count))) {
            /* Make callback to update the register values. */
            found =
                firewall_find_holding_rule(&frame[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF],
                                           reg_addr, reg_count, FIREWALL_REG_WRITE);

        } else {
            return FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid request because the length is incorrect. */
        return FIREWALL_FAIL;
    }

    switch (firewall_type) {
        case FIREWALL_BLACKLIST:
            return found == FIREWALL_RULE_FOUND ? FIREWALL_FAIL : FIREWALL_PASS;
            break;
        case FIREWALL_WHITELIST:
            return found == FIREWALL_RULE_FOUND ? FIREWALL_PASS : FIREWALL_FAIL;
            break;
        default:
            ESP_LOGE(TAG, "uknown value for the firewall type %hhu", (uint8_t)firewall_type);
            return FIREWALL_FAIL;
    }

    return FIREWALL_FAIL;
}

mb_firewall_stat_t mb_firewall_read_registers(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "read holding registers handler");

    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;

    uint16_t reg_addr;
    uint16_t reg_count;

    if (len == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        reg_addr++;

        reg_count = (uint16_t)(frame[MB_PDU_FUNC_READ_REGCNT_OFF] << 8);
        reg_count = (uint16_t)(frame[MB_PDU_FUNC_READ_REGCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if ((reg_count >= 1) && (reg_count <= MB_PDU_FUNC_READ_REGCNT_MAX)) {
            found = firewall_find_holding_rule(NULL, reg_addr, reg_count, FIREWALL_REG_READ);

        } else {
            return FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid request because the length is incorrect. */
        return FIREWALL_FAIL;
    }

    switch (firewall_type) {
        case FIREWALL_BLACKLIST:
            return found == FIREWALL_RULE_FOUND ? FIREWALL_FAIL : FIREWALL_PASS;
            break;
        case FIREWALL_WHITELIST:
            return found == FIREWALL_RULE_FOUND ? FIREWALL_PASS : FIREWALL_FAIL;
            break;
        default:
            ESP_LOGE(TAG, "uknown value for the firewall type %hhu", (uint8_t)firewall_type);
            return FIREWALL_FAIL;
    }

    return FIREWALL_FAIL;
}

mb_firewall_stat_t mb_firewall_read_write_multiple_registers(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "read write mutiple holding registers handler");

    firewall_match_t found_read = FIREWALL_RULE_NOT_FOUND;
    firewall_match_t found_write = FIREWALL_RULE_NOT_FOUND;

    uint16_t reg_addr_read;
    uint16_t reg_count_read;
    uint16_t reg_addr_write;
    uint16_t reg_count_write;
    uint8_t reg_byte_count_write;

    if (len >= (MB_PDU_FUNC_READWRITE_SIZE_MIN + MB_PDU_SIZE_MIN)) {
        reg_addr_read = (uint16_t)(frame[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF] << 8U);
        reg_addr_read |= (uint16_t)(frame[MB_PDU_FUNC_READWRITE_READ_ADDR_OFF + 1]);
        reg_addr_read++;

        reg_count_read = (uint16_t)(frame[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF] << 8U);
        reg_count_read |= (uint16_t)(frame[MB_PDU_FUNC_READWRITE_READ_REGCNT_OFF + 1]);

        reg_addr_write = (uint16_t)(frame[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF] << 8U);
        reg_addr_write |= (uint16_t)(frame[MB_PDU_FUNC_READWRITE_WRITE_ADDR_OFF + 1]);
        reg_addr_write++;

        reg_count_write = (uint16_t)(frame[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF] << 8U);
        reg_count_write |= (uint16_t)(frame[MB_PDU_FUNC_READWRITE_WRITE_REGCNT_OFF + 1]);

        reg_byte_count_write = frame[MB_PDU_FUNC_READWRITE_BYTECNT_OFF];

        if ((reg_count_read >= 1) && (reg_count_read <= 0x7D) &&
            (reg_count_write >= 1) && (reg_count_write <= 0x79) &&
            ((2 * reg_count_write) == reg_byte_count_write)) {
            /* Make callback to update the register values. */

            found_read = firewall_find_holding_rule(&frame[MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF],
                                                    reg_addr_write, reg_count_write, FIREWALL_REG_WRITE);

            found_write = firewall_find_holding_rule(NULL, reg_addr_read, reg_count_read, FIREWALL_REG_READ);

        } else {
            return FIREWALL_FAIL;
        }
    }

    switch (firewall_type) {
        case FIREWALL_BLACKLIST:
            return (((found_read ^ found_write) == FIREWALL_RULE_FOUND) && (found_read == FIREWALL_RULE_FOUND)) ? FIREWALL_FAIL : FIREWALL_PASS;
            break;
        case FIREWALL_WHITELIST:
            return (((found_read ^ found_write) == FIREWALL_RULE_FOUND) && (found_read == FIREWALL_RULE_FOUND)) ? FIREWALL_PASS : FIREWALL_FAIL;
            break;
        default:
            ESP_LOGE(TAG, "uknown value for the firewall type %hhu", (uint8_t)firewall_type);
            return FIREWALL_FAIL;
    }

    return FIREWALL_FAIL;
}
