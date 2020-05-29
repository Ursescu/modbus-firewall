
#include "firewall.h"
#include "mbproto.h"


static const char *TAG = "MODBUS_FIREWALL";

#define MB_FRAME_FUNCTION_CODE 0
#define MB_FRAME_DATA 1

#define MB_FIREWALL_MAX_RULES 16

// |  1B  |   2B  |    2B   |  REST  |
// | ADDR | FCODE |  SADDR  |  DATA  |

typedef enum mb_firewall_rule_result {
    FIREWALL_FAIL = 0,
    FIREWALL_PASS
} mb_firewall_rule_result_t;

typedef mb_firewall_rule_result_t ( *mb_firewall_rule_handler )(uint8_t *, uint16_t);

typedef struct mb_firewall_rule {
    unsigned char mb_function_code;
    mb_firewall_rule_handler handler;
} mb_firewall_rule_t;


/* Pass everything */
static mb_firewall_rule_result_t pass(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "Function code handler");
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
static char firewall_cb(unsigned char addr, unsigned char *frame, unsigned short len) {

    uint8_t function_code = frame[MB_FRAME_FUNCTION_CODE];
    mb_firewall_rule_result_t status = FIREWALL_PASS;

    uint8_t rule_index;
    ESP_LOGI(TAG, "Firewall checker - packet inspection");
    ESP_LOGI(TAG, "addr %02X, len %hu, fcode %02X", addr, len, function_code);

    /* Waterfall scheme firewall */

    /* Destination address check */
    if (addr == 4) {
        return FIREWALL_FAIL;
    }

    for (rule_index = 0; rule_index < MB_FIREWALL_MAX_RULES; rule_index++) {
        /* Handle the rule found for the function code */
        if (firewall_rules[rule_index].mb_function_code == function_code) {

            uint8_t *frame_data = &frame[MB_FRAME_DATA];

            return firewall_rules[rule_index].handler(frame_data, len - 1);
        }
    }

    ESP_LOGI(TAG, "function code unknown %02X", function_code);

    return FIREWALL_FAIL;
}