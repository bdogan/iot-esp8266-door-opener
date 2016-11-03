// Arduino Library
#include "Arduino.h"

// Wifi Libraries
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Web Server
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// mDNS Client
#include <ESP8266mDNS.h>

// File System
#include "FS.h"

// MQTT Client
#include <PubSubClient.h>

// Constants
const int RELAY_PORT = D3;
const int STATUS_LED = LED_BUILTIN;
const int WEB_SERVER_PORT = 9292;
const char* MQTT_SERVER = "192.168.10.2";
const char* MQTT_NODE = "eps8266-door-1";
const char* MQTT_USER = "node1";
const char* MQTT_PASS = "node1";
//const char* WIFI_SSID = "PlatinMarket Private";
const char* WIFI_SSID = "IOT";
//const char* WIFI_PASSWORD = "PGAJ6ZAKVPKE";
const char* WIFI_PASSWORD = "1024603062A";

// Variables
bool opening = false;

// Declerations
MDNSResponder mdns;
ESP8266WiFiClass Wifi;
WiFiClient net;
PubSubClient pClient(net);
ESP8266WebServer webServer(WEB_SERVER_PORT);

/*
 * Make wireless connection
 */
void connectWifi() {
  Serial.print("Waiting wireless connection.");
  while (!Wifi.isConnected()) {
    Serial.print(".");
    digitalWrite(STATUS_LED, LOW);
    delay(200);
    digitalWrite(STATUS_LED, HIGH);
    delay(200);
  }
  Serial.println("Connected!");
  Serial.println("SSID: " + Wifi.SSID() + " & IpAddress: " + Wifi.localIP().toString());
  digitalWrite(STATUS_LED, HIGH);
}

/*
 * Make MQTT Server connection
 */
void connectMQTT() {
  while (!pClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (pClient.connect(MQTT_NODE)) {
      Serial.println("connected");
      // ... and subscribe to topic
      pClient.subscribe("ledStatus");
    } else {
      Serial.print("failed, rc=");
      Serial.print(pClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  digitalWrite(STATUS_LED, HIGH);
}

/*
 * Open door
 */
void openDoor() {
  if (opening) return;
  opening = true;
  digitalWrite(RELAY_PORT, LOW);
  delay(1500);
  digitalWrite(RELAY_PORT, HIGH);
  opening = false;
}

/*
 * Open door from web server
 */
void webOpenDoor() {
  webServer.send(200, "text/plain", "OK");
  openDoor();
}

/*
 * Web not found page
 */
void webNotFound() {
  webServer.send(404, "text/plain", "404 - Not Found");
}

/*
 * Web server index
 */
void webIndex() {
  //File f = SPIFFS.open("/index.html", "r");
  webServer.send(200, "text/plain", "Hello World");
}

/*
 * Setup
 */
void setup(){
  // Set serial
  Serial.begin(115200);
  Serial.println("                ");
  Serial.println("Door opener v1.0");
  Serial.println("Based on ESP8266");
  Serial.println("Author Burak Doğan <bdogan85@gmail.com>");

  // File system begin
  if (SPIFFS.begin()) {
    Serial.println("SPI FileSystem started");
  }

  // Pin config
  pinMode(RELAY_PORT, OUTPUT);
  digitalWrite(RELAY_PORT, HIGH);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, HIGH);

  // Connect Wifi
  Wifi.mode(WIFI_STA);
  Wifi.setAutoConnect(true);
  Wifi.begin(WIFI_SSID, WIFI_PASSWORD);
  connectWifi();

  // Set web server callbacks
  webServer.on("/open", webOpenDoor);
  webServer.on("/", webIndex);
  webServer.onNotFound(webNotFound);
  webServer.begin();
  Serial.println("Web server started at port " + (String)WEB_SERVER_PORT);

  // Start mDns
  if (mdns.begin(MQTT_NODE, WiFi.localIP())) {
    Serial.println("MDNS responder started with node " + (String)MQTT_NODE);
  }

  // MQTT Client
  pClient.setServer(MQTT_SERVER, 1883);

  connectMQTT();

}

/*
 * Main Loop
 */
void loop() {

  // Check Wireless connection
  if (!Wifi.isConnected()) {
    connectWifi();
  }

  // Handle Web Request
  webServer.handleClient();

  delay(500);
}
