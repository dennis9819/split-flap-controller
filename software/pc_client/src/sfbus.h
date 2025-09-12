#include "ftdi485.h"

ssize_t sfbus_recv_frame(int fd, u_int16_t address, char *buffer);
ssize_t sfbus_recv_frame_wait(int fd, u_int16_t address, char *buffer);
void sfbus_send_frame(int fd, u_int16_t address, u_int8_t length, char *buffer);
void print_charHex(char *buffer, int length);
int sfbus_ping(int fd, u_int16_t address);
int sfbus_read_eeprom(int fd, u_int16_t address, char* buffer);
int sfbus_write_eeprom(int fd, u_int16_t address, char* wbuffer, char *rbuffer);
int sfbus_display(int fd, u_int16_t address, u_int8_t flap);
int sfbus_display_full(int fd, u_int16_t address, u_int8_t flap);
u_int8_t sfbus_read_status(int fd, u_int16_t address, double *voltage, u_int32_t *counter);
void sfbus_reset_device(int fd, u_int16_t address);
void sfbus_motor_power(int fd, u_int16_t address, u_int8_t state);