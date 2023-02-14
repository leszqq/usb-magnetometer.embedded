#include "stm32l0xx.h"
#include "assert.h"

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
    Y_CH_RESULT = 0xA,
    Z_CH_RESULT = 0xB,
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

typedef struct reading {
    uint16_t x;
    uint16_t y;
    uint16_t z;
} reading_t;
static_assert(sizeof(reading_t) == 6);

void sensor_init();

void sensor_configure_for_active();

void sensor_put_to_sleep();

void sensor_read(reading_t* result);

void sensor_start_stream();

void sensor_stop_stream();

void sensor_read_register(enum sensor_reg reg, uint16_t* content);