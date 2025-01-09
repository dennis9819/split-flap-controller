#include "rs485.h"

void rs485_init() {
  // init I/O
  DDRD &= ~(1 << PD0);              // BUS_DIR & TX is OUTPUT
  DDRD |= (1 << PD2) | (1 << PD1);  // BUS_DIR & TX is OUTPUT
  PORTD &= 0x07;                    // clear PD0-PD4
  // init UART
  UBRRH = (BAUDRATE >> 8);
  UBRRL = BAUDRATE;                                     // set baud rate
  UCSRB |= (1 << TXEN) | (1 << RXEN);                   // enable receiver and transmitter
  UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);  // 8bit data format
}

void dbg(char data) {
  while (!(UCSRA & (1 << UDRE)))
    ;
  UDR = data;
}



void rs485_send_c(char data) {
  PORTD |= (1 << PD2);  // set transciever to transmitt
  while (!(UCSRA & (1 << UDRE)))
    ;                  // wait until buffer is empty
  UCSRA = (1 << TXC);  // clear transmit Complete bit
  UDR = data;
  while (!(UCSRA & (1 << TXC)))
    ;                    // wait until transmitt complete
  PORTD &= ~(1 << PD2);  // set transciever to transmitt
}

void rs485_send_str(char* data) {
  for (unsigned int i = 0; i < sizeof(data); i++) {
    rs485_send_c(data[i]);
  }
}

char rs485_recv_c() {
  while (!(UCSRA & (1 << RXC)))
    ;
  ;  // wait while data is being received
  return UDR;
}

// SFBUS Functions
uint8_t sfbus_recv_frame(uint16_t address, char* payload) {
  while (rs485_recv_c() != '+') {}  // Wwait for start byte

  uint8_t frm_version = rs485_recv_c();
  if (frm_version != 0) return 0;
  uint8_t frm_length = rs485_recv_c();
  uint8_t frm_addrL = rs485_recv_c();
  uint8_t frm_addrH = rs485_recv_c();

  uint16_t frm_addr = frm_addrL | (frm_addrH << 8);
  if (frm_addr != address) return 0;
  char* _payload = payload;
  for (uint8_t i = 0; i < (frm_length - 3); i++) {
    *_payload = rs485_recv_c();
    _payload++;
  }

  if (rs485_recv_c() != '$') return -1;
  return frm_length;
}

void sfbus_send_frame(uint16_t address, char* payload, uint8_t length) {
  char framelen = length;

  rs485_send_c(SFBUS_SOF_BYTE);  // send startbyte 3 times
  rs485_send_c(0);               // send protocol version
  rs485_send_c(framelen + 3);    // send lentgh of remaining frame

  rs485_send_c(address & 0xFF);  // target address
  rs485_send_c((address >> 8) & 0xFF);

  while (framelen > 0) {  // send payload
    rs485_send_c(*payload);
    payload++;
    framelen--;
  }

  rs485_send_c(SFBUS_EOF_BYTE);  // send end of frame byte
}
