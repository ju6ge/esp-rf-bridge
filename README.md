# Esp 32 RF Bridge

This project implements a simple bridge between MQTT and RF Sockets, to be able to trigger
lights in a room. This is bidirectional, when triggering a light via a remote the MQTT status
will be updated and vice versa.

It uses the Hardware RC Interface to recieve and transmit rf signals. I use it to trigger lights
in my appartment.


# Usage

1. Connect a 433mhz transimitter to the esp32 pin 18 and the reciever to pin 19

2. Use the ``make menuconfig`` to set the wifi credentials as well as the mqtt settings.

3. Setup the recongnized signals in the spiffs_image/lights.conf:

4. Build and Flash to esp32


# Signal configuration

Every signal that we want to map from/to mqtt is defined in a seperate line of the file. The
parameters are seperated by ":".

NAME:PULSE_LEN:INITIAL_STATE:ON_CODE:OFFSET_TO_OFF:PROTOCOL

# MQTT

Every signal defined in the configuration is mapped to the `lights/NAME` topic on the mqtt broker:

You can see it's current state in `lights/NAME/state`. And you can set the state by writing a
0 or 1 to `lights/NAME/set_state` or toggle the state by writing to `lights/NAME/toggle`.

