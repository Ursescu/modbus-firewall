#include <sys/time.h>             // for calculation of time stamp in milliseconds
#include "esp_log.h"              // for log_write
#include "mb_f.h"                   // for mb types definition
#include "mbutils.h"              // for mbutils functions definition for stack callback
#include "sdkconfig.h"            // for KConfig values
#include "esp_modbus_common.h"    // for common defines
#include "esp_modbus_firewall.h"  // for public firewall interface types
#include "mbc_firewall.h"         // for private firewall interface types
#include "mbc_serial_firewall.h"  // for serial firewall implementation definitions
#include "port_serial_firewall.h"

/* Shared pointer to interface structure */
static mb_firewall_interface_t* mbf_interface_ptr = NULL;

// Default modbus fireall packet handler, pass everything
static char modbus_firewall_packet_handler(unsigned char addr, unsigned char *frame, unsigned short len) {

    /* Pass all packets */
    return TRUE;
}

// Modbus task function
static void modbus_firewall_task(void *pvParameters)
{
    // Modbus interface must be initialized before start 
    MB_FIREWALL_ASSERT(mbf_interface_ptr != NULL);
    mb_firewall_options_t* mbf_opts = &mbf_interface_ptr->opts;
    
    MB_FIREWALL_ASSERT(mbf_opts != NULL);


    printf("Internal task for firewall started\n");

    // Main Modbus Firewall stack processing cycle
    for (;;) {
        BaseType_t status = xEventGroupWaitBits(mbf_opts->mbf_event_group,
                                                (BaseType_t)(MB_EVENT_STACK_STARTED),
                                                pdFALSE, // do not clear bits
                                                pdFALSE,
                                                portMAX_DELAY);
        // Check if stack started then poll for data
        if (status & MB_EVENT_STACK_STARTED) {
            printf("Iteration for the main task\n");
            (void)eMBFirewallPoll();
            (void)xMBFirewallOutputPortSerialTxPoll();
            (void)xMBFirewallInputPortSerialTxPoll();
            /* Simulate some waiting here - get the main task some CPU time*/ 
            // vTaskDelay(1000 / portTICK_PERIOD_MS);
            // (void)eMBPoll(); // allow stack to process data
            // (void)xMBPortSerialTxPoll(); // Send response buffer if ready
        }
    }
}
// The helper function to get time stamp in microseconds
static uint64_t get_time_stamp()
{
    uint64_t time_stamp = esp_timer_get_time();
    return time_stamp;
}

// Setup Modbus controller parameters
static esp_err_t mbc_serial_firewall_setup(void* comm_info)
{
    MB_FIREWALL_CHECK((mbf_interface_ptr != NULL),
                    ESP_ERR_INVALID_STATE,
                    "Firewall interface is not correctly initialized.");
    MB_FIREWALL_CHECK((comm_info != NULL), ESP_ERR_INVALID_ARG,
                    "mb wrong communication settings.");
    mb_firewall_options_t* mbf_opts = &mbf_interface_ptr->opts;
    mb_firewall_comm_info_t* comm_settings = (mb_firewall_comm_info_t*)comm_info;
    MB_FIREWALL_CHECK(((comm_settings->mode_output == MB_MODE_RTU) || (comm_settings->mode_output == MB_MODE_ASCII)),
                ESP_ERR_INVALID_ARG, "mb incorrect mode input = (0x%x).",
                (uint32_t)comm_settings->mode_output);
    
    MB_FIREWALL_CHECK(((comm_settings->mode_input == MB_MODE_RTU) || (comm_settings->mode_input == MB_MODE_ASCII)),
                ESP_ERR_INVALID_ARG, "mb incorrect mode input = (0x%x).",
                (uint32_t)comm_settings->mode_input);

    MB_FIREWALL_CHECK((comm_settings->port_input < UART_NUM_MAX), ESP_ERR_INVALID_ARG,
                "mb wrong port to set = (0x%x).", (uint32_t)comm_settings->port_input);
    MB_FIREWALL_CHECK((comm_settings->port_output < UART_NUM_MAX), ESP_ERR_INVALID_ARG,
                "mb wrong port to set = (0x%x).", (uint32_t)comm_settings->port_output);
    
    MB_FIREWALL_CHECK((comm_settings->parity_input <= UART_PARITY_EVEN), ESP_ERR_INVALID_ARG,
                "mb wrong parity option = (0x%x).", (uint32_t)comm_settings->parity_input);

    MB_FIREWALL_CHECK((comm_settings->parity_output <= UART_PARITY_EVEN), ESP_ERR_INVALID_ARG,
            "mb wrong parity option = (0x%x).", (uint32_t)comm_settings->parity_output);

    MB_FIREWALL_CHECK((comm_settings->packet_handler != NULL), ESP_ERR_INVALID_ARG,
                    "mb wrong packet handler function.");

    // Set communication options of the controller
    mbf_opts->mbf_comm_input.mode = comm_settings->mode_input;
    mbf_opts->mbf_comm_input.port = comm_settings->port_input;
    mbf_opts->mbf_comm_input.baudrate = comm_settings->baudrate_input;
    mbf_opts->mbf_comm_input.parity = comm_settings->parity_input;

    mbf_opts->mbf_comm_output.mode = comm_settings->mode_output;
    mbf_opts->mbf_comm_output.port = comm_settings->port_output;
    mbf_opts->mbf_comm_output.baudrate = comm_settings->baudrate_output;
    mbf_opts->mbf_comm_output.parity = comm_settings->parity_output;

    mbf_opts->mbf_packet_handler = comm_settings->packet_handler;

    return ESP_OK;
}

