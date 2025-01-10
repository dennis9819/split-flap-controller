#pragma once
//#define F_CPU 16000000UL
#define UART_BAUD 19200
#define BAUDRATE ((F_CPU) / (UART_BAUD * 16UL) - 1)  // set baud rate value for UBRR

#define SFBUS_SOF_BYTE '+'
#define SFBUS_EOF_BYTE '$'

#include <avr/io.h>
#include <avr/interrupt.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
void dbg(char data);

void rs485_init(void);
void rs485_send_c(char data);
void rs485_send_str(char* data);
char rs485_recv_c(void);

uint8_t sfbus_recv_frame(uint16_t address, char* payload);
void sfbus_send_frame(uint16_t address, char* payload, uint8_t length);

#ifdef __cplusplus
}
#endif // __cplusplus