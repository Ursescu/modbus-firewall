
#include "firewall.h"

/* Allow to see verbose output logging */
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

static const char *TAG = "MODBUS_FIREWALL";

#define MB_PDU_SIZE_MAX 253 /* Maximum size of a PDU. */
#define MB_PDU_SIZE_MIN 1   /* Function Code */
#define MB_PDU_FUNC_OFF 0   /* Offset of function code in PDU. */
#define MB_PDU_DATA_OFF 1   /* Offset for response data in PDU. */

#define MB_FUNC_HANDLERS_MAX 16

#define MB_FUNC_NONE                         0
#define MB_FUNC_READ_COILS                   1
#define MB_FUNC_READ_DISCRETE_INPUTS         2
#define MB_FUNC_WRITE_SINGLE_COIL            5
#define MB_FUNC_WRITE_MULTIPLE_COILS         15
#define MB_FUNC_READ_HOLDING_REGISTER        3
#define MB_FUNC_READ_INPUT_REGISTER          4
#define MB_FUNC_WRITE_REGISTER               6
#define MB_FUNC_WRITE_MULTIPLE_REGISTERS     16
#define MB_FUNC_READWRITE_MULTIPLE_REGISTERS 23
#define MB_FUNC_DIAG_READ_EXCEPTION          7
#define MB_FUNC_DIAG_DIAGNOSTIC              8
#define MB_FUNC_DIAG_GET_COM_EVENT_CNT       11
#define MB_FUNC_DIAG_GET_COM_EVENT_LOG       12
#define MB_FUNC_OTHER_REPORT_SLAVEID         17
#define MB_FUNC_ERROR                        128

#define MB_FIREWALL_NO_HANDLER -1

// |  1B  |   2B  |    2B   |  REST  |
// | ADDR | FCODE |  SADDR  |  DATA  |

/* External firewall type */
extern mb_firewall_mode_t firewall_type;
extern mb_firewall_adress firewall_addresses[MB_FIREWALL_MAX_ADDRS];


/* 
 *  
 *  
 *  
 *  
 */



/* Pass everything */
static mb_firewall_stat_t mb_firewall_pass(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "pass handler");
    return FIREWALL_PASS;
}

/* Fail everything */
static mb_firewall_stat_t mb_firewall_fail(uint8_t *frame, uint16_t len) {
    ESP_LOGI(TAG, "fail hanlder");
    return FIREWALL_FAIL;
}

mb_firewall_func_t mb_firewall_function_handlers[MB_FUNC_HANDLERS_MAX] = {
    {MB_FUNC_OTHER_REPORT_SLAVEID, mb_firewall_pass},
    {MB_FUNC_READ_INPUT_REGISTER, mb_firewall_pass},
    {MB_FUNC_READ_HOLDING_REGISTER, mb_firewall_pass},
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, mb_firewall_pass},
    {MB_FUNC_WRITE_REGISTER, mb_firewall_pass},
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, mb_firewall_pass},
    {MB_FUNC_READ_COILS, mb_firewall_pass},
    {MB_FUNC_WRITE_SINGLE_COIL, mb_firewall_pass},
    {MB_FUNC_WRITE_MULTIPLE_COILS, mb_firewall_pass},
    {MB_FUNC_READ_DISCRETE_INPUTS, mb_firewall_pass},
};

static mb_firewall_stat_t mb_firewall_address_handler(uint8_t addr) {
    ESP_LOGI(TAG, "address handler");
    uint8_t addr_index;
    uint8_t found = 0;

    for (addr_index = 0; addr_index < MB_FIREWALL_MAX_ADDRS; addr_index++) {
        if (firewall_addresses[addr_index] == addr) {
            found = 1;
            break;
        }
    }

    switch (firewall_type)
    {
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
}

/* Firewall callback function implementation, based on liniar lookup tables
 * Implementation based on Waterfall design. Step by step checking.
 */
char mb_firewall_cb(unsigned char addr, unsigned char *frame, unsigned short len) {

    uint8_t function_code = frame[MB_PDU_FUNC_OFF];
    mb_firewall_stat_t status = FIREWALL_PASS;

    uint8_t index;
    int8_t handler_index = MB_FIREWALL_NO_HANDLER;

    ESP_LOGI(TAG, "packet received: addr 0x%02X, len %hu, fcode 0x%02X", addr, len, function_code);

    /* Destination address check */
    status = mb_firewall_address_handler(addr);
    if (!status)
        return FIREWALL_FAIL;

    /* Search the handler for the requested code */
    for (index = 0; index < MB_FUNC_HANDLERS_MAX; index++) {
        if (mb_firewall_function_handlers[index].mb_function_code == function_code) {
            handler_index = index;
            break;
        }
    }

    if (handler_index == MB_FIREWALL_NO_HANDLER) {
        ESP_LOGI(TAG, "function code unknown %02X", function_code);
        return FIREWALL_FAIL;
    }

    /* Call the function handler for requested code */
    status = mb_firewall_function_handlers[handler_index].handler(frame, len);
    if (!status)
        return FIREWALL_FAIL;

    return FIREWALL_PASS;
}