/*
 * This file is part of the split-flap project.
 * Copyright (c) 2024-2025 GuniaLabs (www.dennisgunia.de)
 * Authors: Dennis Gunia
 *
 * This program is licenced under AGPL-3.0 license.
 *
 */

#include "ftdi485.h"

/*
* Open RS485 Interface (FT232RL)
*/
int rs485_init(char *device, int baud) {
  int rs485_fd = open(device, O_RDWR);
  if (rs485_fd < 0) {
    printf("Error %i from open: %s\n", errno, strerror(errno));
    return -1;
  }
  // Flush all data
  int result = tcflush(rs485_fd, TCIOFLUSH);
  if (result) {
    perror("tcflush failed"); // just a warning, not a fatal error
    return -1;
  }
  // Set port config and baud
  struct termios options;
  result = tcgetattr(rs485_fd, &options);
  if (result) {
    perror("tcgetattr failed");
    close(rs485_fd);
    return -1;
  }

  // Turn off any options that might interfere with our ability to send and
  // receive raw binary bytes.
  options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
  options.c_oflag &= ~(ONLCR | OCRNL);
  options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

  // Set up timeouts: Calls to read() will return as soon as there is
  // at least one byte available or when 100 ms has passed.
  options.c_cc[VTIME] = 1;
  options.c_cc[VMIN] = 0;

  cfsetospeed(&options, baud);
  cfsetispeed(&options, baud);
  result = tcsetattr(rs485_fd, TCSANOW, &options);
  if (result) {
    perror("tcsetattr failed");
    close(rs485_fd);
    return -1;
  }

  return rs485_fd;
}
