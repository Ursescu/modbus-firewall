#include "stdlib.h"
#include "string.h"

#include "firewall.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_FUNC_READ_ADDR_OFF   (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_READ_REGCNT_OFF (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_READ_SIZE       (4)
#define MB_PDU_FUNC_READ_REGCNT_MAX (0x007D)

#define MB_PDU_FUNC_READ_RSP_BYTECNT_OFF (MB_PDU_DATA_OFF)

static const char *TAG = "MB_FIREWALL_INPUTS";

extern mb_firewall_mode_t firewall_type;

/* Searching through the generated rules for the inputs */
static firewall_match_t firewall_find_input_rule(uint8_t *reg_buffer, uint16_t reg_addr, uint16_t input_count) {
    ESP_LOGI(TAG, "find input rule: reg 0x%04X, count %hu", reg_addr, input_count);

    return FIREWALL_RULE_NOT_FOUND;
}

mb_firewall_stat_t mb_firewall_read_input_registers(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "read input register handler");

    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;
    uint16_t reg_addr;
    uint16_t reg_count;

    if (len == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        reg_addr++;

        reg_count = (uint16_t)(frame[MB_PDU_FUNC_READ_REGCNT_OFF] << 8);
        reg_count |= (uint16_t)(frame[MB_PDU_FUNC_READ_REGCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if ((reg_count >= 1) && (reg_count < MB_PDU_FUNC_READ_REGCNT_MAX)) {
            /* Set the current PDU data pointer to the beginning. */

            found =
                firewall_find_input_rule(NULL, reg_addr, reg_count);
        } else {
            return FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid read input register request because the length
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

    return FIREWALL_FAIL;
}