/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>



// I/O Pin definition
#define BUX_RX PD0
#define BUX_TX PD1
#define BUX_DIR PD2
#define SENSOR_HOME PD3

#define MOTOR_A PC0
#define MOTOR_B PC1
#define MOTOR_C PC2
#define MOTOR_D PC3

// EEPROM Addresses
#define CONF_CONST_OKAY (uint8_t)0xAA
#define CONF_ADDR_OKAY 0x0004
#define CONF_ADDR_ADDR 0x0000
#define CONF_ADDR_OFFSET 0x0002
