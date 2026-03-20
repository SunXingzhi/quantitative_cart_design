#ifndef QMC5883P_H
#define QMC5883P_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int fd;
    int i2c_addr;
    bool initialized;
} qmc5883p_t;

bool qmc5883p_init(qmc5883p_t *sensor, const char *dev, int i2c_addr);
bool qmc5883p_close(qmc5883p_t *sensor);
bool qmc5883p_is_data_ready(qmc5883p_t *sensor);
bool qmc5883p_is_overflow(qmc5883p_t *sensor);
bool qmc5883p_read_raw_data(qmc5883p_t *sensor, int16_t *x, int16_t *y, int16_t *z);
bool qmc5883p_read_calibrated_data(qmc5883p_t *sensor, float *xg, float *yg, float *zg);
float qmc5883p_calculate_heading(float xg, float yg, float declination);

#ifdef __cplusplus
}
#endif

#endif // QMC5883P_H
