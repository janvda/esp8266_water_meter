/*
 Author Jan Van den Audenaerde
 Date 2018-10-08

 This sketch is used to measure the water consumption of an Elster V100 (15 or 20mm) meter
 https://www.elstermetering.com/assets/products/products_elster_files/ELS10138_V100_Specsheet_V2_SV.pdf
 using a reed switch.
 According those specs: this water meter model generates a pulse every 0.5 l and has a maximum flow rate of 5 m3/h.
 In other words we maximally can expect 2.78 pulses/sec   (= 5 m3/h * 1000 l/m3 * 1h/3600 sec * 1 pulse/0.5 l)
 Or the minimum time between 2 successive pulses is 360 millis.
 => therefore we have set our debouncing time to 100 millis (most likely we can set it even higher).

 This sketch is Based on 
   - some ESP8266 and PubSub code examples 
   - interrupt handler debouncing logic from Delphiño K.M.
     (see https://forum.arduino.cc/index.php?topic=466640.0 )

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h> 
#include "config.h"  // defines all constants starting with CFG_

const char* ssid      = CFG_WIFI_SSID ;  
const char* password  = CFG_WIFI_PASSWORD;
const char* host      = CFG_HOST;

const char* mqttServer = CFG_MQTT_SERVER;
WiFiClient   espClient;
PubSubClient mqttClient(espClient);
long lastMsgTimestamp = 0;
char mqttPublishMsg[50];

// regarding GPIO pins functions https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html
int reedSwitchPin = 4;   // reed switch is connected GPIO4 = D2 (https://circuits4you.com/2017/12/31/nodemcu-pinout/)

/***************  INTERRUPT HANDLER ***********************************/
volatile int pulseCount = 0;
volatile int rawPulseCount = 0;  // this is the pulseCount we get without the software debouncing

volatile unsigned long lastPulseTimeStamp = 0;
unsigned long debouncingTime = 100*1000; //  Debouncing Time in Microseconds - set to 0.1 second.

void ICACHE_RAM_ATTR onWaterMeterPulse() {
   unsigned long current_time; // current time in micro seconds.
   current_time=micros();  // rolls over every 71.6 minutes
      
   rawPulseCount++;
   
   // A (rising) pulse is only counted it the previous counted pulse happened more than debouncingTime microseconds ago
   // overflow is taken into account as specified in https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover ))
   if( (current_time - lastPulseTimeStamp) >= debouncingTime ){
      pulseCount++;
      lastPulseTimeStamp = current_time;
      // Serial.println(pulseCount);
    }
}
/*************** END OF INTERRUPT HANDLER ***********************************/

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\nPreparing the reed switch water meter project...");
    
    pinMode(reedSwitchPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(reedSwitchPin), onWaterMeterPulse, RISING);

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

    // publishing the water meter raw pulse count   
    snprintf(mqttPublishMsg, 50, "%d", rawPulseCount);
    mqttClient.publish(CFG_MQTT_TOPIC_WATER_METER_RAW_PULSE_COUNT , mqttPublishMsg);
  }
}
