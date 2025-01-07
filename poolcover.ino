// ESP8266 Pool cover controller - By Florian COMTE

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>

// Pin definitions
const int OPENPIN = 5;    // GPIO pin for opening the cover
const int PUMPPIN = 15;   // GPIO pin for the pump
const int CLOSEPIN = 14;  // GPIO pin for closing the cover

// Time in seconds for the cover to fully open or close
const int TIMETOACTION = 82;  

// Wi-Fi and server credentials (replace with actual credentials)
const String WIFISSID = ""; // Wi-Fi SSID
const String WIFIPASS = ""; // Wi-Fi Password
const String SERVERUSER = ""; // Server Username
const String SERVERPASS = ""; // Server Password
const int SERVERPORT = 3000;  // Port of the server

// Internal variables for managing state and timing
unsigned long currentActionStart = 0;  // Start time of current action
unsigned long previousMillis = 0;      // Tracks the last update time
String currentState = "waiting";       // Current state of the cover
float closedPourcent = 100.00;         // Percentage closed (0-100%)
ESP8266WebServer server(SERVERPORT);   // Web server instance

void setup() {
  Serial.begin(9600);
  
  // Configure GPIO pins
  pinMode(OPENPIN, OUTPUT);
  pinMode(PUMPPIN, OUTPUT);
  pinMode(CLOSEPIN, OUTPUT);
  resetShutter();  // Initialize cover to "waiting" state
  
  // Connect to Wi-Fi
  WiFi.begin(WIFISSID, WIFIPASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Define server routes and start the server
  server.on("/", HTTP_GET, getCurrentStatus);
  server.on("/RESETSHUTTER", HTTP_POST, resetShutter);
  server.on("/OPENSHUTTER", HTTP_POST, openShutter);
  server.on("/CLOSESHUTTER", HTTP_POST, closeShutter);
  server.onNotFound(handleNotFound);
  
  server.begin();
  ArduinoOTA.begin();  // Start OTA updates
}

// Handle requests to undefined endpoints
void handleNotFound() {
    if (server.method() == HTTP_OPTIONS) {
        // Send CORS headers for OPTIONS requests
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Max-Age", "10000");
        server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
        server.send(204);
    } else {
        server.send(404, "text/plain", "");
    }
}

void loop() {
  if (currentActionStart != 0) {
    unsigned long currentMillis = millis();
    
    // Update closedPourcent every second
    if (currentMillis - previousMillis >= 1000) {
      float percentageDiffFor1Sec = (1000 * 100) / (float)(TIMETOACTION * 1000);
      if (currentState == "opening") {
        closedPourcent -= percentageDiffFor1Sec;  // Decrease percentage while opening
      } else if (currentState == "closing") {
        closedPourcent += percentageDiffFor1Sec;  // Increase percentage while closing
      }

      // Constrain closedPourcent between 0 and 100
      if (closedPourcent > 100) {
        closedPourcent = 100;
      } else if (closedPourcent < 0) {
        closedPourcent = 0;
      }
      
      previousMillis = currentMillis;
    }
    
    // Check if the action duration has completed
    if (currentMillis - currentActionStart >= TIMETOACTION * 1000) {
      // Finalize closedPourcent based on the action
      if (currentState == "closing") {
        closedPourcent = 100.00;
      } else if (currentState == "opening") {
        closedPourcent = 0.00;
      }
    
      resetShutter();  // Reset cover to "waiting" state
    }
  }
  
  ArduinoOTA.handle();     // Handle OTA updates
  server.handleClient();   // Handle client requests
}

// Sends a JSON response with the current state and closedPourcent
void sendJsonResponse(String state, int percentage) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Origin, Content-Type");
    
    // Create a JSON document
    DynamicJsonDocument jsonDoc(256);
    jsonDoc["currentState"] = state;
    jsonDoc["closedPourcent"] = (int)percentage;

    // Serialize JSON document to string
    String response;
    serializeJson(jsonDoc, response);

    server.send(200, "application/json", response);
}

// Endpoint to get the current status of the cover
void getCurrentStatus() {
  sendJsonResponse(currentState, closedPourcent);
}

// Initiates the opening sequence of the cover
void openShutter() {
  sendJsonResponse("opening", closedPourcent);
  digitalWrite(CLOSEPIN, LOW); // Ensure close pin is off
  digitalWrite(PUMPPIN, LOW);  // Turn off pump
  delay(3000);
  digitalWrite(OPENPIN, HIGH); // Activate open pin
  currentActionStart = millis();
  previousMillis = millis();
  currentState = "opening";
}

// Initiates the closing sequence of the cover
void closeShutter() {
  sendJsonResponse("closing", closedPourcent);
  digitalWrite(OPENPIN, LOW);  // Ensure open pin is off
  digitalWrite(PUMPPIN, HIGH); // Activate pump
  delay(3000);
  digitalWrite(CLOSEPIN, HIGH); // Activate close pin
  currentActionStart = millis();
  previousMillis = millis();
  currentState = "closing";
}

// Resets the cover to the "waiting" state, stopping all actions
void resetShutter() {
  sendJsonResponse("waiting", closedPourcent);
  digitalWrite(OPENPIN, LOW);   // Turn off open pin
  digitalWrite(CLOSEPIN, LOW);  // Turn off close pin
  digitalWrite(PUMPPIN, LOW);   // Turn off pump
  currentActionStart = 0;
  currentState = "waiting";
}
