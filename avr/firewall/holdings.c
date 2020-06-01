#include <avr/io.h>

#include "firewall.h"

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

mb_firewall_stat_t mb_firewall_write_single_register(uint8_t *frame, uint16_t len) {

    firewall_match_t found = FIREWALL_RULE_NOT_FOUND;

    uint16_t reg_addr;

    if (len == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN)) {
        reg_addr = (uint16_t)(frame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
        reg_addr |= (uint16_t)(frame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);
        reg_addr++;

        return firewall_find_rule(&frame[MB_PDU_FUNC_WRITE_VALUE_OFF],
                                  reg_addr, 1, MB_FIREWALL_REG_WRITE, MB_FIREWALL_HOLDING);
    } else {
        /* Can't be a valid request because the length is incorrect. */
        return MB_FIREWALL_FAIL;
    }

    return MB_FIREWALL_FAIL;
}

mb_firewall_stat_t mb_firewall_write_multiple_registers(uint8_t *frame, uint16_t len) {

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

            return firewall_find_rule(&frame[MB_PDU_FUNC_WRITE_MUL_VALUES_OFF],
                                      reg_addr, reg_count, MB_FIREWALL_REG_WRITE, MB_FIREWALL_HOLDING);

        } else {
            return MB_FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid request because the length is incorrect. */
        return MB_FIREWALL_FAIL;
    }

    return MB_FIREWALL_FAIL;
}

mb_firewall_stat_t mb_firewall_read_registers(uint8_t *frame, uint16_t len) {

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
            return firewall_find_rule(NULL, reg_addr, reg_count, MB_FIREWALL_REG_READ, MB_FIREWALL_HOLDING);

        } else {
            return MB_FIREWALL_FAIL;
        }
    } else {
        /* Can't be a valid request because the length is incorrect. */
        return MB_FIREWALL_FAIL;
    }

    return MB_FIREWALL_FAIL;
}

mb_firewall_stat_t mb_firewall_read_write_multiple_registers(uint8_t *frame, uint16_t len) {

    mb_firewall_stat_t found_read;

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

            found_read = firewall_find_rule(&frame[MB_PDU_FUNC_READWRITE_WRITE_VALUES_OFF],
                                            reg_addr_write, reg_count_write, MB_FIREWALL_REG_WRITE, MB_FIREWALL_HOLDING);
            if (found_read == MB_FIREWALL_FAIL) {
                return MB_FIREWALL_FAIL;
            }

            return firewall_find_rule(NULL, reg_addr_read, reg_count_read, MB_FIREWALL_REG_READ, MB_FIREWALL_HOLDING);

        } else {
            return MB_FIREWALL_FAIL;
        }
    }

    return MB_FIREWALL_FAIL;
}
