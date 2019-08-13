#include "tasks.h"
#include "rf_433mhz.h"

void RecieverDecoderTask(void *arg) {
	RingbufHandle_t rb = getReceiverBuffer();
	startReceive();

	while(rb) {
		size_t rx_size = 0;
		rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, RECIEVE_MAX_BUFFER_LEN);

		if (item) {
			//Todo decode
			Message433mhz msg;
			if (decodeSignal(item, rx_size/4, &msg)) {
				printf("Got transmittion of size %i!\n", rx_size/4);
			}

			vRingbufferReturnItem(rb, (void *) item);
			item = NULL;
		}
	}
}
