#include <Arduino.h>
#include <function.h>

void setup() {
  Serial.begin(115200);
  _init_();
}

void loop() {
  WebSerial.println("Device is started to send data");
  start();
}

