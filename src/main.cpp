#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WebSerial.h>
#include "FS.h"
#include "Adafruit_Si7021.h"
#include <AsyncElegantOTA.h>
#include <SPI.h>
#include <Wire.h>

#define SEC 1e6
#define HOUR 3600*SEC
ADC_MODE (ADC_VCC);
AsyncWebServer server(80); // Веб Сервер в микроконтроллере с портом 80

// Точка доступа
String AP_SSID     = "ESP-";      // SSID для точки доступа
const char* AP_PASSWORD = "123456789";         // Пароль для точки доступа

String STA_SSID;      // SSID для подключение WIFI
String STA_PASSWORD;  // Пароль для подключение WIFI
String macAddress=WiFi.macAddress(); // мак адрес для отправки на сервер

uint16_t time_to_connection=0; // счетчик для подключение к WIFI
File file_ssid,file_password,file_server; // переменные для работы файлов в флеш памяти
float temperature,humidity; // переменные для датчика(температкра, влажность)

Adafruit_Si7021 sensor = Adafruit_Si7021(); // готовый класс для датчика

String serverName; //API of server
uint8_t n; // количество WiFi сети
String wifi_networks; // список WiFi сети для вывода в сервер 
uint8_t batLevel=0;
const int httpsPort = 443;
const char* host = "script.google.com";
//----------------------------------------
WiFiClientSecure clientSecure;
String GAS_ID = "AKfycbxVNZjWjg5f95EV3Jegevxp-JezpBEtgs3Bnc3LsibRm-esxeQlRHJb60BidOObrRzosQ"; //--> ID скрипта электронной таблицы
uint16_t count_of_connection=0;


/*<----------Oтправкa данных в Google Sheets--------------------->*/
void sendData(String tem, String hum) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  /*Подключение к хосту Google*/
  if (!clientSecure.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
 /*---------------------------------*/
  /*Ссылка на APP Script*/
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + tem + 
                "&humidity=" + hum+ "&mac_address=" + macAddress;
  /*-----------------------------*/

  clientSecure.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  //----------------------------------------

  /*Проверка того, успешно были отправлены данные*/
  while (clientSecure.connected()) {
    String line = clientSecure.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  /*----------------------------------------*/
} 
/*<------------------------------------------------------------------>*/



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
}
</style>
<body>

<div class="container">
<h3>KazTechInnovations</h3>
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

// Ошибка 404
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

/*Инициализация флеш памяти
  если method="r" считывает данные с памяти
  если method="w" записавает данные в память .txt формате 
*/
void SPIFFS_init(const char* method){
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  file_ssid = SPIFFS.open("/ssid.txt",method);
  file_password = SPIFFS.open("/password.txt",method);
  file_server = SPIFFS.open("/server.txt",method);
  if(method=="r"){
    if(!file_ssid && !file_password && !file_server){
        Serial.println("Failed to open file for reading");
        return;
    }
    if(file_ssid.available() && file_password.available() && file_server.available()) {
      STA_SSID=file_ssid.readString();
      STA_PASSWORD=file_password.readString();
      serverName=file_server.readString();
    }
  }
  else if(method=="w"){
    if (!file_ssid && !file_password && !file_server) {
      Serial.println("There was an error opening the file for writing");
      return;
    }
    if (file_ssid.print(STA_SSID) && file_password.print(STA_PASSWORD) && file_server.print(serverName))
        Serial.println("File was written");
    else 
        Serial.println("File write failed");
  }
  file_ssid.close();
  file_password.close();
  file_server.close();
  SPIFFS.end();
}
/*--------------------------*/

/*Инициализация WiFi Сети*/
void wifi_init(){
  if(count_of_connection<3 || WiFi.status() != WL_CONNECTED){
    WiFi.begin(STA_SSID.c_str(), STA_PASSWORD.c_str()); // c_str() метод преобразовает String to const char*
    Serial.println("Connecting to WIFI");
    WebSerial.println("Connecting to WIFI");
    while(WiFi.status() != WL_CONNECTED){ // Пока не подключется Wifi будет инициализация 
      WebSerial.print(".");
      Serial.print(".");
      delay(1000); // каждый цикл задерка 1с
      time_to_connection++; // и добавляет 1000мс
      if(time_to_connection>=10){ time_to_connection=0; break;}// если время для подключение превысет 5000мс или подключиться к WIFI тогда выходит из цикла 
    }
    Serial.println(" ");
    if(WiFi.status()==WL_CONNECTED) { /*Если подключился к сети*/
      WebSerial.println("WiFi is succesfully connected to " + STA_SSID); /*выводит в Веб сериал монитор*/
      Serial.println("WiFi is succesfully connected to " + STA_SSID);
      SPIFFS_init("w"); /*запись подключенной wifi сети в память*/
      count_of_connection=0;
    }
    else { /*В ином случае выводит в монитор что не подключился к сети*/
      WebSerial.println("WiFi was not connected to " + STA_SSID);
    }
    count_of_connection++;
  }
  if (count_of_connection>=3 && WiFi.status() != WL_CONNECTED){
    count_of_connection=0;
<<<<<<< HEAD
    ESP.deepSleep(20e6);
=======
    ESP.deepSleep(12*HOUR);
>>>>>>> 64a51cf2a478e5c583ba0d9ed72f0b2a9bfce48f
  }
  
}

