#include "mctrl.h"
#include "rcount.h"
#include "rs485.h"
#include <avr/io.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <util/delay.h>
/*
 * PD0 -> BUS_RX
 * PD1 -> BUS_TX
 * PD2 -> BUS_DIR (0=RECV/1=SEND)
 *
 * PD3 -> SENSOR_IN
 *
 * PC0-3 -> MOTOR CTRL
 */

uint16_t address = 0x0000;
int16_t calib_offset = 0x0000;

void eeprom_write_c(uint16_t address, uint8_t data) {
  // disable interrupt
  cli();
  while (EECR & (1 << EEWE))
    ; // wait until previous write is done
  EEAR = address;
  EEDR = data;
  EECR |= (1 << EEMWE); // enable Master Write Enable
  EECR |= (1 << EEWE);  // write one
  sei();
}

uint8_t eeprom_read_c(uint16_t address) {
  while (EECR & (1 << EEWE))
    ; // wait until previous write is done
  EEAR = address;
  EECR |= (1 << EERE); // read one
  return EEDR;
}

#define CONF_CONST_OKAY (uint8_t)0xAA
#define CONF_ADDR_OKAY 0x0004
#define CONF_ADDR_ADDR 0x0000
#define CONF_ADDR_OFFSET 0x0002

void initialSetup() {
  wdt_disable();
  if (eeprom_read_c(CONF_ADDR_OKAY) == CONF_CONST_OKAY) {
    uint8_t addrL = eeprom_read_c(CONF_ADDR_ADDR);
    uint8_t addrH = eeprom_read_c(CONF_ADDR_ADDR + 1);
    address = addrL | (addrH << 8);
    uint8_t offsetL = eeprom_read_c(CONF_ADDR_OFFSET);
    uint8_t offsetH = eeprom_read_c(CONF_ADDR_OFFSET + 1);
    calib_offset = offsetL | (offsetH << 8);
  } else {
    eeprom_write_c(CONF_ADDR_ADDR, (uint8_t)0x00);
    eeprom_write_c(CONF_ADDR_ADDR + 1, (uint8_t)0x00);
    eeprom_write_c(CONF_ADDR_OFFSET, (uint8_t)0x00);
    eeprom_write_c(CONF_ADDR_OFFSET + 1, (uint8_t)0x00);
    eeprom_write_c(CONF_ADDR_OKAY, CONF_CONST_OKAY);
  }
}

void readCommand() {
  char *payload = malloc(64);
  uint8_t payload_len = sfbus_recv_frame(address, payload);
  if (payload_len > 0) {
    // read command byte
    uint8_t opcode = *payload;
    // parse commands
    if (opcode == (uint8_t)0x10) {
      // 0x1O = Set Digit
      uint8_t targetDigit = *(payload + 1);
      mctrl_set(targetDigit, 0);
    } else if (opcode == (uint8_t)0x11) {
      // 0x11 = Set Digit (full rotation)
      uint8_t targetDigit = *(payload + 1);
      mctrl_set(targetDigit, 1);
    } else if (opcode == (uint8_t)0xF0) {
      // 0xFO = READ EEPROM
      uint8_t bytes = 5;
      char *msg = malloc(bytes + 1);
      *msg = 0xAA;
      for (uint16_t i = 1; i < (uint16_t)bytes + 1; i++) {
        *(msg + i) = eeprom_read_c(i - 1);
      }
      _delay_ms(2);
      sfbus_send_frame(0xFFFF, msg, bytes + 1);
      free(msg);

    } else if (opcode == (uint8_t)0xF1) {
      // 0xF1 = WRITE EEPROM
      eeprom_write_c(CONF_ADDR_OKAY, (char)0xFF);
      for (uint16_t i = 0; i < 4; i++) {
        eeprom_write_c(i, *(payload + 1 + i));
      }
      eeprom_write_c(CONF_ADDR_OKAY, CONF_CONST_OKAY);
      // respond with readout
      uint8_t bytes = 5;
      char *msg = malloc(bytes + 1);
      *msg = 0xAA;
      for (uint16_t i = 1; i < (uint16_t)bytes + 1; i++) {
        *(msg + i) = eeprom_read_c(i - 1);
      }
      _delay_ms(2);
      sfbus_send_frame(0xFFFF, msg, bytes + 1);
      free(msg);
      // now use new addr
      uint8_t addrL = eeprom_read_c(CONF_ADDR_ADDR);
      uint8_t addrH = eeprom_read_c(CONF_ADDR_ADDR + 1);
      address = addrL | (addrH << 8);
    } else if (opcode == (uint8_t)0xF8) {
      // 0xF8 = Get Status
      char *msg = malloc(7);
      *msg = getSts();
      uint16_t voltage = getVoltage();
      *(msg + 2) = (voltage >> 0) & 0xFF;
      *(msg + 1) = (voltage >> 8) & 0xFF;
      uint32_t counter = rc_getCounter();
      *(msg + 6) = (counter >> 0) & 0xFF;
      *(msg + 5) = (counter >> 8) & 0xFF;
      *(msg + 4) = (counter >> 16) & 0xFF;
      *(msg + 3) = (counter >> 24) & 0xFF;
      _delay_ms(2);
      sfbus_send_frame(0xFFFF, msg, 7);
      free(msg);
    } else if (opcode == (uint8_t)0xFE) {
      // 0xFE = PING
      char msg = 0xFF;
      _delay_ms(2);
      sfbus_send_frame(0xFFFF, &msg, 1);
    } else if ((opcode & (uint8_t)0xFE) == (uint8_t)0x20) {
      // 0x20 = poweroff; 0x21 is poweron
      uint8_t state = opcode & (uint8_t)0x01;
      mctrl_power(state);

    } else if (opcode == (uint8_t)0x30) {
      // 0x30 = reset
      do {
        wdt_enable(WDTO_15MS);
        for (;;) {
        }
      } while (0);
    } else {
      // invalid opcode
      char msg = 0xEE;
      _delay_ms(2);
      sfbus_send_frame(0xFFFF, &msg, 1);
    }
  }
  free(payload);
}

int main() {
  initialSetup();
  rs485_init();
  mctrl_init();

  while (1 == 1) {
    readCommand();
  }
}