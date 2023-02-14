#include "sensor.h"
#include "sensor_defines.h"
#include "spi.h"
#include <stdbool.h>

#define TX_BUFFER_SIZE                  16
#define RX_BUFFER_SIZE                  16

enum sensor_state {
    sensor_state_config,
    sensor_state_streaming
};



// struct sensor_configuration {
//     //TODO: write config to sensor, 
// }

static struct {
    enum sensor_state sensor_state;
    uint8_t tx_buffer[TX_BUFFER_SIZE];
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    bool data_ready;
} ctx;

static void disable_crc();
static void write_reg(enum sensor_reg reg, uint16_t content, bool trigger_conversion);
static void wait_for_conversion();

void sensor_init(){
    ctx.sensor_state = sensor_state_config;
    uint16_t tmp;
    sensor_read_register(AFE_STATUS, &tmp);
    disable_crc();
}

void sensor_configure_for_active(){
    write_reg(SENSOR_CONFIG, 0x07 << 6, false);    // enable data acquisition for X, Y and Z axes
    write_reg(ALERT_CONFIG, 1 << 8, false);        // signal generated on #ALERT pin on complete conversion
}

// void sensor_put_to_sleep();

void sensor_read(reading_t* result){
    write_reg(SENSOR_CONFIG, 0x07 << 6, true);
    wait_for_conversion();
    uint16_t tmp;
    sensor_read_register(CONV_STATUS, &tmp);
    sensor_read_register(X_CH_RESULT, &result->x);
    sensor_read_register(Y_CH_RESULT, &result->y);
    sensor_read_register(Z_CH_RESULT, &result->z);
}

void sensor_start_stream(){
    ctx.sensor_state = sensor_state_streaming;
}

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

static void write_reg(enum sensor_reg reg, uint16_t content, bool trigger_conversion){
    //while (HAL_SPI_STATE_READY != HAL_SPI_GetState(&hspi1)) {}
    ctx.tx_buffer[0] = reg;
    ctx.tx_buffer[1] = (uint8_t) (content >> 8);
    ctx.tx_buffer[2] = (uint8_t) content;
    ctx.tx_buffer[3] = trigger_conversion ? 0x10 : 0x00;
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, (uint8_t *)ctx.tx_buffer, 4, 100);
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
    if (HAL_OK != status) {
        Error_Handler();
    }
}

static void wait_for_conversion(){
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
    uint32_t entry_time = HAL_GetTick();
    while(HAL_GetTick() - entry_time < 100){
        if (ctx.data_ready){
            ctx.data_ready = false;
            HAL_NVIC_DisableIRQ(EXTI2_3_IRQn);
            return;
        }
    }
    Error_Handler();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if (MAG_INT_Pin == GPIO_Pin){
        ctx.data_ready = true;
    }
}