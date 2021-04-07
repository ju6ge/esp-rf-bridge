#ifndef MQTT_H_INCLUDED
#define MQTT_H_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "tcpip_adapter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#include "state.h"

#define MQTT_BROKER_URI CONFIG_ESP_MQTT_SERVER
#define MQTT_USER CONFIG_ESP_MQTT_USER
#define MQTT_PASS CONFIG_ESP_MQTT_PASSWORD

#define MQTT_TOPIC_PREFIX "rf-bridge"

//actions per light
#define MQTT_TOPIC_STATE "state"
#define MQTT_TOPIC_SET_STATE "set_state"
#define MQTT_TOPIC_PULSELEN "pulselength"
#define MQTT_TOPIC_OFFSET "offset"
#define MQTT_TOPIC_CODE "code"
#define MQTT_TOPIC_PROTOCOL "protocol"
#define MQTT_TOPIC_TOGGLE "toggle"

//special actions
#define MQTT_TOPIC_ALLOFF "all_off"
#define MQTT_TOPIC_ALLON "all_on"
#define MQTT_TOPIC_CONFIG "send_config"
#define MQTT_TOPIC_ADD_LIGHT "addLight"

#define MQTT_MESSAGE_ON "ON"
#define MQTT_MESSAGE_OFF "OFF"

static const char *TAG = "MQTT";

esp_mqtt_client_handle_t client = NULL;
bool mqtt_connected = false;

static void mqtt_start();
void mqtt_react(char* data, uint16_t data_len, char* topic, uint16_t topic_len);
bool updateMqttStatus(Light* l);

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	int msg_id;

	// your_context_t *context = event->context;
	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			mqtt_connected = true;

			uint16_t subscribe_len = strlen(MQTT_TOPIC_PREFIX) + 3;
			char* subscribe_topic = malloc(subscribe_len);

			sprintf(subscribe_topic, "%s/#", MQTT_TOPIC_PREFIX);
			//subscribe to topics
			msg_id = esp_mqtt_client_subscribe(client, subscribe_topic, 0);
			free(subscribe_topic);
			break;

		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			mqtt_connected = false;
			break;

		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_UNSUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;

		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			//printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
			//printf("DATA=%.*s\r\n", event->data_len, event->data);
			mqtt_react(event->data, event->data_len, event->topic, event->topic_len);
			break;

		case MQTT_EVENT_ERROR:
			ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
			break;

		default:
			ESP_LOGI(TAG, "Other event id:%d", event->event_id);
			break;
	}
	return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
		ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
		mqtt_event_handler_cb(event_data);
}

static void mqtt_start(void) {
	const esp_mqtt_client_config_t mqtt_cfg = {
		.uri = MQTT_BROKER_URI,
		.username = MQTT_USER,
		.password = MQTT_PASS
	};

	client = esp_mqtt_client_init(&mqtt_cfg);
	esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
	esp_mqtt_client_start(client);
}

bool is_special_action(char* action_name, char* payload) {
	//printf("Function is_special_action \n");
	if ( strncmp(MQTT_TOPIC_ADD_LIGHT, action_name, strlen(MQTT_TOPIC_ADD_LIGHT)) == 0 ) {
		if (addLight(payload, &runstate)) {
			saveState(&runstate);
		}
		return true;
	}
	if (runstate.lightcount <= 0) {
		return false;
	}
	for(int i=0;i<runstate.lightcount; i++) {	
		Light* l = &runstate.lights[i];
		if ( strncmp(MQTT_TOPIC_ALLON, action_name, strlen(MQTT_TOPIC_ALLON)) == 0 ) {
			l->state = true;
			setLightState(l);
			updateMqttStatus(l);
		} else if ( strncmp(MQTT_TOPIC_ALLOFF, action_name, strlen(MQTT_TOPIC_ALLOFF)) == 0 ) {
			l->state = false;
			setLightState(l);
			updateMqttStatus(l);
		} else if ( strncmp(MQTT_TOPIC_CONFIG, action_name, strlen(MQTT_TOPIC_CONFIG)) == 0 ) {
			Light tmp = *l;
			tmp.update_only_state = false;
			updateMqttStatus(l);
		} else {
			return false;
		}
	}
	return true;
}


