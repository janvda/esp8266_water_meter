#include <ESP8266WiFiGratuitous.h>
#include <WiFiServerSecure.h>
#include <WiFiClientSecure.h>
#include <ArduinoWiFiServer.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESP8266WiFiType.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFiAP.h>
#include <WiFiClient.h>
#include <BearSSLHelpers.h>
#include <WiFiServer.h>
#include <ESP8266WiFiScan.h>
#include <WiFiServerSecureBearSSL.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiSTA.h>

/*
 Created by Jan Van den Audenaerde
 Based on some ESP8266 and PubSub code examples 
 
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h> 
#include "config.h"  // defines all constants starting with CFG_

const char* ssid = CFG_WIFI_SSID ;  
const char* password = CFG_WIFI_PASSWORD;
const char* host = CFG_HOST;

const char* mqttServer = CFG_MQTT_SERVER;
WiFiClient   espClient;
PubSubClient mqttClient(espClient);
long lastMsgTimestamp = 0;
char mqttPublishMsg[50];

// regarding GPIO pins functions https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html
int reedSwitchPin = 4;   // reed switch is connected GPIO4 = D2 (https://circuits4you.com/2017/12/31/nodemcu-pinout/)

volatile int pulseCount = 0;

void ICACHE_RAM_ATTR  onWaterMeterPulse() {
    pulseCount++;
    // Serial.println(pulseCount);
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\nPreparing the reed switch water meter project...");
    
    pinMode(reedSwitchPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(reedSwitchPin), onWaterMeterPulse, CHANGE);

    // setting up wifi connnection
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());  

    mqttClient.setServer(mqttServer, 1883);
}

void mqtt_reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish(CFG_MQTT_TOPIC_STATUS, "mqtt_reconnected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

  if (!mqttClient.connected()) {
    mqtt_reconnect();
  }
  mqttClient.loop();

  // sending an mqtt message every 5 second with pulse count
  long now = millis();
  if (now - lastMsgTimestamp > 5000) {
    
    lastMsgTimestamp = now;

    // publishing the water meter pulse count
    // Serial.print("pulse_count=");
    // Serial.println(pulseCount);
    snprintf(mqttPublishMsg, 50, "%d", pulseCount);
    mqttClient.publish(CFG_MQTT_TOPIC_WATER_METER_PULSE_COUNT , mqttPublishMsg);
  }
}
