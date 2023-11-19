#ifndef ESP8266_H
#define ESP8266_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <IPAddress.h>
#include <WiFiUdp.h>
#include <lwip/etharp.h>

#include "settings.h"

extern WiFiUDP Udp;

int init_wifi();
void wifi_pause();
const char *wifi_get_mac();
int init_server();
int recv_datagram(uint8_t *data, int len);
eth_addr *get_sta_mac(uint32_t ip);

#endif
