#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "socket_comm.h"
#include "config.h"
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>

static int server_fd = -1;
static int client_fd = -1;
static pthread_t connect_thread_handle;
static pthread_t receive_thread_handle;
static pthread_t send_thread_handle;
static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
static char data_buffer[1024];
static bool data_ready = false;
static bool running = true;
static socket_receive_callback_t receive_callback = NULL;

void socket_set_receive_callback(socket_receive_callback_t cb)
{
        receive_callback = cb;
}

void socket_put_data(const char *data)
{
        pthread_mutex_lock(&buffer_lock);
        strncpy(data_buffer, data, sizeof(data_buffer) - 1);
        data_buffer[sizeof(data_buffer) - 1] = '\0';
        data_ready = true;
        pthread_mutex_unlock(&buffer_lock);
}

char *socket_take_data(void)
{
        static char tmp[1024];
        pthread_mutex_lock(&buffer_lock);
        if (!data_ready)
        {
                pthread_mutex_unlock(&buffer_lock);
                return NULL;
        }
        strncpy(tmp, data_buffer, sizeof(tmp));
        data_ready = false;
        pthread_mutex_unlock(&buffer_lock);
        return tmp;
}

static void cleanup_socket(void)
{
        if (client_fd >= 0)
        {
                close(client_fd);
                client_fd = -1;
        }
        if (server_fd >= 0)
        {
                close(server_fd);
                server_fd = -1;
        }
}

void *socket_connect_thread(void *arg)
{
        (void)arg;
        struct sockaddr_in server_addr;
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
                return NULL;

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(SOCKET_PORT);

        if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
                perror("bind");
                cleanup_socket();
                return NULL;
        }

        if (listen(server_fd, 1) < 0)
        {
                perror("listen");
                cleanup_socket();
                return NULL;
        }

        while (running)
        {
                client_fd = accept(server_fd, NULL, NULL);
                if (client_fd < 0)
                        continue;
                break;
        }
        return NULL;
}

void *socket_receive_thread(void *arg)
{
        (void)arg;
        char buffer[1024];
        while (running && client_fd >= 0)
        {
                int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (n <= 0)
                {
                        struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000L}; // 100 ms
                        nanosleep(&ts, NULL);
                        continue;
                }
                buffer[n] = '\0';
                // 直接输出，实际可用事件驱动
                puts(buffer);
                if (receive_callback)
                {
                        receive_callback(buffer);
                }
        }
        return NULL;
}

void *socket_send_thread(void *arg)
{
        (void)arg;
        while (running && client_fd >= 0)
        {
                char *data = socket_take_data();
                if (data)
                {
                        send(client_fd, data, strlen(data), 0);
                }
                else
                {
                        struct timespec ts = {.tv_sec = 0, .tv_nsec = 10000000L}; // 10 ms
                        nanosleep(&ts, NULL);
                }
        }
        return NULL;
}

bool socket_comm_start(void)
{
        running = true;
        if (pthread_create(&connect_thread_handle, NULL, socket_connect_thread, NULL) != 0)
                return false;
        if (pthread_create(&send_thread_handle, NULL, socket_send_thread, NULL) != 0)
                return false;
        if (pthread_create(&receive_thread_handle, NULL, socket_receive_thread, NULL) != 0)
                return false;
        return true;
}

void socket_comm_stop(void)
{
        running = false;
        cleanup_socket();
        pthread_join(connect_thread_handle, NULL);
        pthread_join(receive_thread_handle, NULL);
        pthread_join(send_thread_handle, NULL);
}
