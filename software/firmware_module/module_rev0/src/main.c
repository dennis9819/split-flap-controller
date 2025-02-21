/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */

#include "global.h"
#include "mctrl.h"
#include "rcount.h"
#include "rs485.h"

uint16_t address = 0x0000;
uint16_t calib_offset = 0x0000;

void eeprom_write_c(uint16_t address, uint8_t data)
{
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

uint8_t eeprom_read_c(uint16_t address)
{
    while (EECR & (1 << EEWE))
        ; // wait until previous write is done
    EEAR = address;
    EECR |= (1 << EERE); // read one
    return EEDR;
}

void initialSetup()
{
    wdt_disable();
    if (eeprom_read_c(CONF_ADDR_OKAY) == CONF_CONST_OKAY)
    {
        uint8_t addrL = eeprom_read_c(CONF_ADDR_ADDR);
        uint8_t addrH = eeprom_read_c(CONF_ADDR_ADDR + 1);
        address = addrL | (addrH << 8);
        uint8_t offsetL = eeprom_read_c(CONF_ADDR_OFFSET);
        uint8_t offsetH = eeprom_read_c(CONF_ADDR_OFFSET + 1);
        calib_offset = (offsetL | (offsetH << 8));
    }
    else
    {
        eeprom_write_c(CONF_ADDR_ADDR, (uint8_t)0x00);
        eeprom_write_c(CONF_ADDR_ADDR + 1, (uint8_t)0x00);
        eeprom_write_c(CONF_ADDR_OFFSET, (uint8_t)0x00);
        eeprom_write_c(CONF_ADDR_OFFSET + 1, (uint8_t)0x00);
        eeprom_write_c(CONF_ADDR_OKAY, CONF_CONST_OKAY);
    }
}

void readCommand()
{
    char *payload = malloc(PROTO_MAXPKGLEN);
    uint8_t payload_len = sfbus_recv_frame(address, payload);
    if (payload_len > 0)
    {
        // read command byte
        uint8_t opcode = *payload;
        // parse commands
        if (opcode == CMDB_SETVAL)
        {
            // 0x1O = Set Digit
            uint8_t targetDigit = *(payload + 1);
            mctrl_set(targetDigit, 0);
        }
        else if (opcode == CMDB_SETVALR)
        {
            // 0x11 = Set Digit (full rotation)
            uint8_t targetDigit = *(payload + 1);
            mctrl_set(targetDigit, 1);
        }
        else if (opcode == CMDB_EEPROMR)
        {
            // 0xFO = READ EEPROM
            uint8_t bytes = 5;
            char *msg = malloc(bytes + 1);
            *msg = CMDR_ACK;
            for (uint16_t i = 1; i < (uint16_t)bytes + 1; i++)
            {
                *(msg + i) = (char)eeprom_read_c(i - 1);
            }
            _delay_ms(2);
            sfbus_send_frame(0xFFFF, msg, bytes + 1);
            free(msg);
        }
        else if (opcode == CMDB_EEPROMW)
        {
            // 0xF1 = WRITE EEPROM
            eeprom_write_c(CONF_ADDR_OKAY, (char)0xFF);
            for (uint16_t i = 0; i < 4; i++)
            {
                eeprom_write_c(i, *(payload + 1 + i));
            }
            eeprom_write_c(CONF_ADDR_OKAY, CONF_CONST_OKAY);
            // respond with readout
            uint8_t bytes = 5;
            char *msg = malloc(bytes + 1);
            *msg = CMDR_ACK;
            for (uint16_t i = 1; i < (uint16_t)bytes + 1; i++)
            {
                *(msg + i) = (char)eeprom_read_c(i - 1);
            }
            _delay_ms(2);
            sfbus_send_frame(0xFFFF, msg, bytes + 1);
            free(msg);
            // now use new addr
            uint8_t addrL = eeprom_read_c(CONF_ADDR_ADDR);
            uint8_t addrH = eeprom_read_c(CONF_ADDR_ADDR + 1);
            address = addrL | (addrH << SHIFT_1B);
        }
        else if (opcode == CMDB_GSTS)
        {
            char *msg = malloc(7);
            *msg = (char)getSts();
            uint16_t voltage = getVoltage();
            *(msg + 2) = (char)((voltage >> SHIFT_0B) & 0xFF);
            *(msg + 1) = (char)((voltage >> SHIFT_1B) & 0xFF);
            uint32_t counter = rc_getCounter();
            *(msg + 6) = (char)((counter >> SHIFT_0B) & 0xFF);
            *(msg + 5) = (char)((counter >> SHIFT_1B) & 0xFF);
            *(msg + 4) = (char)((counter >> SHIFT_2B) & 0xFF);
            *(msg + 3) = (char)((counter >> SHIFT_3B) & 0xFF);
            _delay_ms(2);
            sfbus_send_frame(0xFFFF, msg, 7);
            free(msg);
        }
        else if (opcode == CMDB_PING)
        {
            char msg = (char)CMDR_PING;
            _delay_ms(2);
            sfbus_send_frame(0xFFFF, &msg, 1);
        }
        else if (opcode == CMDB_RPWROFF)
        {
            mctrl_power(0);
        }
        else if (opcode == CMDB_PWRON)
        {
            mctrl_power(1);
        }
        else if (opcode == CMDB_RESET)
        {
            do
            {
                wdt_enable(WDTO_15MS);
                for (;;)
                {
                }
            } while (0);
        }
        else
        {
            // invalid opcode
            char msg = CMDR_ERR_INVALID;
            _delay_ms(2);
            sfbus_send_frame(0xFFFF, &msg, 1);
        }
    }
    free(payload);
}

int main()
{
    initialSetup();
    rs485_init();
    mctrl_init();

    while (1 == 1)
    {
        readCommand();
    }
}
