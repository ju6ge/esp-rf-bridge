# Esp 32 RF Bridge

This project implements a simple bridge between MQTT and RF Sockets, to be able to trigger lights in a room. This is bidirectional, when triggering a light via a remote the MQTT status will be updated and vice versa.

It uses the Hardware RC Interface to recieve and transmit rf signals.
