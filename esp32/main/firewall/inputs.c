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
            return firewall_find_rule(NULL, reg_addr, reg_count, MB_FIREWALL_REG_READ, MB_FIREWALL_INPUT);

        } else {
            return MB_FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid read input register request because the length
         * is incorrect. */
        return MB_FIREWALL_FAIL;
    }

    return MB_FIREWALL_FAIL;
}