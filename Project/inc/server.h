/*
 * server.h
 *
 *  Created on: Jan 31, 2023
 *      Author: comra
 */

#ifndef INC_SERVER_H_
#define INC_SERVER_H_

#include "sensor.h"

void server_init(void);
void server_run(void);
void server_send_measurmenets_chunk(const reading_t* const readings, uint16_t size);
void server_send_reading(const reading_t* const reading);



#endif /* INC_SERVER_H_ */
