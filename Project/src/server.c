#include "stdbool.h"
#include "server.h"
#include "usart.h"
#include "sensor.h"
#include <string.h>
#define REQUEST_SIZE			2
#define RX_BUFFER_SIZE			4
#define TX_BUFFER_SIZE			10

enum message_type{
	message_type_test = 0,
	message_type_reset = 1,
	message_type_get_reading = 2,
	message_type_start_stream = 3,
	message_type_stop_stream = 4,
	message_type_read_register = 5,
	message_type_set_range = 6
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

void server_send_measurmenets_chunk(const reading_t* const readings, uint16_t size){
	if (readings == NULL) {
		Error_Handler();
	}

	// ctx.tx_buffer[0] = message_type_stream_chunk;
	// ctx.tx_buffer[1] = 0x00;

	// for(uint16_t i = 0; i < size; i++) {
	// 	ctx.tx_buffer[2 + i] = ((uint8_t *)readings)[i];
	// }

	//memcpy((void *) &ctx.tx_buffer[2], (void *) readings, size);
	HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(&huart2, (uint8_t *) readings, size);
	if (status != HAL_OK) {
		Error_Handler();
	}
}

void server_send_reading(const reading_t* const reading){
	send_response(message_type_get_reading, 0, (uint8_t*) reading, sizeof(*reading));
}


static void listen_for_request(void) {
	ctx.got_new_packet = false;
	HAL_UARTEx_ReceiveToIdle_DMA(&huart2, ctx.rx_buffer, RX_BUFFER_SIZE);
}

static void handle_request_if_any(void) {
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
		case message_type_reset: {
			sensor_reset();
			send_response(message_type_reset, 0, NULL, 0);
			break;
		}
		case message_type_read_register: {
			uint16_t reg_content;
			sensor_read_register(request.data, &reg_content);
			send_response(message_type_read_register, 0, (uint8_t*) &reg_content, sizeof(reg_content));
			break;
		}
		case message_type_get_reading: {
			sensor_read();
			break;
		}
		case message_type_start_stream: {
			sensor_start_stream();
			send_response(message_type_start_stream, 0, NULL, 0);
			break;
		}
		case message_type_stop_stream: {
			sensor_stop_stream();
			break;
		}
		case message_type_set_range: {
			sensor_set_range(request.data);
			send_response(message_type_set_range, 0, NULL, 0);
			break;
		}
		default:
			break;
	}
	listen_for_request();
}



static void send_response(enum message_type type, uint8_t status, const uint8_t* data, uint8_t data_size) {	
	ctx.tx_buffer[0] = type;
	ctx.tx_buffer[1] = status;
	if (NULL != data){
		for(uint8_t i = 0; i < data_size; i++) {
			ctx.tx_buffer[2 + i] = data[i];
		}
	}
	HAL_StatusTypeDef hal_status = HAL_UART_Transmit_DMA(&huart2, ctx.tx_buffer, data_size + 2);
	if (hal_status != HAL_OK) {
		Error_Handler();
	}
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size){
  	if (USART2 == huart->Instance){
		ctx.received_byte_count += size;
		if (REQUEST_SIZE == ctx.received_byte_count){
			ctx.received_byte_count = 0;
			ctx.got_new_packet = true;
		} else if (REQUEST_SIZE <= ctx.received_byte_count) {
			// TODO: handle error, ex. wait for 100 ms and call listen_for_request again()
			Error_Handler();
		}
	}
}