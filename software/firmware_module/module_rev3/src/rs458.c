/* Copyright (C) 2025 Dennis Gunia - All Rights Reserved
 * You may use, distribute and modify this code under the
 * terms of the  AGPL-3.0 license.
 *
 * https://www.dennisgunia.de
 * https://github.com/dennis9819/splitflap_v1
 */

#include "mctrl.h"
#include "rs485.h"

void rs485_init()
{
    // init I/O
    DDRD &= ~(1 << PD0);                          // RX is INPUT
    DDRD |= (1 << PD3) | (1 << PD2) | (1 << PD1); // BUS_DIR & TX is OUTPUT
    PORTD &= 0xE0;                                // clear PD0-PD4
    // init UART
    UBRRH = (BAUDRATE >> 8);
    UBRRL = BAUDRATE;                                    // set baud rate
    UCSRB |= (1 << TXEN) | (1 << RXEN);                  // enable receiver and transmitter
    UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1); // 8bit data format
}

// send byte over rs485
void rs485_send_c(char data)
{
    PORTD |= (1 << PD2) | (1 << PD3); // set transciever to transmitt
    while (!(UCSRA & (1 << UDRE)))
        ;               // wait until buffer is empty
    UCSRA = (1 << TXC); // clear transmit Complete bit
    UDR = data;
    while (!(UCSRA & (1 << TXC)))
    {
    }; // wait until transmitt complete
    PORTD &= ~((1 << PD2) | (1 << PD3)); // set transciever back to receive
}

// receive without timeout
char rs485_recv_c()
{
    while (!(UCSRA & (1 << RXC)))
        ; // wait while data is being received
    return UDR;
}

// receive with timeout
int rs485_recv_c_rxout(uint8_t timeout, char *data)
{
    timer_ticks = 0;
    while (!(UCSRA & (1 << RXC)))
    {
        if (timer_ticks > timeout)
        {
            return 0;
        }
    }
    *data = UDR;
    return 1;
}

// SFBUS Functions
// receive paket with crc checking and timeout
int sfbus_recv_frame_v2(uint16_t address, char *payload)
{
    const uint8_t RX_TIMEOUT = 4;
    while (rs485_recv_c() != SFBUS_SOF_BYTE)
    {
    } // Wait for start byte
    uint8_t frm_version, frm_length;
    if (rs485_recv_c_rxout(RX_TIMEOUT, (char *)&frm_version) == 0) // read header: version
        return -1;
    if (rs485_recv_c_rxout(RX_TIMEOUT, (char *)&frm_length) == 0) // read header: payload length
        return -1;

    // ALWAYS!!! receive full packet to avoid sync error
    for (int i = 0; i < frm_length; i++)
    {
        if (rs485_recv_c_rxout(RX_TIMEOUT, payload + i) == 0)
        {
            return -1;
        }
    }
    // check protocol version
    if (frm_version != 0x01)
    {
        return -1; // abort on incompatible version
    }
    // if version matches, read address and checksum
    uint16_t recv_addr = (*(payload + 0) & 0xFF) | ((*(payload + 1) & 0xFF) << SHIFT_1B);
    uint16_t checksum = (*(payload + (frm_length - 2)) & 0xFF) | ((*(payload + (frm_length - 1)) & 0xFF) << SHIFT_1B);

    // calculate checksum of received payload
    uint16_t checksum_calc = calc_CRC16(payload + 2, frm_length - 4);

    // return length on valid address and crc, return 0 to ignore
    return (checksum == checksum_calc && address == recv_addr) ? frm_length : 0;
}

// send paket with crc
void sfbus_send_frame_v2(uint16_t address, char *payload, uint8_t length)
{
    uint8_t framelen = length;
    // claculate crc value
    uint16_t crc = calc_CRC16(payload, framelen);

    rs485_send_c(SFBUS_SOF_BYTE);       // send startbyte
    rs485_send_c(1);                    // send protocol version
    rs485_send_c((char)(framelen + 4)); // send lentgh of remaining frame

    rs485_send_c((char)(address & 0xFF)); // target address
    rs485_send_c((char)((address >> SHIFT_1B) & 0xFF));

    while (framelen > 0)
    { // send payload
        rs485_send_c(*payload);
        payload++;
        framelen--;
    }

    rs485_send_c((char)(crc & 0xFF)); // send crc
    rs485_send_c((char)((crc >> SHIFT_1B) & 0xFF));
}

