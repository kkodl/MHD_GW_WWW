/******************************************************************************************************************
 * 
  MHD_GW - get Prague Public transporrt departure schedules on your Zivy Obraz
  Source data on Prague public transport kindly published by Golemio (https://api.golemio.cz/pid/docs/openapi/)
  "Zivy Obraz" - push your favorite content on almost any e-paper easily and conveniently (https://zivyobraz.eu/)
  
  I used following libraries and/or copied parts of code from these library examples: 
   * WiFi manager by tzapu https://github.com/tzapu/WiFiManager
   * ArduinoJson - https://arduinojson.org Copyright © 2014-2024, Benoit BLANCHON
   * ESP Async Web Server https://github.com/me-no-dev/ESPAsyncWebServer/tree/master
   * ESP Little FS by Brian Pugh
   * Rui Santos & Sara Santos - Random Nerd Tutorials https://RandomNerdTutorials.com/esp32-esp8266-input-data-html-form/

   * Compile with board type: ESP32 dev module

 Use at your own risk :) & enjoy!  
 
 (2024) Karel Kotrba, https://github.com/kkodl

*********/

#include <Arduino.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <LittleFS.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <Hash.h>
  #include <LittleFS.h>
#endif

#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif

#include "html.h"

AsyncWebServer server(80);

char json[7000];                  // buffer response from golemio into char array
String payload;                   //store for the golemio direct response

  // Inside the brackets, 7000 is the capacity of the memory pool in bytes.
  // Don't forget to change this value to match your JSON document.
  // Use arduinojson.org/v6/assistant to compute the capacity.

StaticJsonDocument<8192> jsonBuffer;    // place for deserialization and data extraction
WiFiClient wifi;

int zastavka_index;
int train_count;          // how many vehicles into the future we are asking i.e. 4 = next four vehicles leaving
int Stop_Number;          // our own index of stop i.e. if we ask for data from 3 stops, we must somehow label the values in Zivy Obraz
                          // so this is label index for stops - see your Zivy Obraz "values" after first succesful run
String Number_Of_Trips;   // in http querry it has to be as a string
String Stop_Ids;          // PID stop code. To find your stop, point to http://data.pid.cz/stops/xml/StopsByName.xml
String Walk_Time;         // Time to walk to stop
String Name;              // string to store name of value for Zivy Obraz values
long HttpGetStartMillis;  // timer
int Interval;             // querry interval storage

// labels for HTML
const char* PARAM_GOLEMTOKEN = "GolemToken";
const char* PARAM_ZIVYOBRAZTOKEN = "ZivyObrazToken";
const char* PARAM_STOP1NAME = "Stop1Name";
const char* PARAM_STOP1TRAINS = "Stop1Trains";
const char* PARAM_STOP1WALK = "Stop1Walk";
const char* PARAM_STOP2NAME = "Stop2Name";
const char* PARAM_STOP2TRAINS = "Stop2Trains";
const char* PARAM_STOP2WALK = "Stop2Walk";
const char* PARAM_STOP3NAME = "Stop3Name";
const char* PARAM_STOP3TRAINS = "Stop3Trains";
const char* PARAM_STOP3WALK = "Stop3Walk";
const char* PARAM_STOP4NAME = "Stop4Name";
const char* PARAM_STOP4TRAINS = "Stop4Trains";
const char* PARAM_STOP4WALK = "Stop4Walk";
const char* PARAM_GOFLAG = "GoFlag";
const char* PARAM_TXFLAG = "TxFlag";
const char* PARAM_REPINTERVAL = "Report";

/************************* Server URL and tokens ******************************************/
String Auth_Token;                                                              // Golemio token placeholder
String ZO_Server_Name = "http://in.zivyobraz.eu/?import_key=";                  // server URL for "Zivy Obraz"
String Gol_Server_Name = "https://api.golemio.cz/v2/pid/departureboards?ids=";  // server URL for Golemio
String Server_Key;                                                              // Zivy Obraz key placeholder

/************************* Stops setup ****************************************************/

#define STOP_COUNT_TOTAL 4                // 4 stops in total, if you change this, a lot of other stuff needs to change in the code
      
char Stop_ID [STOP_COUNT_TOTAL] [10];      // char array to store stop codes
int Departure_Count[STOP_COUNT_TOTAL];    // int array to store number of departing trains
int Walk_Distance[STOP_COUNT_TOTAL];      // int array to store walk time per stop

String inputMessage;