/*---------Сканирирование WiFi сети-----------*/
void scan_wifi(){
  n = WiFi.scanNetworks(); // количество WiFi сети
  wifi_networks.clear();
  if (n == 0) Serial.println("no networks found"); // выводит в монитор что wifi сети отсутствуют  
  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) { 
      /*Записавает в String html code список wifi сети*/
      wifi_networks += "<option value=\""+String(WiFi.SSID(i))+"\">"+String(WiFi.SSID(i))+"</option>";
      delay(10);
    }
  }
  index_html+=wifi_networks + index_html_footer;   // веб страница
}
/*---------------------------------------------*/

/*Инициализация точка доступа и веб сервера*/
void AP_server_init(){
  for(auto i=0;i<macAddress.length();i++) {
    if(macAddress.indexOf(':')==i) macAddress.remove(i,1);
  }
  AP_SSID+=macAddress;
  WiFi.softAP(AP_SSID); // создание точка доступа
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); // выводит ip-address для веб сервера
  scan_wifi();                     // Сканирование WiFi
  
  /*Инициализация сервера*/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ /*главная страница веб сервера*/
    request->send(200, "text/html",index_html); // отправляет в сервер html code чтобы отобразить страницу
  });
  
  /*--------------Отправляет GET запрос в сервер----------------------*/ 
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) { 
    if(WiFi.status()==WL_CONNECTED) WiFi.disconnect();
    /*получение данные с сервера*/
    STA_SSID=request->getParam("ssid")->value();          
    STA_PASSWORD=request->getParam("password")->value();
    serverName="http://"+request->getParam("server")->value()+"/post-data.php";
    /*-----------------------------------------------*/
    request->redirect("/webserial"); // перенаправление локальный WEB Serial monitor 
  });
  AsyncElegantOTA.begin(&server);
  server.onNotFound(notFound);
  server.begin();
  /*------------------------------------------------------------------------------------*/
}

void start(){
  String tem=String(sensor.readTemperature());
  String hum=String(sensor.readHumidity());
  batLevel=map(ESP.getVcc(),2309,3466,0,100);
  if(!STA_SSID.isEmpty() && WiFi.status()!=WL_CONNECTED) wifi_init(); // если Wifi не подключен будет инициализировать пока не подключется
  else if (WiFi.status()==WL_CONNECTED ){
    sendData(tem,hum);     // если подключился и данные сервера не пустой
    Serial.print(WiFi.localIP());
    WebSerial.println(Serial.readString());
    if(!serverName.isEmpty()){
      WiFiClient client;  
      HTTPClient http;
      http.begin(client, serverName);  // Подключенный wifi клиент и api server
      
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String httpRequestData = "&temperature=" + tem + "&humidity=" + hum + "&mac_address=" + macAddress + "&batLevel=" + String(batLevel);

      int httpResponseCode = http.POST(httpRequestData);
      /*--------------------------------------------------------------------------------------------------*/
      
      /*httpResponseCode==200 то отправился в сервер без ошибки*/
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        WebSerial.print("HTTP Response code: ");
      }
      else {
        Serial.print("Error code: ");
        WebSerial.print("Error code: ");
      }
      /*----------------------------------------------------*/
      Serial.println(httpResponseCode);
      WebSerial.println(httpResponseCode);
      http.end();
      Serial.println("I'm awake, but I'm going into deep sleep mode for 30 seconds");
      WebSerial.println("I'm awake, but I'm going into deep sleep mode for 30 seconds");
<<<<<<< HEAD
      ESP.deepSleep(20e6);
=======
      ESP.deepSleep(12*HOUR);
>>>>>>> 64a51cf2a478e5c583ba0d9ed72f0b2a9bfce48f
    }
  }
}

void setup() {
  Serial.begin(115200);
  if (!sensor.begin()) while (true);
  WiFi.mode(WIFI_AP_STA);             // Включение точки доступа и WiFi
  count_of_connection=0;
  AP_server_init();                   // Включение точки доступа и сервера
  WebSerial.begin(&server);           // Подключение web serial monitor к серверу
  SPIFFS_init("r");
  clientSecure.setInsecure();
  Serial.println(macAddress);
}

void loop() {
  start();
}

