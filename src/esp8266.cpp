#include "esp8266.h"

#include "util.h"

WiFiUDP Udp;
char mac[7];

int init_wifi() {
  delay(1);

  WiFi.persistent(false);
  WiFi.setOutputPower(20.5);

  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; i++) {
    serial_printf("found SSID: %s\r\n", WiFi.SSID((uint8_t)i).c_str());
  }

  unsigned long startTime = millis();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  DEBUGP("Connecting to wifi...");
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_TIMEOUT) {
    delay(500);
    DEBUGP("..%u..", WiFi.status());
  }

  if (WiFi.status() == WL_CONNECTED) {
    DEBUGP("Connected, IP address: %s\r\n", WiFi.localIP().toString().c_str());
    return 1;

  } else {
    DEBUGP("Wifi connection timed out...\r\n");
    return -1;
  }
}

const char *wifi_get_mac() {
  strcpy(mac, WiFi.macAddress().c_str());
  return mac;
}

int init_server() { return Udp.begin(BROADCAST_PORT); }

int recv_datagram(uint8_t *data, int len) {
  int result = 0;
  int packet_len = Udp.parsePacket();

  // DEBUGP("parsePacket() = %u\r\n", packet_len);

  if (packet_len > 0 && packet_len <= len) {
    result = Udp.read(data, len);
  }

  return result;
}

eth_addr *get_sta_mac(uint32_t ip) {
  ip4_addr requestIP{ip};
  eth_addr *ret_eth_addr = nullptr;
  ip4_addr const *ret_ip_addr = nullptr;
  etharp_request(netif_default, &requestIP);
  etharp_find_addr(netif_default, &requestIP, &ret_eth_addr, &ret_ip_addr);
  return ret_eth_addr;
}
