#include "esp_err.h"               // for esp_err_t
#include "mbc_master.h"            // for master interface define
#include "esp_modbus_firewall.h"   // for public interface defines
#include "mbc_serial_firewall.h"   // for create function of the port
#include "esp_modbus_callbacks.h"  // for callback functions

esp_err_t mbc_firewall_init(mb_port_type_t port_type, void** handler) {
    printf("MBC firewall init\n");

    return ESP_OK;
}

esp_err_t mbc_firewall_destroy(void) {
    printf("MBC firewall destroy\n");
    return ESP_OK;
}

esp_err_t mbc_firewall_start(void) {
    printf("MBC firewall start\n");

    return ESP_OK;
}

esp_err_t mbc_firewall_setup(void* comm_info) {
    printf("MBC firewall setup");

    return ESP_OK;
}