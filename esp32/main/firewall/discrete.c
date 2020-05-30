#include "stdlib.h"
#include "string.h"

#include "firewall.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_FUNC_READ_ADDR_OFF    (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_READ_DISCCNT_OFF (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_READ_SIZE        (4)
#define MB_PDU_FUNC_READ_DISCCNT_MAX (0x07D0)

static const char *TAG = "MB_FIREWALL_DISCRETE";

mb_firewall_stat_t mb_firewall_read_discrete_inputs(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "read discrete inputs handler");

    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;

    uint16_t reg_addr;
    uint16_t discrete_count;

    if (len == (MB_PDU_FUNC_READ_SIZE + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_READ_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_READ_ADDR_OFF + 1]);
        reg_addr++;

        discrete_count = (uint16_t)(frame[MB_PDU_FUNC_READ_DISCCNT_OFF] << 8);
        discrete_count |= (uint16_t)(frame[MB_PDU_FUNC_READ_DISCCNT_OFF + 1]);

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception. 
         */
        if ((discrete_count >= 1) &&
            (discrete_count < MB_PDU_FUNC_READ_DISCCNT_MAX)) {
            return firewall_find_rule(NULL, reg_addr, discrete_count, MB_FIREWALL_REG_READ, MB_FIREWALL_DISCRETE);

        } else {
            return MB_FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid read coil register request because the length
         * is incorrect. */
        return MB_FIREWALL_FAIL;
    }

    return MB_FIREWALL_FAIL;
}
