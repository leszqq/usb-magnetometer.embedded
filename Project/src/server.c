/*
 * server.c
 *
 *  Created on: Jan 31, 2023
 *      Author: comra
 */
#include "stdbool.h"
#include "server.h"
#include "usart.h"
#include "sensor.h"
#include <string.h>
#define REQUEST_SIZE			2
#define RX_BUFFER_SIZE			4
#define TX_BUFFER_SIZE			100

enum message_type{
	message_type_test = 0,
	message_type_init_sensor = 1,
	message_type_deinit_sensor = 2,
	message_type_reset_sensor = 3,
	message_type_get_reading = 4,
	message_type_start_stream = 5,
	message_type_stop_stream = 6,
	message_type_read_register = 7
	};

struct request {
	enum message_type type;
	uint8_t data;
};

struct response {
	enum message_type type;
	uint8_t status;
};

static struct {
	bool got_new_packet;
	uint16_t received_byte_count;
	uint8_t rx_buffer[RX_BUFFER_SIZE];
	uint8_t tx_buffer[TX_BUFFER_SIZE];
}ctx;

static void listen_for_request(void);
static void handle_request_if_any(void);
static void send_response(enum message_type type, uint8_t status, const uint8_t* data, uint8_t data_size);

void server_init(void){
	listen_for_request();
}

void server_run(void){
	handle_request_if_any();
}

static void listen_for_request(void) {
	ctx.got_new_packet = false;
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, ctx.rx_buffer, RX_BUFFER_SIZE);
}

void handle_request_if_any(void) {
	if (false == ctx.got_new_packet)
		return;

	// assume that packets are always received flawless for now
	struct request request = {
		.type = ctx.rx_buffer[0],
		.data = ctx.rx_buffer[1]
	};
	switch(request.type){
		case message_type_test: {
			send_response(message_type_test, 0, NULL, 0);
			break;
		}
		case message_type_init_sensor: {
			sensor_init();
			sensor_configure_for_active();
			send_response(message_type_init_sensor, 0, NULL, 0);
			break;
		}
		case message_type_read_register: {
			uint16_t reg_content;
			sensor_read_register(request.data, &reg_content);
			send_response(message_type_read_register, 0, (uint8_t*) &reg_content, sizeof(reg_content));
			break;
		}
		case message_type_get_reading: {
			reading_t result;
			sensor_read(&result);
			send_response(message_type_get_reading, 0, (uint8_t*) &result, sizeof(result));
			break;
		}
		default:
			break;
	}
	listen_for_request();
}

static void send_response(enum message_type type, uint8_t status, const uint8_t* data, uint8_t data_size) {
	if (data_size > TX_BUFFER_SIZE - 2){
		Error_Handler();
	}
	
	ctx.tx_buffer[0] = type;
	ctx.tx_buffer[1] = status;
	if (NULL != data){
		for(uint8_t i = 0; i < data_size; i++) {
			ctx.tx_buffer[2 + i] = data[i];
		}
	}
	HAL_UART_Transmit_DMA(&huart2, ctx.tx_buffer, data_size + 2);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size){
  	if (USART2 == huart->Instance){
		ctx.received_byte_count += size;
		if (REQUEST_SIZE == ctx.received_byte_count){
			ctx.received_byte_count = 0;
			ctx.got_new_packet = true;
		}
	}
}