// Start Modbus controller start function
static esp_err_t mbc_serial_firewall_start(void)
{
    MB_FIREWALL_CHECK((mbf_interface_ptr != NULL),
                    ESP_ERR_INVALID_STATE,
                    "Firewall interface is not correctly initialized.");
    mb_firewall_options_t* mbf_opts = &mbf_interface_ptr->opts;
    eMBErrorCode status = MB_ENOERR;
    // Initialize Modbus stack using mbcontroller parameters
    status = eMBFirewallInit((eMBMode)mbf_opts->mbf_comm_input.mode,
                         (UCHAR)mbf_opts->mbf_comm_input.port,
                         (ULONG)mbf_opts->mbf_comm_input.baudrate,
                         (eMBParity)mbf_opts->mbf_comm_input.parity,
                         (UCHAR)mbf_opts->mbf_comm_output.port,
                         (ULONG)mbf_opts->mbf_comm_output.baudrate,
                         (eMBParity)mbf_opts->mbf_comm_output.parity,
                         (xMBFirewallSerialPacketHandler)mbf_opts->mbf_packet_handler);

    MB_FIREWALL_CHECK((status == MB_ENOERR), ESP_ERR_INVALID_STATE,
            "mb stack initialization failure, eMBFirewallInit() returns (0x%x).", status);

    status = eMBFirewallEnable();

    MB_FIREWALL_CHECK((status == MB_ENOERR), ESP_ERR_INVALID_STATE,
            "mb stack set slave ID failure, eMBFirewallEnable() returned (0x%x).", (uint32_t)status);

    // Set the mbcontroller start flag
    EventBits_t flag = xEventGroupSetBits(mbf_opts->mbf_event_group,
                                            (EventBits_t)MB_EVENT_STACK_STARTED);
    MB_FIREWALL_CHECK((flag & MB_EVENT_STACK_STARTED),
                ESP_ERR_INVALID_STATE, "mb stack start event set error.");
    return ESP_OK;
}

// Modbus controller destroy function
static esp_err_t mbc_serial_firewall_destroy(void)
{
    MB_FIREWALL_CHECK((mbf_interface_ptr != NULL),
                    ESP_ERR_INVALID_STATE,
                    "Slave interface is not correctly initialized.");
    mb_firewall_options_t* mbf_opts = &mbf_interface_ptr->opts;
    eMBErrorCode mb_error = MB_ENOERR;
    // Stop polling by clearing correspondent bit in the event group
    EventBits_t flag = xEventGroupClearBits(mbf_opts->mbf_event_group,
                                    (EventBits_t)MB_EVENT_STACK_STARTED);
    MB_FIREWALL_CHECK((flag & MB_EVENT_STACK_STARTED),
                ESP_ERR_INVALID_STATE, "mb stack stop event failure.");

    // Disable and then destroy the Modbus stack
    mb_error = eMBFirewallDisable();

    MB_FIREWALL_CHECK((mb_error == MB_ENOERR), ESP_ERR_INVALID_STATE, "mb stack disable failure.");
    (void)vTaskDelete(mbf_opts->mbf_task_handle);
    (void)vQueueDelete(mbf_opts->mbf_notification_queue_handle);
    (void)vEventGroupDelete(mbf_opts->mbf_event_group);

    mb_error = eMBFirewallClose();

    MB_FIREWALL_CHECK((mb_error == MB_ENOERR), ESP_ERR_INVALID_STATE,
            "mb stack close failure returned (0x%x).", (uint32_t)mb_error);
    free(mbf_interface_ptr);

    return ESP_OK;
}

