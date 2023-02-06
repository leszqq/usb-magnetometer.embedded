#include "sensor.h"
#include "sensor_defines.h"
#include "spi.h"

#define TX_BUFFER_SIZE                  16
#define RX_BUFFER_SIZE                  16

enum sensor_state {
    sensor_state_config,
    sensor_state_standby,
    sensor_state_streaming
};



// struct sensor_configuration {
//     //TODO: write config to sensor, 
// }

static struct {
    enum sensor_state sensor_state;
    uint8_t tx_buffer[TX_BUFFER_SIZE];
    uint8_t rx_buffer[RX_BUFFER_SIZE];
} ctx;

static void disable_crc();
static void write_reg(enum sensor_reg reg, uint16_t content);

void sensor_init(){
    ctx.sensor_state = sensor_state_config;
    uint16_t tmp;
    sensor_read_register(AFE_STATUS, &tmp);
    disable_crc();
}

void sensor_configure_for_active(){
    write_reg(SENSOR_CONFIG, 0x07 << 6);    // enable data acquisition for X, Y and Z axes
    write_reg(ALERT_CONFIG, 1 << 8);        // signal generated on #ALERT pin on complete conversion

    // write_reg(DEVICE_CONFIG, 0x02 << 4);    // continuous conversion
}

// void sensor_put_to_sleep();

// void sensor_start_stream();

// void sensor_stop_stream();

void sensor_read_register(enum sensor_reg reg, uint16_t* content){
    //while (HAL_SPI_STATE_READY != HAL_SPI_GetState(&hspi1)) {};
    ctx.tx_buffer[0] = reg | 0x80;
    ctx.tx_buffer[1] = 0x00;
    ctx.tx_buffer[2] = 0x00;
    ctx.tx_buffer[3] = 0x00;
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi1, ctx.tx_buffer, ctx.rx_buffer, 4, 100);
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);

    if (HAL_OK != status) {
        Error_Handler();
    }
    *content = 0x0000 | (ctx.rx_buffer[2] << 8) | ctx.rx_buffer[1];
}

static void disable_crc(){
    ctx.tx_buffer[0] = DISABLE_CRC_B3;
    ctx.tx_buffer[1] = DISABLE_CRC_B2;
    ctx.tx_buffer[2] = DISABLE_CRC_B1;
    ctx.tx_buffer[3] = DISABLE_CRC_B0;
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, (uint8_t *)ctx.tx_buffer, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
    if (HAL_OK != status) {
        Error_Handler();
    }
}

static void write_reg(enum sensor_reg reg, uint16_t content){
    //while (HAL_SPI_STATE_READY != HAL_SPI_GetState(&hspi1)) {}
    ctx.tx_buffer[0] = reg;
    ctx.tx_buffer[1] = (uint8_t) (content >> 8);
    ctx.tx_buffer[2] = (uint8_t) content;
    ctx.tx_buffer[3] = 0;
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, (uint8_t *)ctx.tx_buffer, 4, 100);
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
    if (HAL_OK != status) {
        Error_Handler();
    }
}