#include <Arduino.h>
#include <function.h>
#include <AsyncElegantOTA.h>
ADC_MODE (ADC_VCC); // считавает напряжение на питания в ацп

/*-----------Точка доступа------------------*/ 
String AP_SSID     = "Kaz-";                    // SSID для точки доступа
/*------------------------------------------*/

String macAddress=WiFi.macAddress();      // мак адрес для отправки на сервер
uint16_t count_of_connection=0;      // количество неудачных подключений вай-фай
String html_page;    
Sta sta,sta2;
Adafruit_Si7021 sensor = Adafruit_Si7021(); // готовый класс для датчика
AsyncWebServer server(80); // Веб Сервер в микроконтроллере с портом 80
WiFiClientSecure clientSecure;
File file_ssid,file_password,file_server,file_send; // переменные для работы файлов в флеш памяти
// Ошибка 404
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

/*Инициализация флеш памяти */

/*считывает данные с памяти */ 
void fs_read(){
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  file_ssid = LittleFS.open("/ssid.txt","r");
  file_password = LittleFS.open("/password.txt","r");
  file_server = LittleFS.open("/server.txt","r");
  if(!file_ssid && !file_password && !file_server){
    Serial.println("Failed to open file for reading");
    return;
  }
  if(file_ssid.available() && file_password.available() && file_server.available()) {
    sta.ssid=file_ssid.readString();
    sta.password=file_password.readString();
    sta.severname=file_server.readString();
    
  }
  sta2.ssid=sta.ssid;
  sta2.password=sta.password;
  sta2.severname=sta.severname;
  file_ssid.close();
  file_password.close();
  file_server.close();
  LittleFS.end();
}

/*записавает данные в память .txt формате*/
void fs_write(File file,String path,String data){
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  file = LittleFS.open(path,"w");
  if (!file) {
    Serial.println("There was an error opening the file for writing");
    return;
  }
  if(file.print(data)) Serial.println("File was written");
  else Serial.println("File write failed");
  file.close();
  LittleFS.end();
}

void fs_remove(){
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
 
  LittleFS.remove("/ssid.txt");
  LittleFS.remove("/password.txt");
  LittleFS.remove("/server.txt");
  sta.password.clear();
  sta.ssid.clear();
  sta.severname.clear();
  LittleFS.end();
}
/*--------------------------*/


void sleep(){
  Serial.println("SLEEP!");
  ESP.deepSleep(TIME_TO_SLEEP, WAKE_RF_DISABLED);
}


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
      yield();
      if(time_to_connection>=15){ time_to_connection=0; break;}// если время для подключение превысет 5000мс или подключиться к WIFI тогда выходит из цикла 
    }
    Serial.println(" ");
    if(WiFi.status()==WL_CONNECTED) { /*Если подключился к сети*/
      WebSerial.println("WiFi is succesfully connected to " + sta.ssid); /*выводит в Веб сериал монитор*/
    }
    else { /*В ином случае выводит в монитор что не подключился к сети*/
      WebSerial.println("WiFi was not connected to " + sta.ssid);
      count_of_connection++;
    }
  }

  else if (count_of_connection>=3 && WiFi.status() != WL_CONNECTED) {
    delay(90*1000);
    if(sta.ssid==sta2.ssid && sta.password==sta2.password && sta.severname==sta2.severname) sleep();
    else if (sta.ssid!=sta2.ssid || sta.password!=sta2.password || sta.severname!=sta2.severname)
    {
      count_of_connection=0;
      sta2.password=sta.password;
      sta2.severname=sta.severname;
      sta2.ssid=sta.ssid;
    }
  }
  
  
}
/*----------------------*/

/*---------Сканирирование WiFi сети-----------*/
void scan_wifi(){
  uint8_t n = WiFi.scanNetworks();          // количество WiFi сети
  String wifi_networks="";                 // список WiFi сети для вывода в сервер 
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
  html_page=index_html + wifi_networks + index_html_footer;   // веб страница
}
/*---------------------------------------------*/

/*Инициализация точки доступа*/
void AP_init(){
  for(uint8_t i=0;i<macAddress.length();i++) {
    if(macAddress.indexOf(':')==i) macAddress.remove(i,1);
  }
  AP_SSID+=macAddress;
  WiFi.softAP(AP_SSID); // создание точка доступа
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); // выводит ip-address для веб сервера
}


