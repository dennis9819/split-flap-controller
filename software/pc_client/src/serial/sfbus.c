/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

#include "sfbus.h"
#include "../logging/logger.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/*
* Print buffer as hex values to stdout
* Used for debugging purposes
*
* @param buffer: buffer to print
* @param length: length of buffer
*/
void print_charHex(const char *buffer, int length)
{
    int _tlength = length;
    while (_tlength > 0)
    {
        printf("%02X ", (*buffer & 0xFF));
        buffer++;
        _tlength--;
    }
}

/*
* Print received buffer as hex values to stdout
* Used for debugging purposes
*
* @param buffer: buffer to print
* @param length: length of buffer
* @param address: device address
*/
void print_bufferHexRx(char *buffer, int length, u_int16_t address)
{
    if (log_message_header(LOG_TRACE) > 0)
    {
        printf("Rx at address: 0x%04X Data: ", address);
        print_charHex(buffer, length);
        printf("| %i bytes received\n", length);
    }
}

/*
* Print transmitted buffer as hex values to stdout
* Used for debugging purposes
*
* @param buffer: buffer to print
* @param length: length of buffer
* @param address: device address
*/
void print_bufferHexTx(char *buffer, int length, u_int16_t address)
{
    if (log_message_header(LOG_TRACE) > 0)
    {
        printf("Tx at address: 0x%04X Data: ", address);
        print_charHex(buffer, length);
        printf("| %i bytes sent\n", length);
    }
}

/*
* Receive SFBus frame with protocol version 2.0 and wait for valid data
* Fails after 2 unsuccessful attempts
*
* @param fd: rs485 file descriptor
* @param address: device address
* @param buffer: buffer to store received data
* @return length of received data on success, -1 on error
*/
ssize_t sfbus_recv_frame_wait(int fd, u_int16_t address, char *buffer)
{
    ssize_t len = 0;                                           // length of received data
    for (int rcount = 0; rcount < SFBUS_RETRY_COUNT; rcount++) // retry loop
    {
        log_message(LOG_DEBUG, "Waiting for data from 0x%04X (attempt %i/2)", address, rcount + 1);
        len = sfbus_recv_frame_v2(fd, address, buffer); // receive frame
        if (len > 0)                                    // valid data received
        {
            break;
        }
    }
    if (len <= 0)
    {
        log_message(LOG_WARNING, "No valid data received from 0x%04X after %i attempts!", address, SFBUS_RETRY_COUNT);
        return -1;
    }
    else
    {
        if (len > 4) // valid data length. skip first 4 bytes (header)
        {
            print_bufferHexRx(buffer, len - 4, address);
            return len - 4; // also return length without header
        }
        else
        {
            log_message(LOG_WARNING, "Invalid data length received from 0x%04X!", address);
            return -1;
        }
    }
}

/*
* Receive SFBus frame with protocol version 2.0
* Handles start byte, address check and CRC verification
*
* @param fd: rs485 file descriptor
* @param address: device address
* @param buffer: buffer to store received data
* @return length of received data on success, -1 on error
*/
ssize_t sfbus_recv_frame_v2(int fd, u_int16_t address, char *buffer)
{
    memset(buffer, 0, sizeof(char) * SFBUS_MAX_BUFFER_SIZE); // clear receive buffer
    // wait for start byte
    char byte = 0x00;
    int retryCount = SFBUS_RETRY_COUNT;
    while (byte != '+')
    {
        byte = 0x00;
        read(fd, &byte, 1);
        retryCount--;
        if (retryCount == 0)
        {
            return -1;
        }
    }
    u_int8_t frm_version;
    u_int8_t frm_addr_l, frm_addr_h;
    u_int8_t frm_length;
    u_int8_t frm_crc_l, frm_crc_h;

    read(fd, &frm_version, 1);
    read(fd, &frm_length, 1);
    read(fd, &frm_addr_h, 1);
    read(fd, &frm_addr_l, 1);

    u_int16_t dst_addr = frm_addr_h | (frm_addr_l << 8);
    if (dst_addr != address) // abort on address mismatch
    {
        return 0;
    }

    if (frm_length == 0) // abort on zero length. Should not happen!
    {
        log_message(LOG_WARNING, "Zero length frame received at 0x%04X!", address);
        return 0;
    }

    if (frm_length < 4 || frm_length > SFBUS_MAX_BUFFER_SIZE) // abort on invalid length. Should not happen!
    {
        log_message(LOG_WARNING, "Invalid frame length %i received at 0x%04X!", frm_length, address);
        return -1;
    }

    // read all bytes:
    read(fd, buffer, frm_length - 4); // read n bytes from buffer where n = payload length
    // read crc
    u_int16_t frm_crc = 0xFFFF; // read crc value
    read(fd, &frm_crc, 2);

    u_int16_t calc_crc = calc_CRC16(buffer, frm_length - 4); // calculate CRC


    if (frm_crc == calc_crc)
    {
        log_message(LOG_DEBUG, "crc check okay! expected: %04X received: %04X", calc_crc, frm_crc);
        return frm_length;
    }
    else
    {
        if (log_message_header(LOG_TRACE) > 0)
        {
            print_bufferHexRx(buffer, frm_length, address);
        }
        log_message(LOG_DEBUG, "crc check failed! expected: %04X received: %04X", calc_crc, frm_crc);

        return -1;
    }
}

