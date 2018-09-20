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

int pin = 2;   // reed switch is connected GPIO2 = D4 (https://circuits4you.com/2017/12/31/nodemcu-pinout/)
volatile int pulseCount = 0;

void onWaterMeterPulse() {
    pulseCount++;
    Serial.println(pulseCount);
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\nPreparing the reed switch water meter project...");
    
    pinMode(pin, OUTPUT);  // should this not be INPUT instead of OUTPUT ?
    attachInterrupt(digitalPinToInterrupt(pin), onWaterMeterPulse, CHANGE);

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
    mqttClient.setCallback(mqttCallback);
}

// is not used
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

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
      // ... and resubscribe
      mqttClient.subscribe(CFG_MQTT_SUBSCRIBE_TOPIC);
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
    Serial.print("pulse_count=");
    Serial.println(pulseCount);
    snprintf(mqttPublishMsg, 50, "%d", pulseCount);
    mqttClient.publish(CFG_MQTT_TOPIC_WATER_METER_PULSE_COUNT , mqttPublishMsg);
  }
}