// calculate crc checksum
uint16_t calc_CRC16(char *buffer, uint8_t len)
{
    uint16_t crc16 = 0xFFFF;
    for (uint8_t pos = 0; pos < len; pos++)
    {
        char byte_value = *(buffer); // read byte after byte from buffer
        buffer++;

        crc16 ^= byte_value; // XOR byte into least sig. byte of crc
        for (int i = 8; i != 0; i--)
        { // Loop over each bit
            if ((crc16 & 0x0001) != 0)
            {                // If the LSB is set
                crc16 >>= 1; // Shift right and XOR 0xA001
                crc16 ^= 0xA001;
            }
            else
            {                // Else LSB is not set
                crc16 >>= 1; // Just shift right
            }
        }
    }
    return crc16;
}

// async receive function
uint8_t rx_buffer_ix_ptr = 0;   // receive buffer index pointer
uint8_t rx_buffer_ix_rem = 255; // receive buffer remaining
uint8_t rx_buffer_rx_done = 0;  // receive buffer complete flag (must be reset before next pkg can be received)
uint8_t *rx_buffer_temp;        // receive buffer

// setup async receive function
void setup_async_rx()
{
    rx_buffer_temp = malloc(PROTO_MAXPKGLEN); // reserve memory for rx buffer
    UCSRB |= (1 << RXCIE);                    //enable rx interrupt
}

ISR(USART_RXC_vect)
{
    if (rx_buffer_rx_done == 0) // only receive when last buffer was processed
    {
        uint8_t rx_byte = UDR; // read rx buffer
        if (timer_ticks > 6)   // too long since last byte
        {
            rx_buffer_ix_ptr = 0;   // reset pointer and start new transaction
            rx_buffer_ix_rem = 255; // reset remaining bytes to default
        }
        if (rx_buffer_ix_ptr == 0)
        {
            // wait for startbyte
            if (rx_byte == SFBUS_SOF_BYTE)
            {
                *(rx_buffer_temp) = rx_byte;
                rx_buffer_ix_ptr++;
            }
        }
        else if (rx_buffer_ix_ptr > 0 && rx_buffer_ix_ptr < (PROTO_MAXPKGLEN - 1)) // when package is already started
        {                                                   // and buffer is smaller than max length
            *(rx_buffer_temp + rx_buffer_ix_ptr) = rx_byte; // receive single byte
            rx_buffer_ix_rem--;
            rx_buffer_ix_ptr++;
            if (rx_buffer_ix_ptr == 3) // third byte is always frame length
            {
                rx_buffer_ix_rem = rx_byte;
            }
            if (rx_buffer_ix_rem == 0) // when last byte is received
            {
                rx_buffer_ix_ptr--;
                rx_buffer_rx_done = 1;
            }
        }
        timer_ticks = 0; // reset timer ticks
    }
}

int parse_buffer(uint16_t address, char *payload)
{
    if (rx_buffer_rx_done > 0)
    {
        uint8_t frm_length = *(rx_buffer_temp + 2);
        // check protocol version
        if (*(rx_buffer_temp + 1) != 0x01)
        {
            clear_buffer();
            return -1; // abort on incompatible version
        }
        if (frm_length < PROTO_MAXPKGLEN) // prevent buffer overflow
        {
            memcpy(payload, rx_buffer_temp + 3, frm_length); // copy buffer
            // if version matches, read address and checksum
            uint16_t recv_addr = (*(rx_buffer_temp + 3) & 0xFF) | ((*(rx_buffer_temp + 4) & 0xFF) << SHIFT_1B);
            uint16_t checksum = (*(rx_buffer_temp + (rx_buffer_ix_ptr - 1)) & 0xFF) |
                                ((*(rx_buffer_temp + (rx_buffer_ix_ptr)) & 0xFF) << SHIFT_1B);
            // calculate checksum of received payload
            uint16_t checksum_calc = calc_CRC16((char *)rx_buffer_temp + 5, rx_buffer_ix_ptr - 6);

            //mctrl_set(address == recv_addr ? 20 : recv_addr + 1, 0);
            clear_buffer();
            // return length on valid address and crc, return 0 to ignore
            return (checksum == checksum_calc && address == recv_addr) ? frm_length : 0;
        }
        else
        {
            // invalid package
            clear_buffer();
            return -1;
        }
    }
    return -2;
}

void clear_buffer()
{
    rx_buffer_ix_ptr = 0;               // reset pointer and start new transaction
    rx_buffer_ix_rem = PROTO_MAXPKGLEN; // reset remaining bytes to default
    rx_buffer_rx_done = 0;              // reset done flag
    //memset(rx_buffer_temp, 0x00, PROTO_MAXPKGLEN); // clear buffer
}