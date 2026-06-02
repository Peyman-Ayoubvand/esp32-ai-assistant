// ==========================
// Libraries
// ==========================
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Secrets.h"


// ==========================
// WiFi Credentials
// Fallback Credentials
// ==========================
const char* ssid = "Honor X8";
//const char* password = PASSWORD Loaded From Secrets.h To Keep Credentials Separate From Main Code
// ==========================
// Web Server (Port 80)
// ==========================
WebServer server(80);

// ==========================
// OpenAI + OpenWeather
// ==========================
//const char* openAIAPIKey =  API Keys Loaded From Secrets.h To Keep Credentials Separate From Main Code
//const char* openWeatherAPIKey =  API Keys Loaded From Secrets.h To Keep Credentials Separate From Main Code
const char* units = "metric";
// ==========================
// Runs Once at Startup
// ==========================
void setup() {

  // ==========================
  // Serial Monitor
  // Debug Output
  // ==========================
  Serial.begin(115200);

  // ==========================
  // WiFi Station Mode
  // ==========================
  WiFi.mode(WIFI_STA);

  // ==========================
  // WiFiManager
  // Auto Connect / Captive Portal
  // ==========================
  WiFiManager wm;
  bool connected = wm.autoConnect(ssid,password);

  if(!connected){
    Serial.println("Failed to connect");
  }else{
    Serial.println("Connected!");
  }

  // ==========================
  // Wait For WiFi Connection
  // ==========================
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // ==========================
  // Network Information
  // ==========================
  Serial.println("");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // ==========================
  // mDNS Initialization
  // ==========================
  if(MDNS.begin("ESP32Assistant")){
    Serial.println("MDNS Initialize");
    Serial.println("url : http://ESP32Assistant.local");
  }else{
    Serial.print("MDNS Failed!");
  }

  // ==========================
  // SPIFFS Initialization
  // ==========================
  if (!SPIFFS.begin(true)){
    Serial.print("SPIFFS NOT MOUNT!");
    return;
  }

  // ==========================
  // API Endpoints
  // ==========================
  server.on("/chat",HTTP_POST ,handleChat);
  server.on("/weather",HTTP_POST ,handleWeather);

  // ==========================
  // Static File Server
  // ==========================
  server.onNotFound([](){
    if(!handleFileRead(server.uri())){
      server.send(404,"text/plain","File Not Found");
    }
  });

  // ==========================
  // List SPIFFS Files
  // ==========================
  listFiles();

  // ==========================
  // Start HTTP Server
  // ==========================
  server.begin();
}

// ==========================
// Handle HTTP Requests
// ==========================
void loop() {
  server.handleClient();
}


// ==========================
// Send Prompt To OpenAI
// Return AI Response
// ==========================

String sendToOpenAI(String userMessage){
  HTTPClient http;

  String uri = "https://api.openai.com/v1/responses";

  http.begin(uri);

  http.addHeader("Content-Type","application/json");
  http.addHeader("Authorization","Bearer "+ String(openAIAPIKey));

  StaticJsonDocument<512>doc;

  doc["model"] = "gpt-4o";
  doc["input"] = userMessage;
  doc["instructions"] = "You are a friendly AI assistant. Answer naturally and conversationally.";
  doc["temperature"]= 0.7;

  String payload;
  serializeJson(doc,payload);
  

  int httpCode = http.POST(payload);

  if (httpCode > 0){
    String response = http.getString();

    StaticJsonDocument<1024>responseDoc;
    DeserializationError error = deserializeJson(responseDoc , response);

    if (error){
      Serial.println("Deserialization error ");
      Serial.println(error.c_str());
      return "ERROR";
    }

    String gptResponse = responseDoc["output"][0]["content"][0]["text"];
    return String(gptResponse);
  }else{
    Serial.print("Error in HTTP request!");
  }
  http.end();
}


// ==========================
// Get Weather From OpenWeather
// Return Weather Summary
// ==========================