/*
* Send SFBus frame with protocol version 2.0
* Generates frame with start byte, address and crc checksum
*
* @param fd: rs485 file descriptor
* @param address: device address
* @param length: length of payload
* @param buffer: payload buffer to send
*/
void sfbus_send_frame_v2(int fd, u_int16_t address, u_int8_t length, char *buffer)
{
    int frame_size_complete = length + 7;      // calculate size of complete transmission
                                               // (header + frame + crc)
    char *frame = malloc(frame_size_complete); // allocate frame buffer

    // assemble tx data
    *(frame + 0) = 0x2B;               // startbyte
    *(frame + 1) = 0x01;               // protocol version (v2.0)
    *(frame + 2) = length + 4;         // length of frame data + address + crc
    *(frame + 3) = (address);          // address high byte
    *(frame + 4) = ((address >> 8));   // address low byte
    memcpy(frame + 5, buffer, length); // copy payload to packet

    // add crc to end of frame
    u_int16_t crc = calc_CRC16(buffer, length);          // calculate CRC
    *(frame + (frame_size_complete - 2)) = (crc);        // address high byte
    *(frame + (frame_size_complete - 1)) = ((crc >> 8)); // address low byte
    // send data
    int result = write(fd, frame, frame_size_complete);
    if (result != frame_size_complete)
    {
        log_message(LOG_ERROR,
                    "Error sending data to 0x%04X! Only %i of %i bytes sent.",
                    address,
                    result,
                    frame_size_complete);
    }
    print_bufferHexTx(frame, frame_size_complete, address);
    free(frame); // free frame buffer
}

/*
* Send ping command to device and wait for response
*
* @param fd: rs485 file descriptor
* @param address: device address
* @return 0 on success, 1 on failure
*/
int sfbus_ping(int fd, u_int16_t address)
{
    char *cmd = "\xFE";                           // command byte for ping
    char *buffer = malloc(SFBUS_MAX_BUFFER_SIZE); // allocate rx buffer
    sfbus_send_frame_v2(fd, address, strlen(cmd), cmd);
    int len = sfbus_recv_frame_wait(fd, 0xFFFF, buffer);
    if (len == 5 && *buffer == (char)0xFF) // expect 0xFF on successful ping
    {
        log_message(LOG_DEBUG, "Ping to 0x%04X okay!", address);
        free(buffer);
        return 0;
    }
    else
    {
        log_message(LOG_WARNING, "Ping to 0x%04X failed! Invalid or no response.", address);
        free(buffer);
        return 1;
    }
}

/*
* Read data from device EEPROM
* Verifies received data and checks for ACK byte 
*
* @param fd: rs485 file descriptor
* @param address: device address
* @param buffer: buffer to store received data
* @return length of received data on success, -1 on error
*/
int sfbus_read_eeprom(int fd, u_int16_t address, char *buffer)
{
    char *cmd = "\xF0";
    char *_buffer = malloc(256);
    sfbus_send_frame_v2(fd, address, strlen(cmd), cmd);
    int len = sfbus_recv_frame_wait(fd, 0xFFFF, _buffer);
    if (len != 6)
    {
        free(_buffer);
        log_message(LOG_WARNING, "Invalid data received from 0x%04X! Unexpected package length.", address);
        return -1;
    }
    if (*(_buffer + 5) != (char)0xAA || *(_buffer) != (char)0xAA)
    {
        free(_buffer);
        log_message(LOG_WARNING, "Invalid data received from 0x%04X! No ACK byte received.", address);
        return -1;
    }
    memcpy(buffer, _buffer + 1, len);
    free(_buffer);
    // printf("Read valid data!\n");
    return len;
}

