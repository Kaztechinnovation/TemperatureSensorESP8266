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
#include <map>
extern "C" {
#include "user_interface.h"
}

#define RTCMEMORYSTART 65
#define HOUR 3600e6
#define TIME_TO_SLEEP 3*HOUR
#define SLEEP_TEST 30e6
String GAS_ID = "AKfycbwiLFI4DcGVUuKL36uXKEPJEj7FaPITiQgycWYqSHLGw7LVxKOHBaURYu_V6YhA_V0g";
std::map<String,String> sheet = {
  {"48551912a0a3","AKfycbxTDlPaogg28u0REqVYvXFusqCaTjEsKevbXsfkZm4-WWmPvAWOYNhFVF36wUZKd1pw"},
  {"485519120d71","AKfycbwZo9d-g2DYK-vJIVQ5cJ4afKKq9zxHyJuPhamR9QlFMO6RjOAERmROqzY5FVTP_nFP"},
  {"485519124463","AKfycbzcdeJRcFNwq6Yx-LrKF5Fg1iJepiZ78R8WqWXey-ZzOhdKVA37APj8LTE3UZAryj9B"},
  {"48551912a547","AKfycbyIvUCgrZGKAvrbLXH0t-yTEYRVmIvgl2xzGX73HFOaYYc6UzzvCLSRwl4Pf2IMsXhazw"},
  {"485519123f62","AKfycbwyHlQapaK9DD6RQUPmksUlfFxGo3OW3BYp4MxF6gJOA_L6vtwbZdj_9xBB3z2IFAHgpg"},
  {"48551912465d","AKfycbyvd-FEEWXtJSgZPAEGRvC-Bx83rrKdGJ_NAHzB8tsM9-UdyOGjbaJttPwoAdYPY3-GeQ"},
  {"4855191286e7","AKfycbybW8pX-j7XU3-KfdH6MFkG77YoEAYK3-PD6vZxRS8l--5LeolgMMBlEC1rGDoutJoRhQ"},
  {"4855191284cf","AKfycbzFpLAtXVKjvuLJLAEGQPD-dwkOmvZBXL9yFoHYCrUPDuwiZ-zB2OAIPzrOwiW5w2Ce7w"},
  {"485519123f28","AKfycbz0i7HtnkYj1Hj92GbuJy5tLpY_99Gf3gcBZjlVwpRurMlsfMkhHHav2dZVsDjBmXEfFQ"},
  {"e89f6d92b8e8","AKfycbz8-zvYSkishhBQlQZFBxTLNl_MVZM8xPycQFmlrRcSVkFX77n--wY_5DlGav9oA7rrmA"},
  {"98cdac1d7c3a","AKfycbwiLFI4DcGVUuKL36uXKEPJEj7FaPITiQgycWYqSHLGw7LVxKOHBaURYu_V6YhA_V0g"}
};



typedef struct {
  int count;
}rtcStore;

typedef struct {
  String ssid;
  String password;
  String severname;
}Sta;

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