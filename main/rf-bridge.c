/*
 * A dual core application for decoding 433Mhz signals and keeping mqtt in sync to signals triggering rf sockets.
 * Can also send 433Mhz signals to trigger sockets via mqtt
 *
 * author: Felix Richter <judge@felixrichter.tech>
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "tasks.h"
#include "wifi.h"
#include "rf_433mhz.h"
#include "mqtt.h"
#include "state.h"

#define TX_433MHZ_PIN 18
#define TX_433MHZ_CHANNEL 0

#define RX_433MHZ_PIN 19
#define RX_433MHZ_CHANNEL 1


void app_main()
{
	printf("Application code starts now!\n");
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	wifi_init_sta();

	//setup hardwar
	initReceive(RX_433MHZ_CHANNEL, RX_433MHZ_PIN);
	initTransmit(TX_433MHZ_CHANNEL, TX_433MHZ_PIN);


	//mount filesystem
	esp_vfs_spiffs_conf_t fs_conf = {
		.base_path = "/spiffs",
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = false
	};

	// Use settings defined above to initialize and mount SPIFFS filesystem.
	ret = esp_vfs_spiffs_register(&fs_conf);
	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return;
	}

	//Load saved state
	readState(&runstate);
	for ( int i=0; i < runstate.lightcount; i++){
		printf("Light: %s\n\tpulse:\t%d\n\tstate:\t%d\n\tcode:\t%d\n\toffset:\t%d\n\tproto:\t%d\n", runstate.lights[i].name, runstate.lights[i].pulse_length, runstate.lights[i].state, runstate.lights[i].code, runstate.lights[i].off_set, runstate.lights[i].protocol_num);
	}

	//initialize cached storade for wifi and mqtt config 

	//setup conncetion
	while(!wifi_connected) {}
	mqtt_start();

   	//Start tasks
   	xTaskCreate(RecieverDecoderTask, "433mhz_reciever", 4096, NULL, 10, NULL);
}
