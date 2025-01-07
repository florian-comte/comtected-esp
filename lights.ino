#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

// Pins for controlling the lights (modifiable)
const int LIGHT0 = 16; // Pin for light 0
const int LIGHT1 = 5;  // Pin for light 1
const int LIGHT2 = 4;  // Pin for light 2

const int LIGHTSNUMB = 3; // Number of lights
int lights[LIGHTSNUMB] = {LIGHT0, LIGHT1, LIGHT2}; // Array of light pins

// Wi-Fi credentials (modifiable)
const String WIFISSID = ""; // SSID of the Wi-Fi
const String WIFIPASS = ""; // Password of the Wi-Fi

// Server credentials (modifiable)
const String SERVERUSER = ""; // Username for server authentication
const String SERVERPASS = ""; // Password for server authentication
const int SERVERPORT = 3000; // Port for the server

// Network configuration variables (modifiable)
IPAddress local_IP(192, 168, 1, 100); // Local IP address
IPAddress gateway(192, 168, 1, 1);    // Gateway IP
IPAddress subnet(255, 255, 0, 0);     // Subnet mask
IPAddress primaryDNS(8, 8, 8, 8);     // Primary DNS
IPAddress secondaryDNS(8, 8, 4, 4);   // Secondary DNS

String hostname = ""; // Device hostname (modifiable)

// Array to store the current states of lights
String currentStates[LIGHTSNUMB] = {};

// Create an ESP8266 web server instance
ESP8266WebServer server(SERVERPORT);

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize current light states to "off"
  std::fill_n(currentStates, LIGHTSNUMB, "off");

  // Configure Wi-Fi settings
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.hostname(hostname);
  WiFi.begin(WIFISSID, WIFIPASS);

  // Wait for Wi-Fi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Enable Wi-Fi auto-reconnection
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Set light pins as outputs
  for (int i = 0; i < LIGHTSNUMB; i++) {
    pinMode(lights[i], OUTPUT);
  }

  // Set up server endpoints
  server.on("/", HTTP_GET, getCurrentStatus);
  server.on("/LIGHTON", HTTP_POST, lightOn);
  server.on("/LIGHTOFF", HTTP_POST, lightOff);
  server.onNotFound(handleNotFound);

  // Start the server and OTA updates
  server.begin();
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle(); // Handle OTA updates
  server.handleClient(); // Handle incoming HTTP requests
}

// Handle requests to undefined routes
void handleNotFound() {
  if (server.method() == HTTP_OPTIONS) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Max-Age", "10000");
    server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    server.send(204);
  } else {
    server.send(404, "text/plain", "");
  }
}

// Attempt to reconnect to Wi-Fi if disconnected
void handleConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Trying to reconnect...");
    WiFi.begin(WIFISSID, WIFIPASS);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to WiFi");
  }
}

// Send JSON response with the current light state
void sendResponse(String state) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Origin, Content-Type");

  DynamicJsonDocument jsonDoc(256);
  jsonDoc["state"] = state;

  String response;
  serializeJson(jsonDoc, response);

  server.send(200, "application/json", response);
}

// Handle GET request to retrieve the current status of a light
void getCurrentStatus() {
  int n = getGoodNumber();
  if (n != -1) {
    sendResponse(currentStates[n]);
  }
}

// Turn off the specified light
void lightOff() {
  int n = getGoodNumber();
  if (n != -1) {
    sendResponse("off");
    digitalWrite(lights[n], LOW);
    currentStates[n] = "off";
  }
}

// Turn on the specified light
void lightOn() {
  int n = getGoodNumber();
  if (n != -1) {
    sendResponse("on");
    digitalWrite(lights[n], HIGH);
    currentStates[n] = "on";
  }
}

// Validate and retrieve the light number from the request
int getGoodNumber() {
  if (server.args() != 1) {
    DynamicJsonDocument jsonDoc(256);
    jsonDoc["message"] = "Error: one param required";

    String response;
    serializeJson(jsonDoc, response);

    server.send(404, "application/json", response);
    return -1;
  }

  if (!isNumeric(server.arg(0))) {
    DynamicJsonDocument jsonDoc(256);
    jsonDoc["message"] = "Error: the param must be a number";

    String response;
    serializeJson(jsonDoc, response);

    server.send(404, "application/json", response);
    return -1;
  }

  int number = server.arg(0).toInt();
  if (number < 0 || number >= LIGHTSNUMB) {
    DynamicJsonDocument jsonDoc(256);
    jsonDoc["message"] = "Error: the param must be between 0 and 2";

    String response;
    serializeJson(jsonDoc, response);

    server.send(404, "application/json", response);
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
  for (unsigned int i = 0; i < stringLength; ++i) {
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
