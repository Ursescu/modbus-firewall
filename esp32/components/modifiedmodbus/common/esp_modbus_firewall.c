#include "esp_err.h"               // for esp_err_t
#include "mbc_firewall.h"          // for master interface define
#include "esp_modbus_firewall.h"   // for public interface defines
#include "mbc_serial_firewall.h"   // for create function of the port
#include "esp_modbus_callbacks.h"  // for callback functions

/* Init private firewall interface */
static mb_firewall_interface_t* firewall_interface_ptr = NULL;

esp_err_t mbc_firewall_init(mb_port_type_t port_type, void** handler) {
    printf("MBC firewall public interface: init\n");

    void* port_handler = NULL;
    esp_err_t error = ESP_ERR_NOT_SUPPORTED;
    switch (port_type) {
        case MB_PORT_SERIAL_FIREWALL:
            // Call constructor function of actual port implementation
            error = mbc_serial_firewall_create(port_type, &port_handler);
            break;
        case MB_PORT_TCP_FIREWALL:
            // Not yet supported
            return ESP_ERR_NOT_SUPPORTED;
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }

    MB_FIREWALL_CHECK((port_handler != NULL),
                      ESP_ERR_INVALID_STATE,
                      "Firewall interface initialization failure, error=(0x%x), port type=(0x%x).",
                      error, (uint16_t)port_type);

    if ((port_handler != NULL) && (error == ESP_OK)) {
        firewall_interface_ptr = (mb_firewall_interface_t*)port_handler;
        *handler = port_handler;
    }

    return error;
}

esp_err_t mbc_firewall_destroy(void) {
    printf("MBC firewall public interface: destroy\n");

    esp_err_t error = ESP_OK;
    // Is initialization done?
    MB_FIREWALL_CHECK((firewall_interface_ptr != NULL),
                      ESP_ERR_INVALID_STATE,
                      "Firewall interface is not correctly initialized.");
    // Check if interface has been initialized
    MB_FIREWALL_CHECK((firewall_interface_ptr->destroy != NULL),
                      ESP_ERR_INVALID_STATE,
                      "Firewall interface is not correctly initialized.");
    // Call the firewall port destroy function
    error = firewall_interface_ptr->destroy();
    MB_FIREWALL_CHECK((error == ESP_OK),
                      ESP_ERR_INVALID_STATE,
                      "SERIAL firewall destroy failure error=(0x%x).", error);
    return error;
}

esp_err_t mbc_firewall_start(void) {
    printf("MBC firewall public interface: start\n");

    esp_err_t error = ESP_OK;
    MB_FIREWALL_CHECK((firewall_interface_ptr != NULL),
                      ESP_ERR_INVALID_STATE,
                      "Firewall interface is not correctly initialized.");
    MB_FIREWALL_CHECK((firewall_interface_ptr->start != NULL),
                      ESP_ERR_INVALID_STATE,
                      "Firwall interface is not correctly initialized.");

    error = firewall_interface_ptr->start();
    MB_FIREWALL_CHECK((error == ESP_OK),
                      ESP_ERR_INVALID_STATE,
                      "SERIAL firewall start failure error=(0x%x).", error);
    return error;
}

esp_err_t mbc_firewall_setup(void* comm_info) {
    printf("MBC firewall public interface: setup\n");

    esp_err_t error = ESP_OK;
    MB_FIREWALL_CHECK((firewall_interface_ptr != NULL),
                      ESP_ERR_INVALID_STATE,
                      "Firewall interface is not correctly initialized.");
    MB_FIREWALL_CHECK((firewall_interface_ptr->setup != NULL),
                      ESP_ERR_INVALID_STATE,
                      "Firewall interface is not correctly initialized.");
    error = firewall_interface_ptr->setup(comm_info);
    MB_FIREWALL_CHECK((error == ESP_OK),
                      ESP_ERR_INVALID_STATE,
                      "SERIAL firewall setup failure error=(0x%x).", error);
    return error;
}