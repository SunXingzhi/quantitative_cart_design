#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <stdint.h>
#include <stdio.h>
typedef struct {
    int fd;
} serial_handle_t;

int serial_open(serial_handle_t *h, const char *path, int baud_rate);
int serial_close(serial_handle_t *h);
int serial_write(serial_handle_t *h, const uint8_t *data, size_t len);
int serial_readline(serial_handle_t *h, char *out, size_t max_len);

#endif // SERIAL_COMM_H
