#include "rf_433mhz.h"

#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

Protocol433mhz protocols_433mhz[] = {
	{ { 1, 31 }, { 1, 3 }, { 3, 1 }, false },			// protocol 1
	{ {  1, 10 }, {  1,  2 }, {  2,  1 }, false },		// protocol 2
	{ { 30, 71 }, {  4, 11 }, {  9,  6 }, false },		// protocol 3
	{ {  1,  6 }, {  1,  3 }, {  3,  1 }, false },		// protocol 4
	{ {  6, 14 }, {  1,  2 }, {  2,  1 }, false },		// protocol 5
	{ { 23,  1 }, {  1,  2 }, {  2,  1 }, true },		// protocol 6 (HT6P20B)
	{ {  2, 62 }, {  1,  6 }, {  6,  1 }, false }		// protocol 7 (HS2303-PT, i. e. used in AUKEY Remote)
};

bool rx_init = false;
bool tx_init = false;
bool rx_running = false;

uint8_t rx_channel = 255;
uint8_t tx_channel = 255;

static inline unsigned int diff(int A, int B) {
	  return abs(A - B);
}

void initReceive(uint8_t channel, uint8_t pin) {
	//setup hardware reciever
	rmt_config_t rmt_rx;

	rmt_rx.channel = channel;
	rmt_rx.gpio_num = pin;
	rmt_rx.clk_div = RMT_CLK_DIV;
	rmt_rx.mem_block_num = 1;
	rmt_rx.rmt_mode = RMT_MODE_RX;
	//wtf: this flags is just stupid it enables the REF_TICK as clk src
	rmt_rx.flags = RMT_CHANNEL_FLAGS_ALWAYS_ON;

	rmt_rx.rx_config.filter_en = true;
	rmt_rx.rx_config.filter_ticks_thresh = RX_TICKS_THRESH;
	rmt_rx.rx_config.idle_threshold = RX_IDLE_THRESHOLD;

	rmt_config(&rmt_rx);
	rmt_driver_install(rmt_rx.channel, RECIEVE_MAX_BUFFER_LEN, 0);

	//set global vars to indicate that recieving is now configured
	rx_init = true;
	rx_channel = channel;
}

void initTransmit(uint8_t channel, uint8_t pin) {
	//setup hardware transmit
	rmt_config_t rmt_tx;

	rmt_tx.channel = channel;
	rmt_tx.gpio_num = pin;
	rmt_tx.clk_div = RMT_CLK_DIV;
	rmt_tx.mem_block_num = 1;
	rmt_tx.rmt_mode = RMT_MODE_TX;
	//wtf: this flags is just stupid it enables the REF_TICK as clk src
	rmt_tx.flags = RMT_CHANNEL_FLAGS_ALWAYS_ON;

	rmt_tx.tx_config.loop_en = false;
	rmt_tx.tx_config.carrier_en = RMT_TX_CARRIER_DS;
	rmt_tx.tx_config.idle_level = 0;
	rmt_tx.tx_config.idle_output_en = true;

	rmt_config(&rmt_tx);
	rmt_driver_install(rmt_tx.channel, 0, 0);

	//set global vars to indicate that transmitting is now configured
	tx_init = true;
	tx_channel = channel;
}

void startReceive() {
	if (!rx_init) {
		printf("Can not start recieve! Please initialize the reciever first!");
		return;
	}
	rx_running = true;
	rmt_rx_start(rx_channel, 1);
}

void stopReceive() {
	if (!rx_init) {
		return;
	}
	rx_running = false;
	rmt_rx_stop(rx_channel);
}

RingbufHandle_t getReceiverBuffer() {
	RingbufHandle_t rb = NULL;
	if (!rx_init) {
		printf("Can not start receive! Please initialize the receiver first!");
		return rb;
	}

	rmt_get_ringbuf_handle(rx_channel, &rb);
	return rb;
}

