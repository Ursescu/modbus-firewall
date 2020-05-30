#include "stdlib.h"
#include "string.h"

#include "firewall.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#define MB_PDU_FUNC_READ_ADDR_OFF    (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_READ_COILCNT_OFF (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_READ_SIZE        (4)
#define MB_PDU_FUNC_READ_COILCNT_MAX (0x07D0)

#define MB_PDU_FUNC_WRITE_ADDR_OFF  (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_WRITE_VALUE_OFF (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_SIZE      (4)

#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF    (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF (MB_PDU_DATA_OFF + 4)
#define MB_PDU_FUNC_WRITE_MUL_VALUES_OFF  (MB_PDU_DATA_OFF + 5)
#define MB_PDU_FUNC_WRITE_MUL_SIZE_MIN    (5)
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_MAX (0x07B0)

static const char *TAG = "MB_FIREWALL_COILS";

extern mb_firewall_mode_t firewall_type;

/* Searching through the generated rules for the coils */
static firewall_match_t firewall_find_coil_rule(uint8_t *reg_buffer, uint16_t reg_addr, uint16_t coil_count, mb_firewall_reg_mode_t mode) {
    ESP_LOGI(TAG, "find coil rule: reg 0x%04X, count %hu, mode %s", reg_addr, coil_count, mode == FIREWALL_REG_READ ? "R" : "W");

    return FIREWALL_RULE_NOT_FOUND;
}

mb_firewall_stat_t mb_firewall_read_coils(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "read coils handler");

    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;

    uint16_t reg_addr;
    uint16_t coil_count;

    if (len == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        reg_addr++;

        coil_count = (uint16_t)(frame[MB_PDU_FUNC_READ_COILCNT_OFF] << 8);
        coil_count |= (uint16_t)(frame[MB_PDU_FUNC_READ_COILCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if ((coil_count >= 1) &&
            (coil_count < MB_PDU_FUNC_READ_COILCNT_MAX)) {
            /* Set the current PDU data pointer to the beginning. */

            found =
                firewall_find_coil_rule(NULL, reg_addr, coil_count,
                                        FIREWALL_REG_READ);

        } else {
            return FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid read coil register request because the length
         * is incorrect. */
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
}

mb_firewall_stat_t mb_firewall_write_single_coil(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "wirte single coil handler");
    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;

    uint16_t reg_addr;
    uint8_t buf[2];

    if (len == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);
        reg_addr++;

        if ((frame[MB_PDU_FUNC_WRITE_VALUE_OFF + 1] == 0x00) &&
            ((frame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF) ||
             (frame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0x00))) {
            buf[1] = 0;
            if (frame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF) {
                buf[0] = 1;
            } else {
                buf[0] = 0;
            }
            found =
                firewall_find_coil_rule(&buf[0], reg_addr, 1, FIREWALL_REG_WRITE);
        } else {
            return FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid write coil register request because the length
         * is incorrect. */
        return FIREWALL_FAIL;
    }

    switch (firewall_type) {
        case FIREWALL_BLACKLIST:
            return found ? FIREWALL_FAIL : FIREWALL_PASS;
            break;
        case FIREWALL_WHITELIST:
            return found ? FIREWALL_PASS : FIREWALL_FAIL;
            break;
        default:
            ESP_LOGE(TAG, "uknown value for the firewall type %hhu", (uint8_t)firewall_type);
            return FIREWALL_FAIL;
    }

    return FIREWALL_FAIL;
}

mb_firewall_stat_t mb_firewall_write_multiple_coils(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "wirte mutiple coils handler");
    uint8_t found = 0;

    uint16_t reg_addr;
    uint16_t coil_count;
    uint8_t byte_count;
    uint8_t byte_count_verify;

    if (len > (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]);
        reg_addr++;

        coil_count = (uint16_t)(frame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8);
        coil_count |= (uint16_t)(frame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1]);

        byte_count = frame[MB_PDU_FUNC_WRITE_MUL_BYTECNT_OFF];

        /* Compute the number of expected bytes in the request. */
        if ((coil_count & 0x0007) != 0) {
            byte_count_verify = (uint8_t)(coil_count / 8 + 1);
        } else {
            byte_count_verify = (uint8_t)(coil_count / 8);
        }

        if ((coil_count >= 1) &&
            (coil_count <= MB_PDU_FUNC_WRITE_MUL_COILCNT_MAX) &&
            (byte_count_verify == byte_count)) {
            found =
                firewall_find_coil_rule(&frame[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF],
                                        reg_addr, coil_count, FIREWALL_REG_WRITE);

        } else {
            return FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid write coil register request because the length
         * is incorrect. */
        return FIREWALL_FAIL;
    }
    switch (firewall_type) {
        case FIREWALL_BLACKLIST:
            return found ? FIREWALL_FAIL : FIREWALL_PASS;
            break;
        case FIREWALL_WHITELIST:
            return found ? FIREWALL_PASS : FIREWALL_FAIL;
            break;
        default:
            ESP_LOGE(TAG, "uknown value for the firewall type %hhu", (uint8_t)firewall_type);
            return FIREWALL_FAIL;
    }

    return FIREWALL_FAIL;
}