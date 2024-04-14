#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

const int LIGHTSNUMB = 2; // Number of lights
const int LIGHT0 = 16; // Pin for light 0
const int LIGHT1 = 4; // Pin for light 1

const String WIFISSID = ""; // SSID of the wifi
const String WIFIPASS = ""; // Password of the wifi
const String HOSTNAME = "";

const String SERVERUSER = ""; // Username of the server
const String SERVERPASS = ""; // Password of the server
const int SERVERPORT = 3000; // Port of the server

IPAddress local_IP(192, 168, 1, 102); // The local IP of this controller

// DO NOT TOUCH VARIABLES UNDER THIS LINE
String currentStates[LIGHTSNUMB] = {};
int lights[LIGHTSNUMB] = {LIGHT0,LIGHT1};
ESP8266WebServer server(SERVERPORT); 

IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

// Setup function, executed one time at the boot
void setup() {
  Serial.begin(9600);
  std::fill_n(currentStates, LIGHTSNUMB, "Off");

  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFISSID, WIFIPASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  } 
  
  // Register the pins
  for(int i = 0; i < LIGHTSNUMB; i++){
      pinMode(lights[i], OUTPUT);
  }

  // Add endpoints on the server
  server.on("/", HTTP_POST, getCurrentStatus);
  server.on("/LIGHTON", HTTP_POST, lightOn);
  server.on("/LIGHTOFF", HTTP_POST, lightOff);
  server.begin();
  ArduinoOTA.begin();
}

// Loop function, executing everytime
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
}

// Send answer after receving request with the current state
void sendResponse(String state){
  server.send(200, "text/plain", state);
}

// Get the current status of a certain pin
void getCurrentStatus(){
  int n = getGoodNumber();
  if(n != -1){
      sendResponse(currentStates[n]);
  }
}

// Switch off a certain pin
void lightOff(){
  int n = getGoodNumber();
  String s = "LIGHT" + String(n);
  if(n != -1){
    sendResponse("Off");
    digitalWrite(lights[n], LOW);
    currentStates[n] = "Off";
  }
}

// Switch on a certain pin
void lightOn(){
  int n = getGoodNumber();
  String s = "LIGHT" + String(n);
  if(n != -1){
    sendResponse("On");
    digitalWrite(lights[n], HIGH);
    currentStates[n] = "On";
  }
}

// Extract the pin affected from the request
int getGoodNumber(){
  if(server.args() != 1){
    server.send(404, "text/plain", "Error: one param required");
    return -1;
  }

  if(!isNumeric(server.arg(0))){
    server.send(404, "text/plain", "Error: the param must be a number");
    return -1;
  }

  int number = (server.arg(0)).toInt();
  if(number < 0 || number > 2){
    server.send(404, "text/plain", "Error: the param must be between 0 and 2");
    return -1;  
  }

  return number;
}

// Check if a string is numeric
boolean isNumeric(String str) {
    unsigned int stringLength = str.length();
 
    if (stringLength == 0) {
        return false;
    }
 
    boolean seenDecimal = false;
 
    for(unsigned int i = 0; i < stringLength; ++i) {
        if (isDigit(str.charAt(i))) {
            continue;
        }
 
        if (str.charAt(i) == '.') {
            if (seenDecimal) {
                return false;
            }
            seenDecimal = true;
            continue;
        }
        return false;
    }
    return true;
}