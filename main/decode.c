#include "tasks.h"
#include "rf_433mhz.h"

void RecieverDecoderTask(void *arg) {
	RingbufHandle_t rb = getReceiverBuffer();
	startReceive();

	Message433mhz previous_msg;
	previous_msg.data = 0;
	previous_msg.pulse_length = 0;
	previous_msg.code_lenght = 0;
	previous_msg.repeat = 0;
	previous_msg.protocol = NULL;

	while(rb) {
		size_t rx_size = 0;
		rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, RECIEVE_MAX_BUFFER_LEN);

		if (item) {
			//Todo decode
			Message433mhz msg;

			if (decodeSignal(item, rx_size/4, &msg)) {
				if ( msg_cmp(&previous_msg, &msg) ){
					previous_msg.repeat += 1;
				} else {
					if (previous_msg.repeat >= 2) {
						printf("recived data %d\n", previous_msg.data);
					}
					previous_msg = msg;
				}
			}

			vRingbufferReturnItem(rb, (void *) item);
			item = NULL;
		}
	}
}
