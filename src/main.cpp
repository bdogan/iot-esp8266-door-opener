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
const char* MQTT_SERVER = "iot.burak-dogan.com";
char MQTT_NODE[19];
const char* MQTT_USER = "iotuser";
const char* MQTT_PASS = "BUR@k111";
//const char* WIFI_SSID = "PlatinMarket Private";
const char* WIFI_SSID = "Dogan's Wi-Fi";
//const char* WIFI_PASSWORD = "PGAJ6ZAKVPKE";
const char* WIFI_PASSWORD = "BUR@k111";

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

//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {

	//convert topic to string to make it easier to work with
	String topicStr = topic;

	//this is a fix because the payload passed to the callback isn't null terminated
	//so we create a new string with a null termination and then turn that into a string
	char payloadFixed[100];
	memcpy(payloadFixed, payload, length);
	payloadFixed[length] = '\0';
	String payloadStr = (char*)payloadFixed;

	char byteToSend = 0;

  Serial.println("Topic came:" + topicStr);
  Serial.println(payloadStr);
}

/*
 * Make MQTT Server connection
 */
void connectMQTT() {
  while (!pClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (pClient.connect(MQTT_NODE, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      // ... and subscribe to topic
      pClient.setCallback(callback);
      pClient.subscribe("/house/door1");
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

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
  }
  result.toUpperCase();
  return result;
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
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  Wifi.macAddress(mac);
  Wifi.setAutoConnect(true);
  Wifi.begin(WIFI_SSID, WIFI_PASSWORD);
  connectWifi();

  String macAddress = "ESP8266-" + macToStr(mac);
  memset(MQTT_NODE, 0, macAddress.length() + 1);
  for (int i=0; i < macAddress.length(); i++) MQTT_NODE[i] = macAddress.charAt(i);

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
  pClient.setServer(MQTT_SERVER, 8883);

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

  if (!pClient.connected()) {
    connectMQTT();
  }

  pClient.loop();

  delay(500);
}
