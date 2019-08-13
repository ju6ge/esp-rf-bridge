#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "esp_system.h"

typedef struct {
	uint16_t pulse_length;
	bool state;
	uint32_t code;
	uint8_t off_set;
	uint8_t protocol_num;
	char* name;
	bool update_only_state;
} Light;

typedef struct {
	Light* lights;
	uint8_t lightcount;
} RuntimeState;

#define READBUFFERLEN 10

void saveState(RuntimeState* state);
bool readState(RuntimeState* state);

bool addLight(char* data, RuntimeState* state);
bool readLightFromStr(char* data, Light* light);

#endif 