void mqtt_react(char* data_orig, uint16_t data_len, char* topic_orig, uint16_t topic_len) {
    uint16_t striplen = strlen(MQTT_TOPIC_PREFIX) + 1;

    char* topic = malloc(topic_len+1);
    memset(topic, 0, topic_len+1);
    char* data = malloc(data_len+1);
    memset(data, 0, data_len+1);

    strncpy(topic, topic_orig + striplen, topic_len - striplen);
    strncpy(data, data_orig, data_len);

	printf("Recieved mqtt message: %s and data %s\n", topic, data);

	//implement reactions to mqtt messages here
	char* light_name = strsep(&topic, "/");
	Light* l = NULL;
	bool found = false;
	for (int i=0;i<runstate.lightcount;i++) {
		if ( strcmp(runstate.lights[i].name, light_name) == 0) {
			found = true;
			l = &runstate.lights[i];
			break;
		}
	}
	if (!found) {
		found = is_special_action(light_name, data);
		if (!found) {
			printf("No light or special action with name %s found! Cannot update unkown light ... ignoring!\n", light_name);
		}
		return;
	}
	uint16_t rest_topic_len = topic_len - striplen - strlen(light_name);
	if (rest_topic_len < 2) {
		printf("No subtopic in MQTT request, don't now what to do with payload ... ignoring!\n");
		return;
	}
	char* subtopic = strsep(&topic, "/");
	bool changed = false;
	//update lights data according to recieved topic
	if ( strncmp(MQTT_TOPIC_STATE, subtopic, strlen(MQTT_TOPIC_STATE)) == 0 ) {
		//keep local state and broker state in sync
		int payload_data;
		if (strncmp(MQTT_MESSAGE_ON, data, strlen(MQTT_MESSAGE_ON)) == 0) {
			payload_data = 1;
		} else {
			payload_data = 0;
		}
		if (payload_data) {
			l->state = true;
		} else {
			l->state = false;
		}
	} else if ( strncmp(MQTT_TOPIC_SET_STATE, subtopic, strlen(MQTT_TOPIC_SET_STATE)) == 0 ) {
		int payload_data;
		if (strncmp(MQTT_MESSAGE_ON, data, strlen(MQTT_MESSAGE_ON)) == 0) {
			payload_data = 1;
		} else {
			payload_data = 0;
		}
		if ((bool)payload_data != l->state) {
			changed = true;
		}
		if (payload_data) {
			l->state = true;
		} else {
			l->state = false;
		}
		if (changed) {
			setLightState(l);
		}
	} else if ( strncmp(MQTT_TOPIC_CODE, subtopic, strlen(MQTT_TOPIC_CODE)) == 0 ) {
		int payload_data = atoi(data);
		l->code = payload_data;
	} else if ( strncmp(MQTT_TOPIC_OFFSET, subtopic, strlen(MQTT_TOPIC_OFFSET)) == 0 ) {
		int payload_data = atoi(data);
		l->off_set = payload_data;
	} else if ( strncmp(MQTT_TOPIC_PULSELEN, subtopic, strlen(MQTT_TOPIC_PULSELEN)) == 0 ) {
		int payload_data = atoi(data);
		l->pulse_length = payload_data;
	} else if ( strncmp(MQTT_TOPIC_PROTOCOL, subtopic, strlen(MQTT_TOPIC_PROTOCOL)) == 0 ) {
		int payload_data = atoi(data);
		l->protocol_num = payload_data;
	} else if ( strncmp(MQTT_TOPIC_TOGGLE, subtopic, strlen(MQTT_TOPIC_TOGGLE)) == 0 ) {
		l->state = !l->state;
		setLightState(l);
		changed = true;
	} else {
		printf("Unkown subtopic %s ... ignoring!\n", subtopic);
		return;
	}
	saveState(&runstate);

	if (changed) {
		updateMqttStatus(l);
	}

	free(topic);
	free(data);
}

bool updateMqttStatus(Light* light) {
	if (!mqtt_connected) {
		return false;
	}

	char* topic = malloc(64);
	char* msg = malloc(100);
	memset(topic, 0, 64);
	memset(msg, 0, 100);

	//send light state
	sprintf(topic, "%s/%s/%s", MQTT_TOPIC_PREFIX, light->name, MQTT_TOPIC_STATE);
	if (light->state) {
		sprintf(msg, "%s", MQTT_MESSAGE_ON);
	} else {
		sprintf(msg, "%s", MQTT_MESSAGE_OFF);
	}
	int ret = esp_mqtt_client_publish(client, topic, msg, strlen(msg), 1, 1);
	if (ret == 0){
		return false;
	}

	if (!light->update_only_state) {
		//don't retain messages from these topics
		//send light pulse_length
		memset(topic, 0, 64);
		memset(msg, 0, 100);
		sprintf(topic, "%s/%s/%s", MQTT_TOPIC_PREFIX, light->name, MQTT_TOPIC_PULSELEN);
		sprintf(msg, "%d", light->pulse_length);
		esp_mqtt_client_publish(client, topic, msg, strlen(msg), 1, 0);

		//send light code
		memset(topic, 0, 64);
		memset(msg, 0, 100);
		sprintf(topic, "%s/%s/%s", MQTT_TOPIC_PREFIX, light->name, MQTT_TOPIC_CODE);
		sprintf(msg, "%d", light->code);
		esp_mqtt_client_publish(client, topic, msg, strlen(msg), 1, 0);

		//send light offset
		memset(topic, 0, 64);
		memset(msg, 0, 100);
		sprintf(topic, "%s/%s/%s", MQTT_TOPIC_PREFIX, light->name, MQTT_TOPIC_OFFSET);
		sprintf(msg, "%d", light->off_set);
		esp_mqtt_client_publish(client, topic, msg, strlen(msg), 1, 0);

		//send light protocol num
		memset(topic, 0, 64);
		memset(msg, 0, 100);
		sprintf(topic, "%s/%s/%s", MQTT_TOPIC_PREFIX, light->name, MQTT_TOPIC_PROTOCOL);
		sprintf(msg, "%d", light->protocol_num);
		esp_mqtt_client_publish(client, topic, msg, strlen(msg), 1, 0);
	}

	free(topic);
	free(msg);
	return true;
}

#endif 

