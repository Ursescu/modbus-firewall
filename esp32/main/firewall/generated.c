/* Auto generated, shouldn't be modified manually */
/* This file contains the firewall rules for the modbus firewall */
#include "firewall.h"

extern mb_firewall_policy_t firewall_default_policy;
extern mb_firewall_adress_t firewall_addresses[MB_FIREWALL_MAX_ADDRS];

mb_firewall_policy_t firewall_default_policy = FIREWALL_BLACKLIST;

mb_firewall_adress_t firewall_addresses[MB_FIREWALL_MAX_ADDRS] = {
    10, 15
};


