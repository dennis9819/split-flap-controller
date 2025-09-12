#include "sfbus.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

void print_charHex(char *buffer, int length) {
  int _tlength = length;
  while (_tlength > 0) {
    printf("%02X", (*buffer & 0xFF));
    buffer++;
    _tlength--;
  }
}

void print_bufferHexRx(char *buffer, int length, u_int16_t address) {
  printf("Rx (0x%04X): 0x", address);
  print_charHex(buffer, length);
  printf(" | %i bytes received\n", length);
}

void print_bufferHexTx(char *buffer, int length, u_int16_t address) {
  printf("Tx (0x%04X): 0x", address);
  print_charHex(buffer, length);
  printf(" | %i bytes sent\n", length);
}

ssize_t sfbus_recv_frame_wait(int fd, u_int16_t address, char *buffer) {
  ssize_t len = 0;
  int retryCount = 2;
  do {
    len = sfbus_recv_frame(fd, address, buffer);
    retryCount--;
    if (retryCount == 0) {
      fprintf(stderr, "Rx timeout\n");
      return -1;
    }
  } while (len <= 0);
  print_bufferHexRx(buffer, len - 3, address);
  return len;
}

ssize_t sfbus_recv_frame(int fd, u_int16_t address, char *buffer) {
  // wait for start byte
  char byte = 0x00;
  int retryCount = 3;
  while (byte != '+') {
    byte = 0x00;
    read(fd, &byte, 1);
    retryCount--;
    if (retryCount == 0) {
      return -1;
    }
  }
  u_int8_t frm_version;
  u_int8_t frm_addr_l;
  u_int8_t frm_addr_h;
  u_int8_t frm_length;
  u_int8_t frm_eof;

  read(fd, &frm_version, 1);
  read(fd, &frm_length, 1);
  read(fd, &frm_addr_l, 1);
  read(fd, &frm_addr_h, 1);

  u_int16_t dst_addr = frm_addr_l | (frm_addr_h << 8);
  if (dst_addr != address) {
    return 0;
  }

  u_int8_t frm_length_counter = frm_length - 3;
  // read all bytes:
  while (frm_length_counter > 0) {
    read(fd, buffer, 1);
    buffer++;
    frm_length_counter--;
  }
  read(fd, &frm_eof, 1);

  if (frm_eof == '$') {
    return frm_length;
  } else {
    return -1;
  }
}

void sfbus_send_frame(int fd, u_int16_t address, u_int8_t length,
                      char *buffer) {
  int frame_size_complete = length + 6;
  char *frame = malloc(frame_size_complete);
  char *frame_ptr = frame;

  *frame = '+'; // startbyte
  frame++;
  *frame = 0; // protocol version
  frame++;
  *frame = length + 3; // length
  frame++;
  *frame = (address);
  frame++;
  *frame = ((address >> 8));
  frame++;
  while (length > 0) {
    *frame = *buffer;
    length--;
    buffer++;
    frame++;
  }
  *frame = '$'; // startbyte

  //rs485_trdir(fd, 0);

  int result = write(fd, frame_ptr, frame_size_complete);
  print_bufferHexTx(frame_ptr + 5, frame_size_complete - 6, address);
  free(frame_ptr);
  // tcdrain(fd);
  //usleep(470 * (frame_size_complete + 1));
  //rs485_trdir(fd, 1);
}

int sfbus_ping(int fd, u_int16_t address) {
  char *cmd = "\xFE";
  char *buffer = malloc(64);
  sfbus_send_frame(fd, address, strlen(cmd), cmd);
  int len = sfbus_recv_frame_wait(fd, 0xFFFF, buffer);
  if (len == 4 && *buffer == (char)0xFF) {
    printf("Ping okay!\n");
    free(buffer);
    return 0;
  } else {
    printf("Ping invalid response!\n");
    free(buffer);
    return 1;
  }
}

int sfbus_read_eeprom(int fd, u_int16_t address, char *buffer) {
  char *cmd = "\xF0";
  char *_buffer = malloc(256);
  sfbus_send_frame(fd, address, strlen(cmd), cmd);
  int len = sfbus_recv_frame_wait(fd, 0xFFFF, _buffer);
  if (len != 9) {
    printf("Invalid data!\n");
    return -1;
  }
  if (*(_buffer + 5) != (char)0xAA || *(_buffer) != (char)0xAA) {
    printf("Invalid data!\n");
    return -1;
  }
  memcpy(buffer, _buffer + 1, len - 4);
  free(_buffer);
  // printf("Read valid data!\n");
  return len;
}

int sfbus_write_eeprom(int fd, u_int16_t address, char *wbuffer,
                       char *rbuffer) {
  char *cmd = malloc(5);
  *cmd = (char)0xF1; // write eeprom command
  memcpy(cmd + 1, wbuffer, 4);
  sfbus_send_frame(fd, address, 5, cmd);
  free(cmd);
  // wait for readback
  char *_buffer = malloc(256);
  int len = sfbus_recv_frame_wait(fd, 0xFFFF, _buffer);
  if (len != 9) {
    printf("Invalid data!\n");
    return -1;
  }
  if (*(_buffer + 5) != (char)0xAA || *(_buffer) != (char)0xAA) {
    printf("Invalid data!\n");
    return -1;
  }
  memcpy(rbuffer, _buffer + 1, len - 4);
  free(_buffer);
  // printf("Read valid data!\n");
  return len;
}

int sfbus_display(int fd, u_int16_t address, u_int8_t flap) {
  char *cmd = malloc(5);
  *cmd = (char)0x10; // write eeprom command
  *(cmd + 1) = flap;
  sfbus_send_frame(fd, address, 2, cmd);
  free(cmd);
  return 0;
}

int sfbus_display_full(int fd, u_int16_t address, u_int8_t flap) {
  char *cmd = malloc(5);
  *cmd = (char)0x11; // write eeprom command
  *(cmd + 1) = flap;
  sfbus_send_frame(fd, address, 2, cmd);
  free(cmd);
  return 0;
}

u_int8_t sfbus_read_status(int fd, u_int16_t address, double *voltage,
                           u_int32_t *counter) {
  char *cmd = "\xF8";
  char *_buffer = malloc(256);
  sfbus_send_frame(fd, address, strlen(cmd), cmd);
  int res = sfbus_recv_frame_wait(fd, 0xFFFF, _buffer);
  if (res < 0){
    return 0xFF;
  }
  u_int16_t _voltage = *(_buffer + 2) & 0xFF | ((*(_buffer + 1) << 8) & 0xFF00);
  u_int32_t _counter =
      *(_buffer + 6) & 0xFF | (((*(_buffer + 3) & 0xFF) << 24)) |
      (((*(_buffer + 4) & 0xFF) << 16)) | (((*(_buffer + 5) & 0xFF) << 8));

  double __voltage = ((double)_voltage / 1024) * 55;

  *voltage = __voltage;
  *counter = (u_int32_t)_counter;
  return *_buffer;
}

void sfbus_reset_device(int fd, u_int16_t address) {
  char *cmd = "\x30";
  sfbus_send_frame(fd, address, strlen(cmd), cmd);
}

void sfbus_motor_power(int fd, u_int16_t address, u_int8_t state) {
  char *cmd = "\x20";
  if (state > 0) {
    cmd = "\x21";
  }
  sfbus_send_frame(fd, address, 1, cmd);
}