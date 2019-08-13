#ifndef rf_h_INCLUDED
#define rf_h_INCLUDED

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "driver/rmt.h"

#define RECIEVE_MAX_BUFFER_LEN 2000

#define RMT_CLK_DIV 10			//devide clock to 1MHZ

#define RX_TICKS_THRESH 255		//at 1MHZ signals shorter than 150us are filtered out
#define RX_IDLE_THRESHOLD 200 	//at 1MHZ signals longer than 3ms are filtered out 

#define RMT_TX_CARRIER_DS 0

#define TRANSMITTION_MIN_LEN 18

/*  
 *  C abstraction to use Remote Control Interface of ESP32 for 433Mhz transmittion and reception
 *  Copyright: Felix Richter 2019 <judge@felixrichter.tech>
 */


typedef struct{
	uint8_t high_time;
	uint8_t low_time;
} Pulse;

typedef struct{
	Pulse sync;
	Pulse zero;
	Pulse one;
	bool inverted;
} Protocol433mhz;

typedef struct{
	uint32_t data;
	unsigned int code_lenght;
	Protocol433mhz* protocol;
	uint16_t pulse_length;
	uint8_t repeat;
} Message433mhz;

extern Protocol433mhz protocols_433mhz[];  

extern bool rx_init;
extern uint8_t rx_channel;

extern bool tx_init;
extern uint8_t tx_channel;

typedef enum {
	PROTO1,
	PROTO2,
	PROTO3,
	PROTO4,
	PROTO5,
	PROTO6,
	PROTO7,
	PROTOCOL_COUNT
} ProtocolName433mhz;


void initReceive(uint8_t channel, uint8_t pin);
void initTransmit(uint8_t channel, uint8_t pin);

RingbufHandle_t getReceiverBuffer();

void startReceive();
void stopReceive();

bool decodeSignal(rmt_item32_t* data, size_t size, Message433mhz* message);



#endif // rf_h_INCLUDED

