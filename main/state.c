#include "state.h"
#include "rf_433mhz.h"

RuntimeState runstate;

void saveState(RuntimeState* state) {
	//printf("Function saveState\n");
	FILE *fd = fopen("/spiffs/lights.conf", "w");
	if (fd == NULL) {
		return;
	}
	for (int i=0;i<state->lightcount;i++) {
		fprintf(fd, "%s:%d:%d:%ld:%d:%d\n", state->lights[i].name, state->lights[i].pulse_length, state->lights[i].state, state->lights[i].code, state->lights[i].off_set, state->lights[i].protocol_num);
}
	fclose(fd);
}

bool readState(RuntimeState* state) {
	//printf("Function readState \n");
	FILE *fd = fopen("/spiffs/lights.conf", "r");
	if (fd == NULL) {
		return false;
	}
	state->lightcount = 0;
	state->lights = malloc(READBUFFERLEN * sizeof(Light));
	unsigned int readlen;
	char* line = NULL;

	while(__getline(&line, &readlen, fd) != -1) {
		addLight(line, state);
	}

	fclose(fd);
	return true;
}

bool readLightFromStr(char* data, Light* light) {
	//printf("Function read_light_from_str\n");
	if (strlen(data) <= 1) {
		return false;
	}
	memset(light, 0, sizeof(Light));
	light->update_only_state = true;
	//Read light name
	char* token = strsep(&data, ":");
	light->name = malloc(strlen(data));
	memset(light->name, 0, strlen(data));
	sprintf(light->name, "%s", token);
	//Read light pulse_length 
	token = strsep(&data, ":");
	light->pulse_length = atoi(token);
	//Read light state
	token = strsep(&data, ":");
	if ( atoi(token) == 0 ) {
		light->state = false;
	} else {
		light->state = true;
	}
	//Read light code to turn light on
	token = strsep(&data, ":");
	light->code = atoi(token);
	//Read light code offset to turn light off
	token = strsep(&data, ":");
	light->off_set = atoi(token);
	//Read Protocol Number
	token = strsep(&data, ":");
	light->protocol_num = atoi(token);
	return true;
};

bool addLight(char* data, RuntimeState* state) {
	uint16_t i = floor(state->lightcount/READBUFFERLEN);

	//check if more lights where read then the buffer has space for
	//if not make buffer bigger;

	if ( state->lightcount >= (i+1)*READBUFFERLEN) {
		Light* old = state->lights;
		state->lights = malloc( (i+2) * READBUFFERLEN * sizeof(Light));
		for (int j=0;j<state->lightcount;j++){
			state->lights[j] = old[j];
		}
		free(old);
	}

	if (readLightFromStr(data, &state->lights[state->lightcount])) {
		state->lightcount++;
		return true;
	}
	return false;
}

void setLightState(Light* l) {
	Message433mhz msgtosend;
	msgtosend.code_lenght=24;
	msgtosend.repeat=4;
	msgtosend.pulse_length = l->pulse_length;
	msgtosend.protocol = &protocols_433mhz[l->protocol_num];
	if  (l->state) {
		msgtosend.data = l->code;
	} else {
		msgtosend.data = l->code + l->off_set;
	}
	sendMessage(&msgtosend);
}
