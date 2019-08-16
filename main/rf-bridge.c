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

    //setup hardware
    initReceive(RX_433MHZ_CHANNEL, RX_433MHZ_PIN);
    initTransmit(TX_433MHZ_CHANNEL, TX_433MHZ_PIN);

    //Load saved state


    //setup wifi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_init_sta();

   	//Start tasks
   	xTaskCreate(RecieverDecoderTask, "433mhz_reciever", 2048, NULL, 10, NULL);
}
