#ifndef _FIREWALL_H
#define _FIREWALL_H

#include <avr/io.h>
#include <avr/pgmspace.h>

/* The firewall callback function that will be called in order to determine
 *  if packet shall pass or not.
 */
char mb_firewall_cb(unsigned char, unsigned char *, unsigned short);

#endif