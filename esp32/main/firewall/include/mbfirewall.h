#ifndef _FIREWALL_H
#define _FIREWALL_H

#include <stdio.h>
#include "esp_err.h"
#include "sdkconfig.h"

/* The firewall callback function that will be called in order to determine
 *  if packet shall pass or not.
 */
char mb_firewall_cb(uint8_t, uint8_t *, uint16_t);

#endif