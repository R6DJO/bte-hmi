/*
 * modbus.h
 *
 *  Created on: Jan 11, 2024
 *      Author: r6djo
 */

#ifndef INC_MODBUS_H_
#define INC_MODBUS_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "modbus_conf.h"

#define REGISTER_COUNT 8

#define BUFFER_SIZE 64

typedef enum
{
	MB_ERR,
	MB_OK,
	WRONG_ADDRESS,
	WRONG_COMMAND,
	WRONG_REGISTER
} modbus_status_t;

typedef enum
{
	READ_DO = 1,
	READ_DI,
	READ_AO,
	READ_AI,
	WRITE_DO,
	WRITE_AO,
	READ_EXCEPTION,
	DIAGNOSTIC,
	WRITE_DO_MULTI = 15,
	WRITE_AO_MULTI
} modbus_command_t;

typedef struct UART_message
{
	uint16_t msg_length;
	uint8_t msg_data[BUFFER_SIZE];
} UART_message;

typedef struct MODBUS_message
{
	uint16_t start_address;
	uint16_t data_length;
	uint16_t crc;
	uint16_t data[(BUFFER_SIZE - 10) / 2];
	uint8_t device_address;
	uint8_t command;
	uint8_t byte_count;

} MODBUS_message;

typedef struct MODBUS_registers
{
	uint16_t AO[REGISTER_COUNT];
	uint16_t AI[REGISTER_COUNT];
	uint16_t DO[REGISTER_COUNT];
	uint16_t DI[REGISTER_COUNT];
	uint16_t AO_start_address;
	uint16_t AI_start_address;
	uint16_t DO_start_address;
	uint16_t DI_start_address;
	uint8_t AO_count;
	uint8_t AI_count;
	uint8_t DO_count;
	uint8_t DI_count;
	uint8_t MB_address;
} MODBUS_registers;

// MODBUS-master/slave function
uint16_t MODBUS_CRC16(const uint8_t *nData, uint16_t wLength);
uint16_t CRC16(const uint8_t *nData, uint16_t wLength);
// char * itoalz(char *buf, uint8_t value);
// uint8_t hex2int(char buf);
modbus_status_t msg_validate(UART_message *);

// ToDo maybe slave function...
modbus_status_t msg_parse(UART_message *, MODBUS_message *);

// MODBUS-slave functions
modbus_status_t address_validate(const MODBUS_message *rx_msg, const MODBUS_registers *registers);
modbus_status_t response_prepare(MODBUS_message *, MODBUS_registers *,
								 UART_message *);

// MODBUS-master functions
// modbus_status_t prepare_request_mbmsg(uint8_t device_address, uint8_t command, uint16_t start_address, uint16_t count, UART_message *);
modbus_status_t prepare_request_mbmsg(const MODBUS_message *request, UART_message *uart_request);
modbus_status_t response_processing(const MODBUS_message *response, const MODBUS_message *wait_responce, MODBUS_registers *registers);

// modbus_status_t prepare_request_registers(uint8_t device_address,uint16_t start_address,uint16_t count,UART_message*);
// uint8_t prepare_tx_msg(union message *rx_msg, union message *tx_msg, uint16_t *data);
// void prepare_tx_buf(union message *tx_msg, uint8_t *tx_buf);

#endif /* INC_MODBUS_H_ */
