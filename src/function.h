#ifndef FUNCTION_H
#define FUNCTION_H

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebSerial.h>
#include "LittleFS.h"
#include "Adafruit_Si7021.h"

#include <SPI.h>
#include <Wire.h>
extern "C" {
#include "user_interface.h"
}
#define RTCMEMORYSTART 65
#define MAXHOUR 6e6 
#define TIME_TO_SLEEP 4*3600e6


typedef struct {
  int count;
} rtcStore;


#endif