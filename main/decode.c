#include "tasks.h"
#include "rf_433mhz.h"
#include "time.h"
#include "state.h"
#include <sys/time.h>

double time_diff(struct timeval x, struct timeval y) {
	double x_ms, y_ms;

	x_ms = (double)x.tv_sec * 1000000 + (double)x.tv_usec;
	y_ms = (double)y.tv_sec * 1000000 + (double)y.tv_usec;

	return y_ms - x_ms;
}

bool updateMqttStatus(Light* l);

void handle_recieved_433mhz_msg(Message433mhz* msg) {
	//printf("Function handle_recieved_433mhz_msg \n");
	int light_index = 0;
	bool changed = false;
	for (;light_index<runstate.lightcount;light_index++) {
		Light* l = &runstate.lights[light_index];
		if ( msg->data == l->code) {
			//recieved on signal
			if (!l->state) {
				changed = true;
			}
			l->state = true;
		} else if ( msg->data == l->code + l->off_set ) {
			//recieved off signal
			if (l->state) {
				changed = true;
			}
			l->state = false;
		} else {
			continue;
		}
		if (changed) {
			saveState(&runstate);
			updateMqttStatus(l);
			break;
		}
	}
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

	while(rb) {
		size_t rx_size = 0;
		rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, RECIEVE_MAX_BUFFER_LEN);

		if (item) {
			//Todo decode
			Message433mhz msg;

			if (decodeSignal(item, rx_size/4, &msg)) {
				if ( msg_cmp(&previous_msg, &msg) ){
					printf("recived data %ld\n", previous_msg.data);
					handle_recieved_433mhz_msg(&previous_msg);
				} else {
					previous_msg = msg;
				}
			}

			vRingbufferReturnItem(rb, (void *) item);
			item = NULL;
		}

	}
}
