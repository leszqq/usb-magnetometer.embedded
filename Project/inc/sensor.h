#include "stm32l0xx.h"
#include "assert.h"


#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_

enum sensor_reg {
    DEVICE_CONFIG = 0x00,
    SENSOR_CONFIG = 0x01,
    SYSTEM_CONFIG = 0x02,
    ALERT_CONFIG = 0x03,
    X_THRX_CONFIG = 0x04,
    Y_THRX_CONFIG = 0x05,
    Z_THRX_CONFIG = 0x06,
    T_THRX_CONFIG = 0x07,
    CONV_STATUS = 0x08,
    X_CH_RESULT = 0x09,
    Y_CH_RESULT = 0x0A,
    Z_CH_RESULT = 0x0B,
    TEMP_RESULT = 0x0C,
    AFE_STATUS = 0x0D,
    SYS_STATUS = 0x0E,
    TEST_CONFIG = 0x0F,
    OSC_MONITOR = 0x10,
    MAG_GAIN_CONFIG = 0x11,
    MAG_OFFSET_CONFIG = 0x12,
    ANGLE_RESULT = 0x13,
    MAGNITUDE_RESULT = 0x14
};

enum axis {
    AXIS_X = 0,
    AXIS_Y,
    AXIS_Z,
    AXIS_ALL
};

enum sensor_range {
    sensor_range_plus_minus_50_mt = 0,
    sensor_range_plus_minus_25_mt = 1,
    sensor_range_plus_minus_100_mt = 2
};

typedef struct reading {
    uint16_t x;
    uint16_t y;
    uint16_t z;
} reading_t;
static_assert(sizeof(reading_t) == 6);

void sensor_reset();
void sensor_run();
void sensor_read();
void sensor_start_stream();
void sensor_stop_stream();
void sensor_read_register(enum sensor_reg reg, uint16_t* content);
void sensor_set_range(enum sensor_range range);

#endif /* INC_SENSOR_H_ */