/*
* Write data to device EEPROM
* Verifies received data and checks for ACK byte
*
* @param fd: rs485 file descriptor
* @param address: device address
* @param wbuffer: buffer with data to write
* @param rbuffer: buffer to store received data
* @return length of received data on success, -1 on error
*/
int sfbus_write_eeprom(int fd, u_int16_t address, char *wbuffer, char *rbuffer)
{
    char *cmd = malloc(5);
    *cmd = (char)0xF1; // write eeprom command
    memcpy(cmd + 1, wbuffer, 4);
    sfbus_send_frame_v2(fd, address, 5, cmd);
    free(cmd);
    // wait for readback
    char *_buffer = malloc(256);
    int len = sfbus_recv_frame_wait(fd, 0xFFFF, _buffer);
    if (len != 6)
    {
        free(_buffer);
        log_message(LOG_WARNING, "Invalid data received from 0x%04X! Unexpected package length.", address);
        return -1;
    }
    if (*(_buffer + 5) != (char)0xAA || *(_buffer) != (char)0xAA)
    {
        free(_buffer);
        log_message(LOG_WARNING, "Invalid data received from 0x%04X! No ACK byte received.", address);
        return -1;
    }
    memcpy(rbuffer, _buffer + 1, len);
    free(_buffer);
    return len;
}

/*
* Send display command to device to set flap position
* 
* @param fd: rs485 file descriptor
* @param address: device address
* @param flap: flap ID to set
* @return 0 on success
*/
int sfbus_display(int fd, u_int16_t address, u_int8_t flap)
{
    char *cmd = malloc(5);
    *cmd = (char)0x10; // write eeprom command
    *(cmd + 1) = flap;
    sfbus_send_frame_v2(fd, address, 2, cmd);
    free(cmd);
    return 0;
}

/*
* Send display command to device to set flap position with full rotation
*
* @param fd: rs485 file descriptor
* @param address: device address
* @param flap: flap ID to set
* @return 0 on success
*/
int sfbus_display_full(int fd, u_int16_t address, u_int8_t flap)
{
    char *cmd = malloc(5);
    *cmd = (char)0x11; // write eeprom command
    *(cmd + 1) = flap;
    sfbus_send_frame_v2(fd, address, 2, cmd);
    free(cmd);
    return 0;
}

/*
* Read device status including voltage operation counter and status flags
* 
* @param fd: rs485 file descriptor
* @param address: device address
* @param voltage: pointer to store read voltage value
* @param counter: pointer to store read operation counter value
* @return status byte on success, 0xFF on error
*/
u_int8_t sfbus_read_status(int fd, u_int16_t address, double *voltage, u_int32_t *counter)
{
    char *cmd = "\xF8";
    char *_buffer = malloc(SFBUS_MAX_BUFFER_SIZE);
    sfbus_send_frame_v2(fd, address, strlen(cmd), cmd);

    int res = sfbus_recv_frame_wait(fd, 0xFFFF, _buffer);
    if (res < 0)
    {
        free(_buffer);
        return 0xFF;
    }
    u_int16_t _voltage = *(_buffer + 2) & 0xFF | ((*(_buffer + 1) << 8) & 0xFF00);
    u_int32_t _counter = *(_buffer + 6) & 0xFF | (((*(_buffer + 3) & 0xFF) << 24)) | (((*(_buffer + 4) & 0xFF) << 16)) |
                         (((*(_buffer + 5) & 0xFF) << 8));

    double __voltage = ((double)_voltage / 1024) * 55;
    if (__voltage > 16)
    {
        __voltage = ((double)_voltage / 1024) * 28.16;
    }

    *voltage = __voltage;
    *counter = (u_int32_t)_counter;
    u_int8_t status = *(_buffer + 0) & 0xFF; // store status byte temporarily
    free(_buffer);                           // free rx buffer to prevent memory leak
    return status;
}

/*
* Reset device
*
* @param fd: rs485 file descriptor  
* @param address: device address
*/
void sfbus_reset_device(int fd, u_int16_t address)
{
    char *cmd = "\x30";
    sfbus_send_frame_v2(fd, address, strlen(cmd), cmd);
}

/*
* Enable or disable motor power on device
*
* @param fd: rs485 file descriptor
* @param address: device address
* @param state: 0 = disable power, >0 = enable power
*/
void sfbus_motor_power(int fd, u_int16_t address, u_int8_t state)
{
    char *cmd = "\x20";
    if (state > 0)
    {
        cmd = "\x21";
    }
    sfbus_send_frame_v2(fd, address, 1, cmd);
}

/*
* Utility function to calculate CRC16 checksum
* Used for SFBus protocol v2.0
*
* @param buffer: buffer to calculate checksum for
* @param len: length of buffer
* @return calculated CRC16 checksum
*/
u_int16_t calc_CRC16(char *buffer, u_int8_t len)
{
    u_int16_t crc16 = 0xFFFF;
    for (u_int8_t pos = 0; pos < len; pos++)
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