void sendMessage(Message433mhz *message) {
	if (!tx_init) {
		printf("Can not send message! Please initialize the transmitter first!");
		return;
	}

	if (message->pulse_length == 0) {
		printf("Can not send message with zero pulse lenght!");
		return;
	}

	if (message->protocol == NULL) {
		printf("Can not send message with no protocol!");
		return;
	}
	bool was_rx_running = rx_running;
    if (was_rx_running) {
        stopReceive();
    }

	uint16_t item_num = message->code_lenght+2; // sync + message_length + zero termination	
	uint16_t size = item_num * sizeof(rmt_item32_t); 
	rmt_item32_t* signal = malloc(size);

	uint8_t level0 = message->protocol->inverted ? 0 : 1; 
	uint8_t level1 = message->protocol->inverted ? 1 : 0; 

	//Todo build waveform and send
	signal[0].level0 = level0;
	signal[0].duration0 = message->pulse_length * message->protocol->sync.high_time;
	signal[0].level1 = level1;
	signal[0].duration1 = message->pulse_length * message->protocol->sync.low_time;

	for (int i=1;i<=message->code_lenght;i++) {
		if ( message->data & ( 1 << (message->code_lenght-i) )) {
			//send one
			signal[i].level0 = level0;
			signal[i].duration0 = message->pulse_length * message->protocol->one.high_time;
			signal[i].level1 = level1;
			signal[i].duration1 = message->pulse_length * message->protocol->one.low_time;
		} else {
			//send zero
			signal[i].level0 = level0;
			signal[i].duration0 = message->pulse_length * message->protocol->zero.high_time;
			signal[i].level1 = level1;
			signal[i].duration1 = message->pulse_length * message->protocol->zero.low_time;
		}
	}
	signal[message->code_lenght+1].level0 = level0;
	signal[message->code_lenght+1].duration0 = 0;
	signal[message->code_lenght+1].level0 = level1;
	signal[message->code_lenght+1].duration1 = 0;

	for (int i=0; i<message->repeat; i++) {
		rmt_write_items(tx_channel, signal, item_num, true);
		rmt_wait_tx_done(tx_channel, portMAX_DELAY);
	}
	free(signal);

    if (was_rx_running) {
        startReceive();
    }
}


bool decodeSignal(rmt_item32_t *data, size_t size, Message433mhz* message) {
	//only try to decode signals with enought datapoints
	if (size < TRANSMITTION_MIN_LEN ) {
		return false;
	}

	//valid signals end with an duration of zero in last data duration
	if (data[size-1].duration1 != 0) {
		return false;
	}

	//printf ("--------------------------------------------\n");
	//for (int i=0; i<size; i++) {
	//	printf("%d\t%d\t:\t%d\t%d\n", data[i].level0, data[i].duration0, data[i].level1, data[i].duration1);
	//}
	//printf ("--------------------------------------------\n");

	//decode signal for every known protocol
	for(int i=0; i < PROTOCOL_COUNT ; i++) {
		message->data = 0;
		Protocol433mhz* proto = &protocols_433mhz[i];

		//if signal levels don't match the protocols levels try next protocol
		if (( proto->inverted && (data[0].level0 == 1 && data[0].level1 == 0)) 
		   || ( !proto->inverted && (data[0].level0 == 0 && data[0].level1 == 1)) ) { 
			continue;
		}

		bool first_longer = (data[0].duration0 > data[0].duration1) ? true : false;
		uint16_t div = 1;

		if (first_longer) {
			if (proto->zero.high_time > proto->zero.low_time) {
				div = proto->zero.high_time;
			} else {
				div = proto->one.high_time;
			}
		} else {
			if (proto->zero.high_time < proto->zero.low_time) {
				div = proto->zero.low_time;
			} else {
				div = proto->one.low_time;
			}
		}

		uint32_t pulse_len = first_longer ? data[0].duration0 : data[0].duration1;
		pulse_len /= div;
		uint32_t rec_tolerance = pulse_len * RX_DELAY_TOLERANCE / 100;
		bool failed = false;

		for (int j=0; j<size-1; j++) {
			message->data <<= 1;
			if ( diff(data[j].duration0, pulse_len*proto->zero.high_time) < rec_tolerance && 
				diff(data[j].duration1, pulse_len*proto->zero.low_time) < rec_tolerance ){
				//recieved zero
			} else if ( diff(data[j].duration0, pulse_len*proto->one.high_time) < rec_tolerance && 
				diff(data[j].duration1, pulse_len*proto->one.low_time) < rec_tolerance ) {
				//recieved one
				message->data |= 1;
			} else {
				failed = true;
				break;
			}
		}
		if (!failed) {
			message->protocol = proto;
			message->code_lenght = size;
			message->pulse_length = pulse_len;
			message->repeat = 1;
			return true;
		}
	}

	return false;
}

bool msg_cmp(Message433mhz* msg1, Message433mhz* msg2) {
	if (msg1->data != msg2->data) {
		return false;
	}

	//if (msg1->code_lenght != msg2->code_lenght) {
	//	return false;
	//}

	//if ( diff(msg1->pulse_length, msg2->pulse_length) > RX_DELAY_TOLERANCE) {
	//	return false;
	//}

	if ( msg1->protocol != msg2->protocol ) {
		return false;
	}

	return true;
}
