#include <Arduino.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "esp8266.h"
#include "settings.h"
#include "util.h"
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define LIMON_VOLTAGE_FRAME 0x4ff

HardwareSerial console = Serial;

U8G2_SSD1306_128X64_NONAME_F_SW_I2C
u8g2(U8G2_R0, /* clock=*/D6, /* data=*/D5,
     /* reset=*/U8X8_PIN_NONE);

#define U8LOG_WIDTH 25
#define U8LOG_HEIGHT 6
uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];
U8G2LOG u8g2log;

unsigned long lastReport = 0;

void draw_nowifi();

void setup() {
  console.begin(9600);
  console.setDebugOutput(DEBUG_ESP);
  u8g2.begin();

  serial_printf("LiMon Display initializing...\r\n");
  serial_printf("LiMon Display MAC: %s\r\n", wifi_get_mac());

  while (!(init_wifi() > 0)) {
    draw_nowifi();
    serial_printf("Wifi initialization error\r\n");
    delay(10000);
  }

  serial_printf("Wifi initialized\r\n");

  if (init_server() > 0) {
    DEBUGP("UDP server started\r\n");
  }

  u8g2log.begin(U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  u8g2log.setLineHeightOffset(0);
  u8g2log.setRedrawMode(0);
  u8g2log.print("\f");

  serial_printf("LiMon Display initialized...\r\n");
}

void draw_nowifi() {
  u8g2.firstPage();

  do {
    u8g2.setFont(u8g2_font_9x15_tr);
    u8g2.setCursor(0, 36);
    u8g2.printf("    No WiFi");
  } while (u8g2.nextPage());
}

void loop() {
  uint8_t dlen;
  IPAddress prevSource;

  struct {
    unsigned long id;
    uint8_t can_frame[4];
  } datagram;

  while ((dlen = recv_datagram((uint8_t *)&datagram, sizeof(datagram))) > 0) {
    DEBUGP("Received UDP datagram from %s with size %hhu\r\n",
           Udp.remoteIP().toString().c_str(), dlen);

    eth_addr *mac = get_sta_mac(Udp.remoteIP().v4());

    serial_printf("mac is %u\r\n", (uint32_t *)mac);

    if (datagram.id == LIMON_VOLTAGE_FRAME) {
      // some trivial dedupping...
      if (prevSource != Udp.remoteIP() || lastReport < millis() - 60000) {
        uint16_t volts = *((uint16_t *)&datagram.can_frame);

        if (mac) {
          u8g2log.printf("%hhX%hhX%hhX%hhX%hhX%hhX volts: %2.2f\n",
                         mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3],
                         mac->addr[4], mac->addr[5], volts / 1000.0);
        } else {
          u8g2log.printf("%s volts: %2.2f\n", Udp.remoteIP().toString().c_str(),
                         volts / 1000.0);
        }

        prevSource = Udp.remoteIP();
      }

      lastReport = millis();
    } else {
      DEBUGP("Unknown frame type %x\r\n", datagram.id);
    }
  }

  u8g2.firstPage();

  do {
    u8g2.setFont(u8g2_font_6x13_tr);
    u8g2.setCursor(0, 13);
    u8g2.printf("Last report %luh ago:", (millis() - lastReport) / 3600000);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawLog(0, 23, u8g2log);
  } while (u8g2.nextPage());

  DEBUGP("*");

  delay(1000);
}
