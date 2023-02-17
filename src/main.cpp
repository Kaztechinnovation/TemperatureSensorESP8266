#include <Arduino.h>
#include <function.h>
#include <AsyncElegantOTA.h>
ADC_MODE (ADC_VCC); // считавает напряжение на питания в ацп

/*-----------Точка доступа------------------*/ 
String AP_SSID     = "ESP-";                    // SSID для точки доступа
const char* AP_PASSWORD = "123456789";         // Пароль для точки доступа
/*------------------------------------------*/

String macAddress=WiFi.macAddress();      // мак адрес для отправки на сервер
String wifi_networks;                 // список WiFi сети для вывода в сервер 
uint16_t count_of_connection=0;      // количество неудачных подключений вай-фай     
Sta sta;
rtcStore rtcMem;
Adafruit_Si7021 sensor = Adafruit_Si7021(); // готовый класс для датчика
AsyncWebServer server(80); // Веб Сервер в микроконтроллере с портом 80
WiFiClientSecure clientSecure;

void readFromRTCMemory() {
  system_rtc_mem_read(RTCMEMORYSTART, &rtcMem, sizeof(rtcMem));
  Serial.print("\nread count = ");
  Serial.println(rtcMem.count);
  yield();
}

void writeToRTCMemory() {
  if (rtcMem.count <= MAXHOUR) {
    rtcMem.count++;
  } else {
    rtcMem.count = 0;
  }

  system_rtc_mem_write(RTCMEMORYSTART, &rtcMem, 4);

  Serial.print("write count = ");
  Serial.println(rtcMem.count);
  yield();
}

// Ошибка 404
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void sleep(double time){
  if (rtcMem.count == 0) {
    Serial.println("Will wake up with radio!");
    ESP.deepSleep(time, WAKE_RFCAL);
  } 
  else {
    Serial.println("Will wake up without radio!");
    ESP.deepSleep(time, WAKE_RF_DISABLED);
  }
}

/*
  Инициализация флеш памяти
  если method="r" считывает данные с памяти 
  если method="w" записавает данные в память .txt формате 
*/
void LittleFS_init(const char* method){
  File file_ssid,file_password,file_server; // переменные для работы файлов в флеш памяти
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  file_ssid = LittleFS.open("/ssid.txt",method);
  file_password = LittleFS.open("/password.txt",method);
  file_server = LittleFS.open("/server.txt",method);
  if(method == "r"){
    if(!file_ssid && !file_password && !file_server){
        Serial.println("Failed to open file for reading");
        return;
    }
    if(file_ssid.available() && file_password.available() && file_server.available()) {
      sta.ssid=file_ssid.readString();
      sta.password=file_password.readString();
      sta.severname=file_server.readString();
    }
  }
  else if(method == "w"){
    if (!file_ssid && !file_password && !file_server) {
      Serial.println("There was an error opening the file for writing");
      return;
    }
    if (file_ssid.print(sta.ssid) && file_password.print(sta.password) && file_server.print(sta.severname))
        Serial.println("File was written");
    else 
        Serial.println("File write failed");
  }
  file_ssid.close();
  file_password.close();
  file_server.close();
  LittleFS.end();
}
/*--------------------------*/

/*Инициализация WiFi Сети*/
void wifi_init(){
  if(count_of_connection<3 && WiFi.status() != WL_CONNECTED){
    WiFi.begin(sta.ssid.c_str(), sta.password.c_str()); // c_str() метод преобразовает String to const char*
    WebSerial.println("Connecting to WIFI");
    Serial.println("Connecting to WIFI");
    uint16_t time_to_connection=0; // счетчик для подключение к WIFI
    while(WiFi.status() != WL_CONNECTED){ // Пока не подключется Wifi будет инициализация 
      WebSerial.print(".");
      Serial.print(".");
      delay(1000); // каждый цикл задерка 1с
      time_to_connection++; // и добавляет 1с
      if(time_to_connection>=15){ time_to_connection=0; break;}// если время для подключение превысет 5000мс или подключиться к WIFI тогда выходит из цикла 
    }
    Serial.println(" ");
    if(WiFi.status()==WL_CONNECTED) { /*Если подключился к сети*/
      WebSerial.println("WiFi is succesfully connected to " + sta.ssid); /*выводит в Веб сериал монитор*/
      LittleFS_init("w"); /*запись подключенной wifi сети в память*/
    }
    else { /*В ином случае выводит в монитор что не подключился к сети*/
      WebSerial.println("WiFi was not connected to " + sta.ssid);
      count_of_connection++;
    }
  }

  else if (count_of_connection>=3 && WiFi.status() != WL_CONNECTED) sleep(TIME_TO_SLEEP);
  
  
}
/*----------------------*/
/*---------Сканирирование WiFi сети-----------*/
void scan_wifi(){
  uint8_t n = WiFi.scanNetworks(); // количество WiFi сети
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
  for(uint8_t i=0;i<macAddress.length();i++) {
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
    sta.ssid=request->getParam("ssid")->value();          
    sta.password=request->getParam("password")->value();
    sta.severname="http://"+request->getParam("server")->value()+"/post-data.php";
    /*-----------------------------------------------*/
    request->redirect("/webserial"); // перенаправление локальный WEB Serial monitor 
  });
  server.onNotFound(notFound);
  server.begin();
  /*------------------------------------------------------------------------------------*/
}
/*----------------------------------------*/

/*----------Oтправкa данных в Google Sheets---------------------*/
void sendDataToGoogleSheets(String tem, String hum, uint8_t batLevel) {
  const int httpsPort = 443;
  const char* host = "script.google.com";
  String GAS_ID = "AKfycbwz3xRklsgg8l_LB313YyaRlBlycnjCRGala5ZfCEz9bGhm0RxuDfWPdQX7JzA2CrXh"; //--> ID скрипта электронной таблицы
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
                "&humidity=" + hum + "&mac_address=" + macAddress + "&bat_level=" + String(batLevel);
  /*-----------------------------*/

  clientSecure.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  WebSerial.println("request sent");
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

void SendToServer(String tem, String hum, uint8_t batLevel){
  if(!sta.severname.isEmpty()){ // если подключился и данные сервера не пустой
    WiFiClient client;  
    HTTPClient http;
    http.begin(client, sta.severname);  // Подключенный wifi клиент и api server
    
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
    WebSerial.println(httpResponseCode);
    WebSerial.println("I'm going into deep sleep mode");
    http.end();
  }
}

void _init_(){
  readFromRTCMemory();
  writeToRTCMemory();
  WiFi.mode(WIFI_AP_STA);              // Включение точки доступа и WiFi
  AP_server_init();                   // Включение точки доступа и сервера
  LittleFS_init("r");                // Cчитывает данные с памяти
  // if (!sensor.begin()) while (true);
  clientSecure.setInsecure(); 
  WebSerial.begin(&server);            // Подключение web serial monitor к серверу
  AsyncElegantOTA.begin(&server);     // Сервер для прошивки по воздуху
  if(rtcMem.count==0) delay(90*1000);
  WebSerial.println("Device is started to send data");
}

void start(){
  if(WiFi.status()!=WL_CONNECTED) wifi_init(); // если Wifi не подключен будет инициализировать пока не подключется
  else{
    String tem=String(sensor.readTemperature());
    String hum=String(sensor.readHumidity());
    uint8_t batLevel=map(ESP.getVcc(),2309,3336,0,100);
    sendDataToGoogleSheets(tem,hum,batLevel);
    SendToServer(tem,hum,batLevel);
    sleep(TIME_TO_SLEEP);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  _init_();
}

void loop() {
  start();
}

