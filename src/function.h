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


typedef struct rtcStore{
  int count;
};

typedef struct Sta{
  String ssid;
  String password;
  String severname;

};

/*HTML Страница веб сервера*/
String index_html = R"rawliteral(                                          
<!DOCTYPE html>
<html>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {
  font-family: Arial;
}

input[type=text], select {
  width: 100%;
  padding: 12px 20px;
  margin: 8px 0;
  display: block;
  border: 1px solid #ccc;
  border-radius: 4px;
  box-sizing: border-box;
}

input[type=password], select {
  width: 100%;
  padding: 12px 20px;
  margin: 8px 0;
  display: block;
  border: 1px solid #ccc;
  border-radius: 4px;
  box-sizing: border-box;
}

.btn {
  border: none; /* Remove borders */
  color: white; /* Add a text color */
  padding: 14px 28px; /* Add some padding */
  cursor: pointer; /* Add a pointer cursor on mouse-over */
}

input[type=submit] {
  width: 100%;
  background-color: #04AA6D;
  color: white;
  padding: 14px 20px;
  margin: 8px 0;
  border: none;
  border-radius: 4px;
  cursor: pointer;
}

.info {background-color: #2196F3;}

input[type=submit]:hover {
  background-color: #45a049;
}

div.container {
  width:100%;
  max-width: 1200px;
  margin: 0 auto;
}
.esp-action{
  border-radius: 5px;
  background-color: #f2f2f2;
  padding: 20px;
  margin-top: 20px;
}
</style>
<body>

<div class="container">
  <form class="esp-action" action="/get">
    <label for="ssid">SSID</label>
    <select id="ssid" name="ssid">)rawliteral";

/*------------------------------------------------------------------------------------------*/
String index_html_footer=R"rawliteral(</select>
            <label for="password">Password</label>
            <input type="password" id="password" name="password">
            <label for="server">Server</label>
            <input type="text" id="server" name="server">
            <label for="mac-address">MAC Address</label>
            <input type="text" disabled="disabled" value=")rawliteral" + WiFi.macAddress() +"\">" 
            R"rawliteral(
            <input type="submit" value="Submit">
          </form>
          
        </div>
      </body>
    </html>)rawliteral";
/*---------------------------------------------------------------------------------------*/

#endif