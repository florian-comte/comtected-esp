#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

const int OPENPIN = 5; // Pin for open
const int PUMPPIN = 15; // Pin for pump
const int CLOSEPIN = 14; // Pin for close

const int TIMETOACTION = 82; // Time for the shutter to open or close in seconds

const String WIFISSID = ""; // SSID of the wifi
const String WIFIPASS = ""; // Password of the wifi
const String HOSTNAME = "";

const String SERVERUSER = ""; // Username of the server
const String SERVERPASS = ""; // Password of the server
const int SERVERPORT = 3000; // Port of the server

IPAddress local_IP(192, 168, 1, 102); // The local IP of this controller

// DO NOT TOUCH VARIABLES UNDER THIS LINE
unsigned long currentActionStart = 0;
String currentState = "Waiting";
ESP8266WebServer server(SERVERPORT); 

IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

// Setup function, executed one time at the boot
void setup() {
  Serial.begin(9600);
  
  // Register the pins as OUTPUT
  pinMode(OPENPIN, OUTPUT);
  pinMode(PUMPPIN, OUTPUT);
  pinMode(CLOSEPIN, OUTPUT);
  resetShutter();
  
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(WIFISSID, WIFIPASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  } 

  // Add endpoints on the server
  server.on("/", HTTP_POST, getCurrentStatus);
  server.on("/RESETSHUTTER", HTTP_POST, resetShutter);
  server.on("/OPENSHUTTER", HTTP_POST, openShutter);
  server.on("/CLOSESHUTTER", HTTP_POST, closeShutter);
  server.begin();
  ArduinoOTA.begin();
}

// Loop function, executing everytime
void loop() {
  unsigned long currentMillis = millis();
  if(currentActionStart != 0 && currentMillis - currentActionStart >= TIMETOACTION * 1000){ // Check if a process is running && if it is achieved
    resetShutter();
  }
  ArduinoOTA.handle();
  server.handleClient();
}

// OPEN, CLOSE, WAIT
void sendResponse(String state){
    server.send(200, "text/plain", state);
}

// Get the current status of a certain pin
void getCurrentStatus(){
  sendResponse(currentState);
}

// Open the shutter
void openShutter(){
  sendResponse("Opening");
  digitalWrite(CLOSEPIN, LOW);
  delay(3000);
  digitalWrite(OPENPIN, HIGH);
  currentActionStart = millis();
  currentState = "Opening";
}

void closeShutter(){
  sendResponse("Closing");
  digitalWrite(OPENPIN, LOW);
    // First we stop the pump of the swimming pool
  digitalWrite(PUMPPIN, HIGH); 
  delay(3000); 
  digitalWrite(CLOSEPIN, HIGH);
  currentActionStart = millis();
  currentState = "Closing";
}

void resetShutter(){
  digitalWrite(OPENPIN, LOW); 
  digitalWrite(CLOSEPIN, LOW);
  digitalWrite(PUMPPIN, LOW); 
  currentActionStart = 0;
  sendResponse("Waiting");
  currentState = "Waiting";
}
