/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */


#include "rcount.h"

uint8_t rc_eeprom_write_c(uint16_t address, uint8_t data) {
  // disable interrupt
  if (EECR & (1 << EEWE)) { return 1; }
  EEAR = address;
  EEDR = data;
  EECR |= (1 << EEMWE);  // enable Master Write Enable
  EECR |= (1 << EEWE);   // write one
  return 0;
}

uint8_t rc_eeprom_read_c(uint16_t address) {
  while (EECR & (1 << EEWE))
    ;  // wait until previous write is done
  EEAR = address;
  EECR |= (1 << EERE);  // read one
  return EEDR;
}

uint32_t counter = (uint32_t)0xFFFFFFFF;
uint8_t counter_phase = 5;
void rc_tick() {
  if (counter == (uint32_t)0xFFFFFFFF) { counter = rc_getCounter(); }
  if (counter_phase < 5) {
    cli();
    if (rc_eeprom_write_c(0x100 + counter_phase, ((counter >> (counter_phase * 8)) & 0xFF)) == 0) {
      counter_phase++;
    }
    sei();
  }
}

void incrementCounter() {
  counter++;
  counter_phase = 0;
}

uint32_t rc_getCounter() {
  uint32_t counter = rc_eeprom_read_c(0x100);
  counter |= ((uint32_t)rc_eeprom_read_c(0x101) << 8);
  counter |= ((uint32_t)rc_eeprom_read_c(0x102) << 16);
  counter |= ((uint32_t)rc_eeprom_read_c(0x103) << 24);
  return counter;
}
