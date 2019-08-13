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

uint8_t rx_channel = 255;
uint8_t tx_channel = 255;

void initReceive(uint8_t channel, uint8_t pin) {
	//setup hardware reciever
	rmt_config_t rmt_rx;

	rmt_rx.channel = channel;
	rmt_rx.gpio_num = pin;
	rmt_rx.clk_div = RMT_CLK_DIV;
	rmt_rx.mem_block_num = 1;
	rmt_rx.rmt_mode = RMT_MODE_RX;

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

	rmt_rx_start(rx_channel, 1);
}

void stopReceive() {
	if (!rx_init) {
		return;
	}

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

	//Todo build waveform and send
}


bool decodeSignal(rmt_item32_t *data, size_t size, Message433mhz* message) {
	//only try to decode signals with enought datapoints
	if (size < TRANSMITTION_MIN_LEN ) {
		return false;
	}
	if (data[0].duration0 <= 2) {
		return false;
	}

	for (int i=0; i<size; i++) {
		printf("Data:\t%i\t%i\t:\t%i\t%i\n", data[i].level0, data[i].duration0, data[i].level1, data[i].duration1);
	}

	//try to decode signal for every known protocol
	for(int i=0; i < PROTOCOL_COUNT ; i++) {
		message->data = 0;
		Protocol433mhz* proto = &protocols_433mhz[i];

		unsigned int sync_len = (proto->sync.low_time > proto->sync.high_time) ? proto->sync.low_time : proto->sync.high_time;

		/* For protocols that start low, the sync period looks like
		 *               _________
		 * _____________|         |XXXXXXXXXXXX|
		 *
		 * |--1st dur--|-2nd dur-|-Start data-|
		 *
		 * The 3rd saved duration starts the data.
		 *
		 * For protocols that start high, the sync period looks like
		 *
		 *  ______________
		 * |              |____________|XXXXXXXXXXXXX|
		 *
		 * |-filtered out-|--1st dur--|--Start data--|
		 *
		 * The 2nd saved duration starts the data
		 */
	}


	return true;
}
