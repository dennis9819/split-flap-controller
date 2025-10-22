/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */
#include "global.h"

#pragma once
//#define F_CPU 16000000UL
//#define UART_BAUD 19200     // RS485 baud rate
#define UART_BAUD 57600                             // RS485 baud rate
#define BAUDRATE ((F_CPU) / (UART_BAUD * 16UL) - 1) // set baud rate value for UBRR

#define SFBUS_SOF_BYTE '+' // Byte marks start of frame
#define SFBUS_EOF_BYTE '$' // Byte marks end of frame

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

    void rs485_init(void);
    void rs485_send_c(char data);
    char rs485_recv_c(void);
    int rs485_recv_c_rxout(uint8_t timeout, char *data);

    int sfbus_recv_frame_v2(uint16_t address, char *payload);
    void sfbus_send_frame_v2(uint16_t address, char *payload, uint8_t length);
    uint16_t calc_CRC16(char *buffer, uint8_t len);

    void setup_async_rx();
    int parse_buffer(uint16_t address, char *payload);
    void clear_buffer();
    // auxilary var for uart timeout
    extern uint16_t timer_ticks;

#ifdef __cplusplus
}
#endif // __cplusplus
