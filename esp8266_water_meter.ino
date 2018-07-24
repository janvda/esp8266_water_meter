/*
 Created by Jan Van den Audenaerde
 Based on some ESP8266 and PubSub code examples 
 
*/

// stuff needed for DHT22 sensor
#include "DHT.h"
#define DHTPIN 4     // what digital pin we're connected to

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

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

int pin = 2;   // reed switch is connected GPIO2 (https://circuits4you.com/2017/12/31/nodemcu-pinout/)
volatile int pulseCount = 0;

void onWaterMeterPulse() {
    pulseCount++;
    Serial.println(pulseCount);
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    delay(100);
    Serial.println("Preparing the reed switch water meter project...");
    
    pinMode(pin, OUTPUT);  // should this not be INPUT instead of OUTPUT ?
    attachInterrupt(digitalPinToInterrupt(pin), onWaterMeterPulse, CHANGE);

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
      mqttClient.publish("water_meter/status", "mqtt_reconnected");
      // ... and resubscribe
      mqttClient.subscribe("inTopic");
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
    Serial.print("water_meter/pulse_count=");
    Serial.println(pulseCount);
    snprintf(mqttPublishMsg, 50, "%d", pulseCount);
    mqttClient.publish("water_meter/pulse_count", mqttPublishMsg);

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    if (! isnan(h)){
      Serial.print("nodeMCU/humidity= ");
      Serial.println(h);
      snprintf(mqttPublishMsg, 50, "%f", h);
      mqttClient.publish("nodeMCU/humidity", mqttPublishMsg);
    }

    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    if (! isnan(t)){
      Serial.print("nodeMCU/temperature=");
      Serial.print(t);
      Serial.println(" °C");
      snprintf(mqttPublishMsg, 50, "%f", t);
      mqttClient.publish("nodeMCU/temperature", mqttPublishMsg);
    }
  }
}