/*Инициализация веб сервера*/
void server_init(){
  scan_wifi();                     // Сканирование WiFi

  /*Инициализация сервера*/
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
    /*главная страница веб сервера*/
    request->send(200, "text/html",html_page); // отправляет в сервер html code чтобы отобразить страницу
  });
  
  /*--------------Отправляет GET запрос в сервер----------------------*/ 
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) { 
    if(WiFi.status()==WL_CONNECTED) WiFi.disconnect();
    /*получение данные с сервера*/
    sta.ssid=request->getParam("ssid")->value();          
    sta.password=request->getParam("password")->value();
    sta.severname="http://"+request->getParam("server")->value();
    fs_write(file_password,"/password.txt",sta.password); /*запись подключенной wifi сети в память*/
    fs_write(file_ssid,"/ssid.txt",sta.ssid);
    fs_write(file_server,"/server.txt",sta.severname);
    /*-----------------------------------------------*/
    request->redirect("/webserial"); // перенаправление локальный WEB Serial monitor 
  });

  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
    scan_wifi();
    request->redirect("/"); 
  });

  server.on("/r", HTTP_GET, [](AsyncWebServerRequest *request){
    fs_remove();
    request->send(200,"text/plain","All files removed"); 
  });
  server.onNotFound(notFound);
  server.begin();
  /*------------------------------------------------------------------------------------*/
}
/*----------------------------------------*/

/*----------Oтправкa данных в Google Sheets---------------------*/
void sendDataToGoogleSheets(String tem, String hum, uint batLevel) {
  const int httpsPort = 443;
  const char* host = "script.google.com";
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
  String url = "/macros/s/" + sheet[macAddress] + "/exec?temperature=" + tem + 
                "&humidity=" + hum + "&mac_address=" + WiFi.macAddress() + "&bat_level=" + String(batLevel);
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


void BootMode(){
  WiFiClient client;  
  HTTPClient http;
  String payload;

  http.begin(client, sta.severname + "/get.php?mac_address=" + macAddress); 
  int httpResponseCode = http.GET();
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    Serial.println(payload);
  }
  else {
    Serial.print("Error code: ");
    WebSerial.print("Error code: ");
  }
  http.end();
  if(payload=="1") {
    WebSerial.println("IN BOOT");
    Serial.println("IN BOOT");
    delay(90e3);
  }
}

void sendDataToServer(String tem, String hum, uint8_t batLevel){
    WiFiClient client;  
    HTTPClient http;
    http.begin(client, sta.severname+"/post.php");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String httpRequestData = "temperature=" + tem + "&humidity=" + hum + "&mac_address=" + macAddress + "&batLevel=" + String(batLevel);

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
    Serial.println(httpResponseCode);
    WebSerial.println("I'm going into deep sleep mode");
    Serial.println("I'm going into deep sleep mode");
    http.end();
}

void _init_(){
  fs_read();
  WiFi.mode(WIFI_AP_STA);                 // Включение точки доступа и WiFi
  AP_init();
  server_init();                      // Включение точки доступа и сервера
  sensor.begin();
  clientSecure.setInsecure(); 
  WebSerial.begin(&server);            // Подключение web serial monitor к серверу
  AsyncElegantOTA.begin(&server);     // Сервер для прошивки по воздуху
  WebSerial.println("Device is started to send data");
  delay(1000);
  macAddress.toLowerCase();
}

void start(){
  if(WiFi.status()!=WL_CONNECTED) wifi_init(); // если Wifi не подключен будет инициализировать пока не подключется
  else{
    BootMode();
    float temp=0.0;
    float humi=0.0;
    for (int i=0;i<100;i++){
      temp+=sensor.readTemperature();
      delay(11);
    }
    for (int i=0;i<100;i++){
      humi+=sensor.readHumidity();
      delay(12);
    }
    String tem=String(temp/100.0);
    String hum=String(humi/100.0);
    uint batLevel=map(ESP.getVcc(),2309,3130,0,100);
    sendDataToGoogleSheets(tem,hum,batLevel);
    sendDataToServer(tem,hum,batLevel);
    sleep();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  _init_();
  Serial.println(sheet[macAddress]);
}

void loop() {
  start();
}

