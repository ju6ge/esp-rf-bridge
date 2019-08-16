#include "tasks.h"
#include "rf_433mhz.h"
#include "time.h"
#include <sys/time.h>

double time_diff(struct timeval x, struct timeval y) {
	double x_ms, y_ms;

	x_ms = (double)x.tv_sec * 1000000 + (double)x.tv_usec;
	y_ms = (double)y.tv_sec * 1000000 + (double)y.tv_usec;

	return y_ms - x_ms;
}

void RecieverDecoderTask(void *arg) {
	RingbufHandle_t rb = getReceiverBuffer();
	startReceive();

	Message433mhz previous_msg;
	previous_msg.data = 0;
	previous_msg.pulse_length = 0;
	previous_msg.code_lenght = 0;
	previous_msg.repeat = 0;
	previous_msg.protocol = NULL;

	Message433mhz pre_previous_msg;
	pre_previous_msg.data = 0;
	pre_previous_msg.pulse_length = 0;
	pre_previous_msg.code_lenght = 0;
	pre_previous_msg.repeat = 0;
	pre_previous_msg.protocol = NULL;

	while(rb) {
		size_t rx_size = 0;
		rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, RECIEVE_MAX_BUFFER_LEN);

		if (item) {
			//Todo decode
			Message433mhz msg;

			if (decodeSignal(item, rx_size/4, &msg)) {
				if ( msg_cmp(&previous_msg, &msg) ){
					printf("recived data %d\n", previous_msg.data);
				} else {
					previous_msg = msg;
				}
			}

			vRingbufferReturnItem(rb, (void *) item);
			item = NULL;
		}

	}
}