//define your default values here, if there are different values in config hey are overwritten.

//default custom static IP in case of empty filesystem
String static_ip = "192.168.1.69";
String static_gw = "192.168.1.1";
String static_sn = "255.255.255.0";
String static_dn = "8.8.8.8";

// default: report interval=60s, gw is stopped, no TX to Zivy Obraz
String Reprt_Interval = "60";
String GoFlag = "stop";
String TxFlag = "no";


// ---------- SETUP -----------------------------------------------------------------------
//
void setup() {
  Serial.begin(115200);
  // Initialize LittleFS
  #ifdef ESP32
    if(!LittleFS.begin(true)){
      Serial.println("An Error has occurred while mounting LittleFS");
      return;
    }
  #else
    if(!LittleFS.begin()){
      Serial.println("An Error has occurred while mounting LittleFS");
      return;
    }
  #endif

    if(!LittleFS.begin(true)){
      Serial.println("An Error has occurred while mounting LittleFS");
      return;
    }
//In case of empty filesystem, load the defaults
    if (readFile(LittleFS, "/StaticIP.txt") != "") {static_ip = readFile(LittleFS, "/StaticIP.txt");}
    if (readFile(LittleFS, "/StaticGW.txt") != "") {static_gw = readFile(LittleFS, "/StaticGW.txt");}
    if (readFile(LittleFS, "/StaticSN.txt") != "") {static_sn = readFile(LittleFS, "/StaticSN.txt");}
    if (readFile(LittleFS, "/StaticDN.txt") != "") {static_dn = readFile(LittleFS, "/StaticDN.txt");}
    if (readFile(LittleFS, "/GoFlag.txt") != "") {GoFlag = readFile(LittleFS, "/GoFlag.txt");}
    if (readFile(LittleFS, "/TxFlag.txt") != "") {TxFlag = readFile(LittleFS, "/TxFlag.txt");}
    if (readFile(LittleFS, "/Report.txt") != "") {Reprt_Interval = readFile(LittleFS, "/Report.txt");}


// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Debug
/*
  Serial.println ("Loaded values:");  
  Serial.println(static_ip);
  Serial.println(static_gw);
  Serial.println(static_sn);
  Serial.println(static_dn);

  Serial.println(GoFlag);
  Serial.println(TxFlag);
  Serial.println(Reprt_Interval);
*/

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  IPAddress _ip,_gw,_sn,_dn;
  _ip.fromString(static_ip);
  _gw.fromString(static_gw);
  _sn.fromString(static_sn);
  _dn.fromString(static_dn);

  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn, _dn);

  //reset wifi manager settings - for testing<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(240);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("GW_AutoConnectAP", "heslicko123")) {
    Serial.println("failed to connect and hit timeout");
    delay(10000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(15000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("OK, connected... :)");

// Save the user IP to filesystem
writeFile(LittleFS, "/StaticIP.txt", (WiFi.localIP().toString()).c_str());
writeFile(LittleFS, "/StaticGW.txt", (WiFi.gatewayIP().toString()).c_str());
writeFile(LittleFS, "/StaticSN.txt", (WiFi.subnetMask().toString()).c_str());
writeFile(LittleFS, "/StaticDN.txt", (WiFi.dnsIP(0).toString()).c_str());
  

/*
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>Print it - for debug
  Serial.println("local from file>");
  Serial.println(readFile(LittleFS, "/StaticIP.txt"));
  Serial.println(readFile(LittleFS, "/StaticGW.txt"));
  Serial.println(readFile(LittleFS, "/StaticSN.txt"));
  Serial.println(readFile(LittleFS, "/StaticDN.txt"));

  Serial.print("GoFlag: ");
  Serial.println(readFile(LittleFS, "/GoFlag.txt"));
  Serial.print("TxFlag: ");
  Serial.println(readFile(LittleFS, "/TxFlag.txt"));
  Serial.print("ReportInterval: ");
  Serial.println(readFile(LittleFS, "/Report.txt"));
  
  Serial.println("local from adapter>");  
  Serial.print("Current ESP32 IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway (router) IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet Mask: " );
  Serial.println(WiFi.subnetMask());
  Serial.print("Primary DNS: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("Secondary DNS: ");
  Serial.println(WiFi.dnsIP(1));

  Serial.println(WiFi.status());
  Serial.println();
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/
  
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  HttpGetStartMillis = millis();
  Interval = readFile(LittleFS, "/Report.txt").toInt();
  server.begin();

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    //String inputMessage;
    if (request->hasParam(PARAM_GOLEMTOKEN)) {
      inputMessage = request->getParam(PARAM_GOLEMTOKEN)->value();
      writeFile(LittleFS, "/GolemToken.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_ZIVYOBRAZTOKEN)) {
      inputMessage = request->getParam(PARAM_ZIVYOBRAZTOKEN)->value();
      writeFile(LittleFS, "/ZivyObrazToken.txt", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(PARAM_STOP1NAME)) {
      inputMessage = request->getParam(PARAM_STOP1NAME)->value();
      writeFile(LittleFS, "/Stop1Name.txt", inputMessage.c_str());
    }
    // GET inputBool value on <ESP_IP>/get?inputBool=<inputMessage>
    else if (request->hasParam(PARAM_STOP1TRAINS)) {
      inputMessage = request->getParam(PARAM_STOP1TRAINS)->value();
      writeFile(LittleFS, "/Stop1Trains.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STOP1WALK)) {
      inputMessage = request->getParam(PARAM_STOP1WALK)->value();
      writeFile(LittleFS, "/Stop1Walk.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STOP2NAME)) {
      inputMessage = request->getParam(PARAM_STOP2NAME)->value();
      writeFile(LittleFS, "/Stop2Name.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STOP2TRAINS)) {
      inputMessage = request->getParam(PARAM_STOP2TRAINS)->value();
      writeFile(LittleFS, "/Stop2Trains.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STOP2WALK)) {
      inputMessage = request->getParam(PARAM_STOP2WALK)->value();
      writeFile(LittleFS, "/Stop2Walk.txt", inputMessage.c_str());
    }    
    else if (request->hasParam(PARAM_STOP3NAME)) {
      inputMessage = request->getParam(PARAM_STOP3NAME)->value();
      writeFile(LittleFS, "/Stop3Name.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STOP3TRAINS)) {
      inputMessage = request->getParam(PARAM_STOP3TRAINS)->value();
      writeFile(LittleFS, "/Stop3Trains.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STOP3WALK)) {
      inputMessage = request->getParam(PARAM_STOP3WALK)->value();
      writeFile(LittleFS, "/Stop3Walk.txt", inputMessage.c_str());
    }    
    else if (request->hasParam(PARAM_STOP4NAME)) {
      inputMessage = request->getParam(PARAM_STOP4NAME)->value();
      writeFile(LittleFS, "/Stop4Name.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STOP4TRAINS)) {
      inputMessage = request->getParam(PARAM_STOP4TRAINS)->value();
      writeFile(LittleFS, "/Stop4Trains.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_STOP4WALK)) {
      inputMessage = request->getParam(PARAM_STOP4WALK)->value();
      writeFile(LittleFS, "/Stop4Walk.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_GOFLAG)) {
      inputMessage = request->getParam(PARAM_GOFLAG)->value();
      writeFile(LittleFS, "/GoFlag.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_TXFLAG)) {
      inputMessage = request->getParam(PARAM_TXFLAG)->value();
      writeFile(LittleFS, "/TxFlag.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_REPINTERVAL)) {
      inputMessage = request->getParam(PARAM_REPINTERVAL)->value();
      writeFile(LittleFS, "/Report.txt", inputMessage.c_str());         // after report interval change, reset timer
      HttpGetStartMillis = millis();
      Interval = readFile(LittleFS, "/Report.txt").toInt();
      Serial.print("Timer Reloaded ");
      Serial.println(Interval);
    }   
    else {
      inputMessage = "No message sent"; 
    }
    //Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
  });
  server.onNotFound(notFound);
}


// ---------- MAIN LOOP -----------------------------------------------------------------------
//
void loop() {
    if (millis() - HttpGetStartMillis > 1000 * Interval) {                                    // time to update?
      HttpGetStartMillis = millis();
      LoadValuesForQuerry();
      
      if (GoFlag == "run"){
        Serial.println ("Time to get data!");
        connectWifi();
        for (zastavka_index = 0; zastavka_index < STOP_COUNT_TOTAL; zastavka_index++){
          Number_Of_Trips = Departure_Count[zastavka_index];                                  // count of future departures from this stop
          Stop_Ids = Stop_ID[zastavka_index];                                                 // load code of stop
          Stop_Ids.trim();
          Walk_Time = Walk_Distance[zastavka_index];
          Stop_Number = zastavka_index + 1;                         // index of Stop in Zivy Obraz starts with 1

          if (Stop_Ids != ""){
            Request_Train();                                        // ask for data and push it
            } else {
               Serial.println ("No data required for this stop.");
               if (zastavka_index == 0){
                writeFile(LittleFS, "/Zastavka1TextName.txt", "Not Used");
                }
               else if (zastavka_index == 1){
                writeFile(LittleFS, "/Zastavka2TextName.txt", "Not Used");
                }
               else if (zastavka_index == 2){
                writeFile(LittleFS, "/Zastavka3TextName.txt", "Not Used");
                }
               else {
                writeFile(LittleFS, "/Zastavka4TextName.txt", "Not Used");
                }
            }
        }
          Serial.println("--------- Done for the moment ------------------\n\n");
        } else {
          Serial.println ("Gateway in stopped status");
        }
   }
}
// -------------end of main loop ------------------------------------------------------------------


// ---------- Request_Train -----------------------------------------------------------------------
// Process one full stop, for all trains requested.
//
void Request_Train(){
  train_count = Number_Of_Trips.toInt();
  Ask_Golem();
DeserializationError error = deserializeJson(jsonBuffer, json);         // Deserialize the JSON response
if (error) {
  Serial.print(F("deserializeJson() failed: "));
  Serial.println(error.f_str());
  return;
 }

//Extract stop name from Golemio JSON response, print it to console, push it to Ž.O. It's first member of array.
const char* field = jsonBuffer["stops"][0]["stop_name"];
if (field == NULL) {
  field = ".";                                                       //in case there is NULL response, put in some dummy load
  }
Serial.print("Zastávka: ");
Serial.println(field);

if (zastavka_index == 0){
  writeFile(LittleFS, "/Zastavka1TextName.txt", field);
}
else if (zastavka_index == 1){
  writeFile(LittleFS, "/Zastavka2TextName.txt", field);
}
else if (zastavka_index == 2){
  writeFile(LittleFS, "/Zastavka3TextName.txt", field);
}
else {
  writeFile(LittleFS, "/Zastavka4TextName.txt", field);
}


//Prepare value name/index for Ž.O. and push it together with value in to Ž.O. server
Name = "Zastavka_" + String(Stop_Number) +"_Name";
Push_Data_To_LiPi(Name, field);

Serial.print ("Počet příštích spojů: ");                    // How many next depatures we aim for - it's time determined, even for cluster of stops 
Serial.println(train_count);                                // These are all already inside the JSON from Golemio
Serial.println();                                           // newline to console

String Arr_Time;
for (int i=0; i<train_count; i++) {                                           // extract necessary data for one departure after another
field = jsonBuffer["departures"][i]["departure_timestamp"]["predicted"];     // extract the departure time (use "arrival_timestamp" instead??)
//field = jsonBuffer["departures"][i]["arrival_timestamp"]["predicted"];
if (field == NULL) {
  Arr_Time = ".";
} else {
  String temp_time_string = String(field);
  int delimiter = temp_time_string.indexOf("T");
  Arr_Time = temp_time_string.substring(delimiter + 1, delimiter + 6);         // parse just the hour and minute from complete date&time record 
}
  Serial.print("Odjezd: ");
  Serial.println(Arr_Time);
  Name = "Z" + String(Stop_Number) + "_Poradi_" + String(i) + "_Odjezd";        // compose the value label/name for Ž.O.
  Push_Data_To_LiPi(Name, Arr_Time);                                            // and push it to Ž.O.

  Serial.print("Linka: ");
  field = jsonBuffer["departures"][i]["route"]["short_name"];                  // parse the line name i.e. line number
  String AC;
  if (field == NULL) {
    AC = ".";
  } else {
    if (jsonBuffer["departures"][i]["trip"]["is_air_conditioned"] == true){
      AC = "* " +  String(field);
    } else{
      AC = String(field);
    }
  }
  Serial.println(AC);
  Name = "Z" + String(Stop_Number) + "_Poradi_" + String(i) + "_Linka";       // compose the value label/name for Ž.O.
  Push_Data_To_LiPi(Name, AC);                                                // and push it to Ž.O.

  Serial.print("Směr: ");
  field = jsonBuffer["departures"][i]["trip"]["headsign"];                   // parse the direction - actually this is a vehicle headsign it works in case it goes to depot or is derouted
  if (field == NULL) {
    field = ".";
  }
  Serial.println(field);
  Name = "Z" + String(Stop_Number) + "_Poradi_" + String(i) + "_Smer";        // compose the value label/name for Ž.O.
  Push_Data_To_LiPi(Name, field);                                            // and push it to Ž.O.
  
  Serial.println();
  Serial.println();
 }
}


// ---------- Push_Data_To_LiPi -----------------------------------------------------------------------
// Takes one data item and pushes it to Zivy Obraz. Takes in consideration the TxFlag
//
void Push_Data_To_LiPi(String DataName, String DataValue){
  DataValue.replace(" ","%20");                                                           // replace spaces in names, to have clean http request
  HTTPClient http;
  String serverPath = ZO_Server_Name + Server_Key + "&" + DataName + "=" + DataValue;     // compose URL for Ž.O. server
  Serial.print ("Pushing to Ž.O.: ");
  Serial.print(serverPath);
  if (TxFlag == "yes"){
    http.begin(wifi, serverPath.c_str());
    int httpResponseCode = http.GET();      // Send HTTP GET request
    if (httpResponseCode>0) {
      Serial.print(" >> Response: ");
      Serial.print(httpResponseCode);
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.print(" >>Err code: ");
      Serial.println(httpResponseCode);
     }
      http.end();             // Free resources
      Serial.println();       // newline to console
      delay (1000);           //most probbably not needed, just not to piss off Živý Obraz server
  } else {
    Serial.println("  --> In silent mode, no TX to Zivy Obraz");   // newline to console
    }
}


// ---------- Ask_Golem ---------------------------------------------------------------------------------------
// Asks Golemio for data, if succesful, returns JSON with all departure data for specific stop
//
void Ask_Golem (){
  Serial.println("GET request to PID Golemio server");
  String serverName = Gol_Server_Name + Stop_Ids + "&total=" + Number_Of_Trips + "&preferredTimezone=Europe%2FPrague&minutesBefore=-" + Walk_Time + "&minutesAfter=720";  // server address, must include querry
  Serial.println(serverName);
  HTTPClient http;
  http.begin(serverName.c_str());
  http.addHeader("Content-Type", "application/json; charset=utf-8");
  http.addHeader("X-Access-Token", Auth_Token);
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
    payload.toCharArray(json, payload.length()+1);
    //Serial.println(json);
   } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
   }
  // Free resources
  http.end();
}


// ---------- LoadValuesForQuerry -----------------------------------------------------------------------------
// Loads value stored in LittleFS to prepare for the querry loops
//
void LoadValuesForQuerry(){
  Auth_Token = readFile(LittleFS, "/GolemToken.txt");
  Server_Key = readFile(LittleFS, "/ZivyObrazToken.txt");

  readFile(LittleFS, "/Stop1Name.txt").toCharArray(Stop_ID [0], readFile(LittleFS, "/Stop1Name.txt").length() + 1);
  readFile(LittleFS, "/Stop2Name.txt").toCharArray(Stop_ID [1], readFile(LittleFS, "/Stop2Name.txt").length() + 1);
  readFile(LittleFS, "/Stop3Name.txt").toCharArray(Stop_ID [2], readFile(LittleFS, "/Stop3Name.txt").length() + 1);
  readFile(LittleFS, "/Stop4Name.txt").toCharArray(Stop_ID [3], readFile(LittleFS, "/Stop4Name.txt").length() + 1);

  Departure_Count [0] = readFile(LittleFS, "/Stop1Trains.txt").toInt();
  Departure_Count [1] = readFile(LittleFS, "/Stop2Trains.txt").toInt();
  Departure_Count [2] = readFile(LittleFS, "/Stop3Trains.txt").toInt();
  Departure_Count [3] = readFile(LittleFS, "/Stop4Trains.txt").toInt();

  Walk_Distance [0] = readFile(LittleFS, "/Stop1Walk.txt").toInt();
  Walk_Distance [1] = readFile(LittleFS, "/Stop2Walk.txt").toInt();
  Walk_Distance [2] = readFile(LittleFS, "/Stop3Walk.txt").toInt();
  Walk_Distance [3] = readFile(LittleFS, "/Stop4Walk.txt").toInt();

  GoFlag = readFile(LittleFS, "/GoFlag.txt");
  TxFlag = readFile(LittleFS, "/TxFlag.txt");
  Interval = readFile(LittleFS, "/Report.txt").toInt();
}


// ---------- connectWifi ---------------------------------------------------------------------
// Reconnects to WiFi
//
void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return;            // ret if connected or already had problem - not to try forever
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      delay(5000);
      WiFi.disconnect();
      WiFi.reconnect();
      ESP.restart();
}


// ---------- Not Found ---------------------------------------------------------------------
//
void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}


// ---------- readFile ---------------------------------------------------------------------
// Reads value from LittleFS file
//
String readFile(fs::FS &fs, const char * path){
  //Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    //Serial.println("- empty file or failed to open file");
    return String();
  }
  //Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  //Serial.println(fileContent);
  return fileContent;
}


// ---------- writeFile ---------------------------------------------------------------------
// Writes value to LittleFS file
//
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: '%s' with value: '%s'\r\n", path, message);
  File file = fs.open(path, "w");
  if(!file){
    //Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    //Serial.println("- file written");
  } else {
    //Serial.println("- write failed");
  }
  file.close();
}


// ---------- processor ---------------------------------------------------------------------
// Replaces placeholder in HTML with stored values
//
String processor(const String& var){
  //Serial.print ("Processor >>>");
  //Serial.println(var);
  if(var == "GolemToken"){
    return readFile(LittleFS, "/GolemToken.txt");
  }
  else if(var == "ZivyObrazToken"){
    return readFile(LittleFS, "/ZivyObrazToken.txt");
  }
  else if(var == "Report"){
    return readFile(LittleFS, "/Report.txt");
  }
  else if(var == "Stop1Name"){
    return readFile(LittleFS, "/Stop1Name.txt");
  }
  else if(var == "Stop1Trains"){
    return readFile(LittleFS, "/Stop1Trains.txt");
  }
  else if(var == "Stop1Walk"){
    return readFile(LittleFS, "/Stop1Walk.txt");
  }
  else if(var == "Stop2Name"){
    return readFile(LittleFS, "/Stop2Name.txt");
  }
  else if(var == "Stop2Trains"){
    return readFile(LittleFS, "/Stop2Trains.txt");
  }
  else if(var == "Stop2Walk"){
    return readFile(LittleFS, "/Stop2Walk.txt");
  }
  else if(var == "Stop3Name"){
    return readFile(LittleFS, "/Stop3Name.txt");
  }
  else if(var == "Stop3Trains"){
    return readFile(LittleFS, "/Stop3Trains.txt");
  }
  else if(var == "Stop3Walk"){
    return readFile(LittleFS, "/Stop3Walk.txt");
  }
  else if(var == "Stop4Name"){
    return readFile(LittleFS, "/Stop4Name.txt");
  }
  else if(var == "Stop4Trains"){
    return readFile(LittleFS, "/Stop4Trains.txt");
  }
  else if(var == "Stop4Walk"){
    return readFile(LittleFS, "/Stop4Walk.txt");
  }
  else if(var == "GoFlag"){
    return readFile(LittleFS, "/GoFlag.txt");  
  }
  else if(var == "TxFlag"){
    return readFile(LittleFS, "/TxFlag.txt");  
  }
  else if(var == "Zastavka1TextName"){
    return readFile(LittleFS, "/Zastavka1TextName.txt");  
  }
  else if(var == "Zastavka2TextName"){
    return readFile(LittleFS, "/Zastavka2TextName.txt");  
  }
  else if(var == "Zastavka3TextName"){
    return readFile(LittleFS, "/Zastavka3TextName.txt");  
  }
  else if(var == "Zastavka4TextName"){
    return readFile(LittleFS, "/Zastavka4TextName.txt");  
  }
}


// ---------- saveConfigCallback ---------------------------------------------------------------------
//callback to save config after WiFi manager exit
//
void saveConfigCallback () {
  Serial.println("Should save config");
  // Save the user IP to filesystem
  writeFile(LittleFS, "/StaticIP.txt", (WiFi.localIP().toString()).c_str());
  writeFile(LittleFS, "/StaticGW.txt", (WiFi.gatewayIP().toString()).c_str());
  writeFile(LittleFS, "/StaticSN.txt", (WiFi.subnetMask().toString()).c_str());
  writeFile(LittleFS, "/StaticDN.txt", (WiFi.dnsIP(0).toString()).c_str());
  writeFile(LittleFS, "/GoFlag.txt", GoFlag.c_str());
  writeFile(LittleFS, "/TxFlag.txt", TxFlag.c_str());  
  writeFile(LittleFS, "/Report.txt", Reprt_Interval.c_str());
}


// ----- END
