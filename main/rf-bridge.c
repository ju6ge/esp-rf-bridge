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
#include "rf_433mhz.h"
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

   	//Start tasks
   	xTaskCreate(RecieverDecoderTask, "433mhz_reciever", 2048, NULL, 10, NULL);
}
