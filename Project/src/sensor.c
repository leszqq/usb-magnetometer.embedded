#include "sensor.h"
#include "sensor_defines.h"
#include "spi.h"
#include <stdbool.h>
#include "server.h"

#define TX_BUFFER_SIZE                  16
#define RX_BUFFER_SIZE                  16
#define STREAM_CHUNK_SIZE               80

enum sensor_state {
    sensor_state_config = 0,
    sensor_state_streaming,
    sensor_state_single_reading
};


static struct {
    enum sensor_state sensor_state;
    uint8_t tx_buffer[TX_BUFFER_SIZE];
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    bool data_ready;
    uint16_t samples_in_stream_buffer;
    reading_t first_stream_buffer[STREAM_CHUNK_SIZE], second_stream_buffer[STREAM_CHUNK_SIZE];
    bool use_first_readings_buffer;
    bool stop_stream_request;
    bool last_stream_chunk;
} ctx;

static void disable_crc();
static void write_reg(enum sensor_reg reg, uint16_t content, bool trigger_conversion);
static void enable_drdy_interrupt();
static void disable_drdy_interrupt();

void sensor_reset(){
    if (sensor_state_streaming == ctx.sensor_state) {
        write_reg(DEVICE_CONFIG, 0 << 4, false);
    }

    ctx.sensor_state = sensor_state_config;
    uint16_t tmp;
    sensor_read_register(AFE_STATUS, &tmp);
    disable_crc();
    write_reg(SENSOR_CONFIG, 0x07 << 6, false);    // enable data acquisition for X, Y and Z axes
    write_reg(ALERT_CONFIG, 1 << 8, false);        // signal generated on #ALERT pin on complete conversion
}

void sensor_run(){
    reading_t result;
    uint16_t tmp;
    if (ctx.data_ready) {
        if (ctx.sensor_state == sensor_state_streaming) {

            sensor_read_register(CONV_STATUS, &tmp);
            sensor_read_register(X_CH_RESULT, &result.x);
            sensor_read_register(Y_CH_RESULT, &result.y);
            sensor_read_register(Z_CH_RESULT, &result.z);
            
            ctx.data_ready = false;

            reading_t* stream_buffer = ctx.use_first_readings_buffer ? ctx.first_stream_buffer : ctx.second_stream_buffer;
            stream_buffer[ctx.samples_in_stream_buffer++] = result;

            if (ctx.samples_in_stream_buffer == STREAM_CHUNK_SIZE) {
                ctx.samples_in_stream_buffer = 0;
                ctx.use_first_readings_buffer = !ctx.use_first_readings_buffer;

                if (ctx.stop_stream_request) {
                    ctx.stop_stream_request = false;
                    write_reg(DEVICE_CONFIG, 0, false);
                    disable_drdy_interrupt();
                    reading_t empty_reading = {0};
                    for (int16_t i = 0; i < STREAM_CHUNK_SIZE; i++) {
                        stream_buffer[i] = empty_reading;
                    }
                }

                server_send_measurmenets_chunk(stream_buffer, sizeof(ctx.first_stream_buffer));
            }
        } else if (ctx.sensor_state == sensor_state_single_reading) {
            disable_drdy_interrupt();
            sensor_read_register(CONV_STATUS, &tmp);
            sensor_read_register(X_CH_RESULT, &result.x);
            sensor_read_register(Y_CH_RESULT, &result.y);
            sensor_read_register(Z_CH_RESULT, &result.z);
            ctx.data_ready = false;
            server_send_reading(&result);
        }
    }
}

void sensor_read(){
    write_reg(SENSOR_CONFIG, 0x07 << 6, true);
    ctx.sensor_state = sensor_state_single_reading;
    enable_drdy_interrupt();
}

void sensor_start_stream(){
    write_reg(DEVICE_CONFIG, (0x02 << 4) | (0x02 << 12), true);
    ctx.sensor_state = sensor_state_streaming;
    enable_drdy_interrupt();
}

void sensor_stop_stream() {
    ctx.stop_stream_request = true;
}

void sensor_read_register(enum sensor_reg reg, uint16_t* content){
    while (HAL_SPI_STATE_READY != HAL_SPI_GetState(&hspi1)) {
        Error_Handler();
    };
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
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, ctx.tx_buffer, 4, HAL_MAX_DELAY);
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
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, ctx.tx_buffer, 4, 100);
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
    if (HAL_OK != status) {
        Error_Handler();
    }
}

static void enable_drdy_interrupt() {
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
}

static void disable_drdy_interrupt() {
    HAL_NVIC_DisableIRQ(EXTI2_3_IRQn);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if (MAG_INT_Pin == GPIO_Pin){
        if (ctx.data_ready) {
            Error_Handler();
        }
        ctx.data_ready = true;
    }
}