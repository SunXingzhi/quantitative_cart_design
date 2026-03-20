#include "serial_comm.h"
#include "config.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

int serial_open(serial_handle_t *h, const char *path, int baud_rate)
{
        (void)baud_rate;
        if (!h)
                return -1;
        h->fd = open(path, O_RDWR | O_NOCTTY | O_SYNC);
        if (h->fd < 0)
                return -1;

        struct termios tty;
        if (tcgetattr(h->fd, &tty) != 0)
        {
                close(h->fd);
                return -1;
        }

        cfsetospeed(&tty, B9600);
        cfsetispeed(&tty, B9600);

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_iflag &= ~IGNBRK;
        tty.c_lflag = 0;
        tty.c_oflag = 0;
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = TIMEOUT_TIME_SEC * 10;

        if (tcsetattr(h->fd, TCSANOW, &tty) != 0)
        {
                close(h->fd);
                return -1;
        }
        return 0;
}

int serial_close(serial_handle_t *h)
{
        if (!h || h->fd < 0)
                return -1;
        close(h->fd);
        h->fd = -1;
        return 0;
}

int serial_write(serial_handle_t *h, const uint8_t *data, size_t len)
{
        if (!h || h->fd < 0 || !data)
                return -1;
        return write(h->fd, data, len);
}

int serial_readline(serial_handle_t *h, char *out, size_t max_len)
{
        if (!h || h->fd < 0 || !out)
                return -1;

        size_t offset = 0;
        while (offset + 1 < max_len)
        {
                char c;
                ssize_t n = read(h->fd, &c, 1);
                if (n <= 0)
                        break;
                out[offset++] = c;
                if (c == '\n')
                        break;
        }
        out[offset] = '\0';
        return (int)offset;
}
