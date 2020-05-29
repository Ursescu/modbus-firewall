
#include "firewall.h"

/* Allow to see verbose output logging */
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char *TAG = "MODBUS_FIREWALL";

#define MB_PDU_SIZE_MAX     253 /*!< Maximum size of a PDU. */
#define MB_PDU_SIZE_MIN     1   /*!< Function Code */
#define MB_PDU_FUNC_OFF     0   /*!< Offset of function code in PDU. */
#define MB_PDU_DATA_OFF     1   /*!< Offset for response data in PDU. */

#define MB_FUNC_NONE (0)
#define MB_FUNC_READ_COILS (1)
#define MB_FUNC_READ_DISCRETE_INPUTS (2)
#define MB_FUNC_WRITE_SINGLE_COIL (5)
#define MB_FUNC_WRITE_MULTIPLE_COILS (15)
#define MB_FUNC_READ_HOLDING_REGISTER (3)
#define MB_FUNC_READ_INPUT_REGISTER (4)
#define MB_FUNC_WRITE_REGISTER (6)
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS (16)
#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS (23)
#define MB_FUNC_DIAG_READ_EXCEPTION (7)
#define MB_FUNC_DIAG_DIAGNOSTIC (8)
#define MB_FUNC_DIAG_GET_COM_EVENT_CNT (11)
#define MB_FUNC_DIAG_GET_COM_EVENT_LOG (12)
#define MB_FUNC_OTHER_REPORT_SLAVEID (17)
#define MB_FUNC_ERROR (128)

#define MB_FIREWALL_MAX_RULES 16

// |  1B  |   2B  |    2B   |  REST  |
// | ADDR | FCODE |  SADDR  |  DATA  |

typedef enum mb_firewall_rule_result {
    FIREWALL_FAIL = 0,
    FIREWALL_PASS
} mb_firewall_rule_result_t;

typedef mb_firewall_rule_result_t (*mb_firewall_rule_handler)(uint8_t *, uint16_t);

typedef struct mb_firewall_rule {
    unsigned char mb_function_code;
    mb_firewall_rule_handler handler;
} mb_firewall_rule_t;

/* Pass everything */
static mb_firewall_rule_result_t pass(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "handling the function code");
    return FIREWALL_PASS;
}


mb_firewall_rule_t firewall_rules[MB_FIREWALL_MAX_RULES] = {
    {MB_FUNC_OTHER_REPORT_SLAVEID, pass},
    {MB_FUNC_READ_INPUT_REGISTER, pass},
    {MB_FUNC_READ_HOLDING_REGISTER, pass},
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, pass},
    {MB_FUNC_WRITE_REGISTER, pass},
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, pass},
    {MB_FUNC_READ_COILS, pass},
    {MB_FUNC_WRITE_SINGLE_COIL, pass},
    {MB_FUNC_WRITE_MULTIPLE_COILS, pass},
    {MB_FUNC_READ_DISCRETE_INPUTS, pass},
};

/* Write the firewall function rule */
char firewall_cb(unsigned char addr, unsigned char *frame, unsigned short len) {
    uint8_t function_code = frame[MB_PDU_FUNC_OFF];
    mb_firewall_rule_result_t status = FIREWALL_PASS;

    uint8_t rule_index;
    ESP_LOGI(TAG, "packet received: addr 0x%02X, len %hu, fcode 0x%02X", addr, len, function_code);

    /* Destination address check */
    if (addr == 4) {
        return FIREWALL_FAIL;
    }

    for (rule_index = 0; rule_index < MB_FIREWALL_MAX_RULES; rule_index++) {
        /* Handle the rule found for the function code */
        if (firewall_rules[rule_index].mb_function_code == function_code) {

            return firewall_rules[rule_index].handler(frame, len);
        }
    }

    ESP_LOGI(TAG, "function code unknown %02X", function_code);

    return FIREWALL_FAIL;
}