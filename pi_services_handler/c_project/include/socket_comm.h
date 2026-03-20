#ifndef SOCKET_COMM_H
#define SOCKET_COMM_H

#include <stdbool.h>

typedef void (*socket_receive_callback_t)(const char *data);

void *socket_connect_thread(void *arg);
void *socket_receive_thread(void *arg);
void *socket_send_thread(void *arg);

bool socket_comm_start(void);
void socket_comm_stop(void);

void socket_set_receive_callback(socket_receive_callback_t cb);

void socket_put_data(const char *data);
char *socket_take_data(void);

#endif // SOCKET_COMM_H
