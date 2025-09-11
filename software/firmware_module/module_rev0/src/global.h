/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/iom8.h>


// I/O Pin definition
#define BUX_RX PD0                  // RX Pin (to buffer)
#define BUX_TX PD1                  // TX Pin (to buffer)
#define BUX_DIR PD2                 // Buffer direction pin
#define SENSOR_HOME PD3             // Home sensor pin

#define MOTOR_A PC0                 // Motor phase A driver
#define MOTOR_B PC1                 // Motor phase B driver
#define MOTOR_C PC2                 // Motor phase C driver
#define MOTOR_D PC3                 // Motor phase D driver

// EEPROM Addresses
#define CONF_CONST_OKAY (uint8_t)0xAA   
#define CONF_ADDR_OKAY 0x0004
#define CONF_ADDR_ADDR 0x0000
#define CONF_ADDR_OFFSET 0x0002

// Protocol definitions
#define PROTO_MAXPKGLEN 64          // maximum size of package in bytes

// Command Bytes
#define CMDB_SETVAL (uint8_t)0x10   // Set display value
#define CMDB_SETVALR (uint8_t)0x11  // Set display value and do a full rotation 
#define CMDB_EEPROMR (uint8_t)0xF0  // Read EEPROM
#define CMDB_EEPROMW (uint8_t)0xF1  // Write EEPROM
#define CMDB_GSTS (uint8_t)0xF8     // Get status
#define CMDB_PING (uint8_t)0xFE     // Ping
#define CMDB_RESET (uint8_t)0x30    // Reset device
#define CMDB_PWRON (uint8_t)0x21    // Power motor on
#define CMDB_RPWROFF (uint8_t)0x20  // Poer motor off

// Command Responses
#define CMDR_ERR_INVALID 0xEE       // Invalid command
#define CMDR_ACK 0xAA               // Acknowledge
#define CMDR_PING 0xFF              // Ping response

// Utility definitions
#define SHIFT_0B 0
#define SHIFT_1B 8
#define SHIFT_2B 16
#define SHIFT_3B 24