String getWeatherData(String city){
  HTTPClient http;

  String uri = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "&appid=" + String(openWeatherAPIKey) + "&units=" + String(units);
  http.begin(uri);

  int httpCode = http.GET();

  if (httpCode > 0){
    String payload = http.getString();
    Serial.println(payload);

    StaticJsonDocument<1024>doc;
    DeserializationError error = deserializeJson(doc,payload);

    if (error){
      Serial.println("Deserializtion Error");
      Serial.println(error.c_str());
      return "";
    }

    String weather = doc["weather"][0]["description"];
    float temperature = doc["main"]["temp"];
    int humidity = doc["main"]["humidity"];
    float windSpeed = doc["wind"]["speed"];

    String weatherString = "The weather in " + city + " is " + weather + ", the temperature is " + String(temperature) + " °C, the humidity is " + String(humidity) + "%, and the wind speed is " + String(windSpeed) + " m/s.";
    Serial.println(weatherString);
    return weatherString;
  }
  http.end();
}



// ==========================
// OpenAI Chat Endpoint
// ==========================
void handleChat(){

  if (!server.hasArg("plain")){
    server.send(400,"text/plain","Bad Request");
    return;
  }

  String body = server.arg("plain");

  StaticJsonDocument<1024>doc;
  DeserializationError error = deserializeJson(doc,body);

  if (error){
    Serial.println("Deserialize Error");
    Serial.println(error.c_str());
    return;
  }

  String userMessage = doc["message"];
  String chatResponse = sendToOpenAI(userMessage);

  if (chatResponse == ""){
    server.send(500,"text/plain","Internal Server Error!");
  }else{
    StaticJsonDocument<200>chatDoc;
    chatDoc["response"] = chatResponse;

    String response;
    serializeJson(chatDoc,response);
    server.send(200,"application/json",response);
  }
}

// ==========================
// Weather Request Endpoint
// ==========================
void handleWeather(){

  if (!server.hasArg("plain")){
    server.send(400,"text/plain","Bad Request");
    return;
  }

  String body = server.arg("plain");

  StaticJsonDocument<1024>doc;
  DeserializationError error = deserializeJson(doc,body);

  if (error){
    Serial.println("Deserialization Error");
    Serial.println(error.c_str());
    return;
  }

  String userMessage = doc["message"];

  String promptMessage = "This is a user Input: "+ userMessage +"Please extract the name fo city that the user mentioned. Retern only the name of city nothing else.";

  String weatherCity = sendToOpenAI(promptMessage);

  weatherCity.trim();
  weatherCity.replace(" ","%20");

  String weatherResponse = getWeatherData(weatherCity);

  if (weatherResponse == ""){
    server.send(500,"text/plain","Internal Server Error!");
    return;
  }else{
    StaticJsonDocument<1024>weatherDoc;
    weatherDoc["response"] = weatherResponse;

    String response;
    serializeJson(weatherDoc,response);
    server.send(200,"application/json",response);
  }
}

// ==========================
// MIME Type Detection
// ==========================
String getContentType(String filename){

  if (filename.endsWith(".html")){
    return "text/html";
  }
  else if (filename.endsWith(".css")){
    return "text/css";
  }
  else if (filename.endsWith(".js")){
    return "application/javascript";
  }
  else if (filename.endsWith(".png")){
    return "image/png";
  }
  else{
    return "text/plain";
  }
}

// ==========================
// Read File From SPIFFS
// ==========================
bool handleFileRead(String path){

  if (path.endsWith("/")){
    path += "index.html";
  }

  String contentType = getContentType(path);

  File file = SPIFFS.open(path,"r");

  if (!file){
    return false;
  }
  else{
    server.streamFile(file,contentType);
    file.close();
    return true;
  }
}

// ==========================
// List Files In SPIFFS
// ==========================
void listFiles(){

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while(file){

    Serial.print("File Name: ");
    Serial.println(file.name());

    Serial.print("File Size: ");
    Serial.println(file.size());

    Serial.println();

    file = root.openNextFile();
  }
}
