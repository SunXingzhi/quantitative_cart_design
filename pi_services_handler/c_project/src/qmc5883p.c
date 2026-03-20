#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include "qmc5883p.h"
#include "config.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#define QMC5883P_I2C_DEFAULT_ADDR 0x2C
#define QMC5883P_REG_CHIP_ID 0x00
#define QMC5883P_REG_DATA_X_L 0x01
#define QMC5883P_REG_DATA_X_H 0x02
#define QMC5883P_REG_DATA_Y_L 0x03
#define QMC5883P_REG_DATA_Y_H 0x04
#define QMC5883P_REG_DATA_Z_L 0x05
#define QMC5883P_REG_DATA_Z_H 0x06
#define QMC5883P_REG_STATUS 0x09
#define QMC5883P_REG_CTRL_1 0x0A
#define QMC5883P_REG_CTRL_2 0x0B

#define QMC5883P_STATUS_DRDY 0x01
#define QMC5883P_STATUS_OVFL 0x02

static int combine_bytes(uint8_t msb, uint8_t lsb)
{
    int value = (msb << 8) | lsb;
    if (value & 0x8000)
        value -= 0x10000;
    return value;
}

bool qmc5883p_init(qmc5883p_t *sensor, const char *dev, int i2c_addr)
{
    if (!sensor || !dev)
        return false;

    sensor->fd = open(dev, O_RDWR);
    if (sensor->fd < 0)
        return false;

    if (ioctl(sensor->fd, I2C_SLAVE, i2c_addr) < 0)
    {
        close(sensor->fd);
        sensor->fd = -1;
        return false;
    }

    sensor->i2c_addr = i2c_addr;

    // read chip id using register protocol
    uint8_t reg = QMC5883P_REG_CHIP_ID;
    if (write(sensor->fd, &reg, 1) != 1)
    {
        close(sensor->fd);
        sensor->fd = -1;
        return false;
    }

    uint8_t chip_id = 0;
    if (read(sensor->fd, &chip_id, 1) != 1)
    {
        close(sensor->fd);
        sensor->fd = -1;
        return false;
    }

    // soft reset
    uint8_t buf[2] = {QMC5883P_REG_CTRL_2, 0x80};
    if (write(sensor->fd, buf, 2) != 2)
    {
        close(sensor->fd);
        sensor->fd = -1;
        return false;
    }
    {
        struct timespec ts = {.tv_sec = 0, .tv_nsec = 100000000L};
        nanosleep(&ts, NULL);
    }

    // setup mode
    buf[0] = QMC5883P_REG_CTRL_1;
    buf[1] = 0x2F; // continuous, 200hz, osr=128, range±8
    if (write(sensor->fd, buf, 2) != 2)
    {
        close(sensor->fd);
        sensor->fd = -1;
        return false;
    }

    sensor->initialized = true;
    return true;
}

bool qmc5883p_close(qmc5883p_t *sensor)
{
    if (!sensor || !sensor->initialized)
        return false;
    close(sensor->fd);
    sensor->fd = -1;
    sensor->initialized = false;
    return true;
}

bool qmc5883p_read_register(qmc5883p_t *sensor, uint8_t reg, uint8_t *value)
{
    if (!sensor || !sensor->initialized || !value)
        return false;
    if (write(sensor->fd, &reg, 1) != 1)
        return false;
    if (read(sensor->fd, value, 1) != 1)
        return false;
    return true;
}

bool qmc5883p_is_data_ready(qmc5883p_t *sensor)
{
    uint8_t status;
    if (!qmc5883p_read_register(sensor, QMC5883P_REG_STATUS, &status))
        return false;
    return (status & QMC5883P_STATUS_DRDY) != 0;
}

bool qmc5883p_is_overflow(qmc5883p_t *sensor)
{
    uint8_t status;
    if (!qmc5883p_read_register(sensor, QMC5883P_REG_STATUS, &status))
        return false;
    return (status & QMC5883P_STATUS_OVFL) != 0;
}

bool qmc5883p_read_raw_data(qmc5883p_t *sensor, int16_t *x, int16_t *y, int16_t *z)
{
    if (!sensor || !sensor->initialized || !x || !y || !z)
        return false;

    uint8_t reg = QMC5883P_REG_DATA_X_L;
    if (write(sensor->fd, &reg, 1) != 1)
        return false;

    uint8_t buf[6];
    if (read(sensor->fd, buf, 6) != 6)
        return false;

    *x = combine_bytes(buf[1], buf[0]);
    *y = combine_bytes(buf[3], buf[2]);
    *z = combine_bytes(buf[5], buf[4]);

    return true;
}

bool qmc5883p_read_calibrated_data(qmc5883p_t *sensor, float *xg, float *yg, float *zg)
{
    int16_t x_raw, y_raw, z_raw;
    if (!qmc5883p_read_raw_data(sensor, &x_raw, &y_raw, &z_raw))
        return false;

    const float sensitivity = 3750.0f;
    *xg = x_raw / sensitivity;
    *yg = y_raw / sensitivity;
    *zg = z_raw / sensitivity;

    return true;
}

float qmc5883p_calculate_heading(float xg, float yg, float declination)
{
    float heading_rad = atan2f(yg, xg);
    float heading_deg = heading_rad * 180.0f / PI;
    if (heading_deg < 0.0f)
        heading_deg += 360.0f;
    if (heading_deg >= 360.0f)
        heading_deg -= 360.0f;
    heading_deg += declination;

    if (heading_deg < 0.0f)
        heading_deg += 360.0f;
    else if (heading_deg >= 360.0f)
        heading_deg -= 360.0f;

    return heading_deg;
}