esp_err_t mbc_serial_firewall_create(mb_port_type_t port_type, void** handler) {
    MB_FIREWALL_CHECK((port_type == MB_PORT_SERIAL_FIREWALL), 
                    ESP_ERR_NOT_SUPPORTED, 
                    "mb port not supported = %u.", (uint32_t)port_type);
    // Allocate space for options
    if (mbf_interface_ptr == NULL) {
        mbf_interface_ptr = malloc(sizeof(mb_firewall_interface_t));
    }

    MB_FIREWALL_ASSERT(mbf_interface_ptr != NULL);
    vMBPortSetMode((UCHAR)port_type);
    mb_firewall_options_t* mbf_opts = &mbf_interface_ptr->opts;
    mbf_opts->port_type = MB_PORT_SERIAL_FIREWALL; // set interface port type

    // Set default values of communication options INPUT
    mbf_opts->mbf_comm_input.mode = MB_MODE_RTU;

    /* Not important to define slave_addr because firewall just forwards */
    mbf_opts->mbf_comm_input.port = MB_UART_PORT_IN;
    mbf_opts->mbf_comm_input.baudrate = MB_DEVICE_SPEED;
    mbf_opts->mbf_comm_input.parity = MB_PARITY_NONE;


    // Set default values of communication options OUTPUT
    mbf_opts->mbf_comm_output.mode = MB_MODE_RTU;

    /* Not important to define slave_addr because firewall just forwards */
    mbf_opts->mbf_comm_output.port = MB_UART_PORT_OUT;
    mbf_opts->mbf_comm_output.baudrate = MB_DEVICE_SPEED;
    mbf_opts->mbf_comm_output.parity = MB_PARITY_NONE;

    mbf_opts->mbf_packet_handler = &modbus_firewall_packet_handler;

    // Initialization of active context of the Modbus controller
    BaseType_t status = 0;
    // Parameter change notification queue
    mbf_opts->mbf_event_group = xEventGroupCreate();
    MB_FIREWALL_CHECK((mbf_opts->mbf_event_group != NULL),
            ESP_ERR_NO_MEM, "mb event group error.");
    // Parameter change notification queue
    mbf_opts->mbf_notification_queue_handle = xQueueCreate(
                                                MB_CONTROLLER_NOTIFY_QUEUE_SIZE,
                                                sizeof(mb_firewall_info_t));
    MB_FIREWALL_CHECK((mbf_opts->mbf_notification_queue_handle != NULL),
            ESP_ERR_NO_MEM, "mb notify queue creation error.");
    // Create Modbus controller task
    status = xTaskCreate((void*)&modbus_firewall_task,
                            "modbus_firewall_task",
                            MB_CONTROLLER_STACK_SIZE,
                            NULL,
                            MB_CONTROLLER_PRIORITY,
                            &mbf_opts->mbf_task_handle);
    if (status != pdPASS) {
        vTaskDelete(mbf_opts->mbf_task_handle);
        MB_FIREWALL_CHECK((status == pdPASS), ESP_ERR_NO_MEM,
                "mb controller task creation error, xTaskCreate() returns (0x%x).",
                (uint32_t)status);
    }
    MB_FIREWALL_ASSERT(mbf_opts->mbf_task_handle != NULL); // The task is created but handle is incorrect

    // Initialize interface function pointers
    // mbs_interface_ptr->check_event = mbc_serial_slave_check_event;
    mbf_interface_ptr->destroy = mbc_serial_firewall_destroy;
    // mbs_interface_ptr->get_param_info = mbc_serial_slave_get_param_info;
    mbf_interface_ptr->init = mbc_serial_firewall_create;
    // mbs_interface_ptr->set_descriptor = mbc_serial_slave_set_descriptor;
    mbf_interface_ptr->setup = mbc_serial_firewall_setup;
    mbf_interface_ptr->start = mbc_serial_firewall_start;

    // Initialize stack callback function pointers
    // mbs_interface_ptr->slave_reg_cb_discrete = eMBRegDiscreteCBSerialSlave;
    // mbs_interface_ptr->slave_reg_cb_input = eMBRegInputCBSerialSlave;
    // mbs_interface_ptr->slave_reg_cb_holding = eMBRegHoldingCBSerialSlave;
    // mbs_interface_ptr->slave_reg_cb_coils = eMBRegCoilsCBSerialSlave;

    *handler = (void*)mbf_interface_ptr;

    return ESP_OK;
}